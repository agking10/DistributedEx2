#include "machine.h"

Machine::Machine(int n_packets
        , int machine_index
        , int n_machines
        , int loss_rate) 
        : n_packets_to_send_(n_packets)
        , id_(machine_index - 1)
        , n_machines_(n_machines)
        , rng_dst_(1, 1000000)
        , out_file_("Machine_" + std::to_string(id_))
    {
        std::cout << "Initializing machine " << machine_index << " of " << n_machines
        << ".\nPrepared to send " << n_packets << " packets." << std::endl;
        recv_dbg_init(loss_rate, machine_index);
        packets_ = std::vector<std::vector<Message>>(n_machines, std::vector<Message>(WINDOW_SIZE));
        last_rec_cont_
        = last_rec_
        = last_delivered_
        = std::vector<int>(n_machines, 0);

        last_acked_ = std::vector<AbsoluteTimestamp>(n_machines);
        done_sending_ = std::vector<bool>(n_machines, false);
        send_addr_.sin_family = AF_INET;
        send_addr_.sin_addr.s_addr = htonl(MCAST_ADDR);  /* mcast address */
        send_addr_.sin_port = htons(PORT);
        generator_.seed(0);
    }

void Machine::start() 
{
    sockaddr_in name;
    ip_mreq mreq;
    unsigned char ttl_val;

    rec_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (rec_socket_ < 0)
    {
        perror("Mcast: socket");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if (bind(rec_socket_, (sockaddr *)&name, sizeof(name)) < 0)
    {
        perror("Mcast: bind");
        exit(1);
    }

    mreq.imr_multiaddr.s_addr = htonl(MCAST_ADDR);

    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    if (setsockopt(rec_socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, 
        sizeof(mreq)) < 0) 
    {
        perror("Mcast: problem in setsockopt to join multicast address" );
    }

    send_socket_ = socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
    if (send_socket_ < 0) 
    {
        perror("Mcast: socket");
        exit(1);
    }

    ttl_val = 1;
    if (setsockopt(send_socket_, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val, 
        sizeof(ttl_val)) < 0)
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d \n", ttl_val );
    }

    wait_for_start_signal();
    start_protocol();
}

void Machine::wait_for_start_signal()
{
    int bytes;
    int num;
    while (1)
    {
        bytes = recv( rec_socket_, reinterpret_cast<char*>(&message_buf_)
            , sizeof(message_buf_), 0);
        if (bytes < sizeof(Message)) continue;
        if (message_buf_.type == MessageType::START) break;
    }
}

void Machine::start_protocol()
{
    int num;
    FD_ZERO( &mask_ );
    FD_ZERO( &write_mask_);
    FD_ZERO( &read_mask_ );
    FD_ZERO( &excep_mask_ );
    FD_SET( rec_socket_, &mask_ );
    send_new_packets();
    int timeout = (n_machines_ + 1) * MAX_RETRANSMIT;
    while (1)
    {
        read_mask_ = mask_;
        timeout_.tv_usec = MACHINE_TIMEOUT;
        num = select( FD_SETSIZE
            , &read_mask_, &write_mask_, &excep_mask_, &timeout_);
        if (FD_ISSET(rec_socket_, &read_mask_))
        {
            handle_packet_in();
            ++timeout_counter_;
            if (timeout_counter_ > timeout)
            {
                // Potential termination bug
                send_undelivered_packets();
                timeout_counter_ = 0;
            }
        }
        else if (num == 0)
        {
            send_undelivered_packets();
        }
        if (all_empty())
        {
            out_file_.close();
            exit(0);
        }
    }
}

// Iterate starting at the last packet we haven't been able to deliver
// to the last packet we want to resend
void Machine::send_undelivered_packets()
{
    for (int i = last_delivered_[id_] + 1; 
        i <= std::min(last_sent_, last_delivered_[id_] + MAX_RETRANSMIT); i++)
    {
        send_packet(i);
    }
}

void Machine::send_new_packets()
{
    for (int i = last_sent_ + 1; 
        i < last_delivered_[id_] + WINDOW_SIZE - WINDOW_PADDING; i++)
    {
        if (i >= n_packets_to_send_)
        {
            finished_sending_ = true;
        }
        write_packet(i, finished_sending_);
        send_packet(i);
        ++last_sent_;
        ++last_rec_cont_[id_];
        ++last_rec_[id_];
    }
    update_last_acked();
}

void Machine::update_last_acked()
{
    AbsoluteTimestamp min(INT_MAX, 0);
    for (int i = 0; i < n_machines_; i++)
    {
        Message& m = packets_[i][last_rec_cont_[i] % WINDOW_SIZE];
        if (m.stamp < min) 
            min = m.stamp;
    }
    last_acked_[id_] = min;
}

void Machine::write_packet(int index, bool is_empty)
{
    Message& packet = packets_[id_][index % WINDOW_SIZE];
    packet.index = index;
    packet.magic_number = generate_magic_number();
    packet.stamp.machine = id_;
    packet.stamp.timestamp = ++timestamp_;
    packet.type = is_empty ? MessageType::EMPTY : MessageType::DATA;
}

// Attach cumulative ack to message, send to mcast
void Machine::send_packet(Message& msg)
{
    msg.ready_to_deliver = last_acked_[id_];
    msg.last_delivered = last_acked_all_;
    sendto(send_socket_, reinterpret_cast<const char *>(&msg)
        , sizeof(Message), 0, (sockaddr*)&send_addr_, sizeof(send_addr_));
}

void Machine::send_packet(int index)
{
    send_packet(packets_[id_][index % WINDOW_SIZE]);
    if (index % 1000 == 0) {
        printf("index: %d\n", index);
        fflush(0);
    }
}

// check if done_sending for each machine is true
bool Machine::all_empty()
{
    return std::all_of(done_sending_.begin(), done_sending_.end()
        , [](bool done){ return done; });
}

void Machine::handle_packet_in()
{

    sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes = recv_dbg(rec_socket_
        , reinterpret_cast<char*>(&message_buf_)
        , sizeof(Message), 0);
    if (bytes < sizeof(Message))
    {
        //std::cerr << "Short read" << std::endl;
        return;
    }

    if (message_buf_.stamp.machine == id_) return;

    if (message_buf_.stamp.timestamp > timestamp_)
    {
        timestamp_ = message_buf_.stamp.timestamp;
    }

    const int sender_id = message_buf_.stamp.machine;
    const AbsoluteTimestamp last_counter
        = message_buf_.ready_to_deliver;
    const int index = message_buf_.index;

    last_acked_[sender_id] = std::max(last_acked_[sender_id]
        , last_counter);
    set_min_acked(message_buf_.last_delivered);
    last_acked_all_ = *std::min_element(last_acked_.begin()
        , last_acked_.end());
    // last_acked_all_ = std::max(last_acked_all_, message_buf_.last_delivered);

    if (can_deliver_messages())
    {
        deliver_messages();
        if (last_sent_ - last_delivered_[id_] < WINDOW_SIZE)
        {
            send_new_packets();
        }
    }
    
    if (message_buf_.index > last_delivered_[sender_id])
    {
        packets_[sender_id][index % WINDOW_SIZE] = message_buf_;
    }

    // update last received value for this sender
    last_rec_[sender_id] = 
        std::max(last_rec_[sender_id], message_buf_.index); 

    // Check if we have larger continuous window
    if (index == last_rec_cont_[sender_id] + 1)
    {
        update_window_counters(sender_id);
    }
    update_last_acked();
}

void Machine::set_min_acked(const AbsoluteTimestamp& stamp)
{
    for (int i = 0; i < n_machines_; i++)
    {
        last_acked_[i] = std::max(stamp, last_acked_[i]);
    }
}

bool Machine::can_deliver_messages()
{
    for (int i = 0; i < n_machines_; i++)
    {
        if (last_delivered_[i] >= last_rec_cont_[i])
        {
            return false;
        }
    }
    return true;
}

void Machine::update_window_counters(int sender)
{
    int index = last_rec_cont_[sender];
    while (packets_[sender][(index + 1) % WINDOW_SIZE].index == index + 1)
    {
        index++;
    }
    last_rec_cont_[sender] = index;
}

void Machine::deliver_messages()
{
    if (all_empty()) exit(0);
    int deliver_index;
    Message* next_to_deliver;
    while (1)
    {
        deliver_index = find_next_to_deliver();
        next_to_deliver = &packets_[deliver_index]
            [(last_delivered_[deliver_index] + 1) % WINDOW_SIZE];
        // check if there are no more packets within this timestamp
        if (next_to_deliver->stamp > last_acked_all_) break;
        
        last_delivered_[deliver_index]++;
        if (next_to_deliver->type == MessageType::EMPTY)
        {
            done_sending_[deliver_index] = true;
        } else {
            deliver_packet(*next_to_deliver);
        } 
        if (last_delivered_[deliver_index] == last_rec_cont_[deliver_index]) 
        {
            break;
        }
    }
}

uint32_t Machine::generate_magic_number()
{
    return rng_dst_(generator_);
}

int Machine::find_next_to_deliver() {
    int min_machine = 0;
    for (int i = 0; i < n_machines_; i++) {
        if (packets_[i][(last_delivered_[i] + 1) % WINDOW_SIZE].stamp < 
            packets_[min_machine][(last_delivered_[min_machine] + 1) % WINDOW_SIZE].stamp) {
            min_machine = i;
        }
    }
    return min_machine;
}

void Machine::deliver_packet(Message& msg)
{
    out_file_ << "Machine: "
    << msg.stamp.machine
    << ", Index: "
    << msg.index
    << ", Magic Number: "
    << msg.magic_number
    << ", stamp: "
    << msg.stamp.timestamp
    << "\n";
}