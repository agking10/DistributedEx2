#include "machine.h"

Machine::Machine(int n_packets
        , int machine_index
        , int n_machines
        , int loss_rate) 
        : n_packets_to_send_(n_packets)
        , id_(machine_index)
        , n_machines_(n_machines)
        , rng_dst_(1, 1000000)
    {
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
        //generator_(dev());
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
    while (1)
    {
        bytes = recv_dbg( rec_socket_, reinterpret_cast<char*>(&message_buf_)
            , sizeof(message_buf_), 0);
        if (bytes < sizeof(Message)) continue;
        if (message_buf_.type == MessageType::START) break;
    }
}

void Machine::start_protocol()
{
    int num;
    send_new_packets();
    while (1)
    {
        read_mask_ = mask_;
        timeout_.tv_usec = MACHINE_TIMEOUT;
        num = select( FD_SETSIZE
            , &read_mask_, &write_mask_, &excep_mask_, &timeout_);
        if (FD_ISSET(rec_socket_, &read_mask_))
        {
            handle_packet_in();
        }
        else if (num == 0)
        {
            if (all_empty()) exit(0);
            send_undelivered_packets();
        }
    }
}

// Iterate starting at the last packet we haven't been able to deliver
// to the last packet we want to resend
void Machine::send_undelivered_packets()
{
    if (safe_to_leave_) send_empty();

    for (int i = last_delivered_[id_] + 1; 
        i <= std::min(last_sent_, last_delivered_[id_] + MAX_RETRANSMIT); i++)
    {
        send_packet(i);
    }
}

void Machine::send_new_packets()
{
    if (finished_sending_) return;

    for (int i = last_sent_ + 1; i < last_delivered_[id_] + WINDOW_SIZE; i++)
    {
        write_packet(i);
        send_packet(i);
        ++last_sent_;
        if (i == n_packets_to_send_)
        {
            finished_sending_ = true;
            break;
        }
    }
}

void Machine::write_packet(int index, bool isEmpty )
{
    if (index < n_packets_to_send_) {
    Message& packet = packets_[id_][index % WINDOW_SIZE];
    packet.index = index;
    packet.magic_number = generate_magic_number();
    packet.pid = id_;
    packet.timestamp = ++timestamp_;
    }
}

// Attach cumulative ack to message, send to mcast
void Machine::send_packet(Message& msg)
{
    msg.ready_to_deliver = last_acked_all_;
    sendto(send_socket_, reinterpret_cast<const char *>(&msg)
        , sizeof(Message), 0, (sockaddr*)&send_addr_, sizeof(send_addr_));
}

void Machine::send_packet(int index)
{
    send_packet(packets_[id_][index % WINDOW_SIZE]);
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
    int bytes = recvfrom(rec_socket_
        , reinterpret_cast<char*>(&message_buf_)
        , sizeof(Message), 0
        , (sockaddr*)&from_addr, &from_len);
    if (bytes < sizeof(Message))
    {
        std::cerr << "Short read" << std::endl;
    }

    if (message_buf_.timestamp > timestamp_)
    {
        timestamp_ = message_buf_.timestamp;
    }

    const int sender_id = message_buf_.pid;
    const AbsoluteTimestamp last_counter 
        = message_buf_.ready_to_deliver;
    const int index = message_buf_.index;
    
    packets_[sender_id][index % WINDOW_SIZE] = message_buf_;

    // update last received value for this sender
    last_rec_[sender_id] = 
        std::max(last_rec_[sender_id], message_buf_.index); 

    // Check if we have larger continuous window
    if (index == last_rec_cont_[sender_id] + 1)
    {
        update_window_counters(sender_id);
    }

    // Update cumulative ack for this process
    last_acked_[sender_id] = std::max(last_acked_[sender_id]
        , last_counter);
    AbsoluteTimestamp& min_acked = *std::min_element(last_acked_.begin()
        , last_acked_.end());
    if (min_acked > last_acked_all_)
    {
        last_acked_all_ = min_acked;
        deliver_messages();
        if (!finished_sending_ 
            && last_sent_ - last_delivered_[id_] < WINDOW_SIZE)
        {
            send_new_packets();
        }
        else if (finished_sending_ && safe_to_leave_)
        {
            send_empty();
        }
    }
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
        if (AbsoluteTimestamp(ready_to_deliver->timestamp, deliver_index) > last_acked_all_) break;
        
        if (next_to_deliver->type == MessageType::EMPTY)
        {
            done_sending_[deliver_index] = true;
        } else {
            deliver_packet(packets_[deliver_index][(last_delivered_[deliver_index] + 1) % WINDOW_SIZE]);
            last_delivered_[deliver_index]++;
            if (last_delivered_[deliver_index] == last_rec_cont_[deliver_index]) {
                break;
            }
        }
    }
}

// POTENTIAL BUG HERE: WHAT IS INDEX OF EMPTY???
void Machine::send_empty()
{
    write_packet()
    Message msg;
    msg.type = MessageType::EMPTY;
    msg.timestamp = ++timestamp_;
    msg.index = ++last_sent_;
    send_packet(msg);
}


uint32_t Machine::generate_magic_number()
{
    return rng_dst_(generator_);
}

int Machine::find_next_to_deliver() {
    int min_machine = 0;
    for (int i = 0; i < n_machines_; i++) {
        if (packets_[i][(last_delivered_[i] + 1) % WINDOW_SIZE]->timestamp < 
            packets_[min_machine][(last_delivered_[min_machine] + 1) % WINDOW_SIZE]->timestamp) {
            min_machine = i;
        }
    }
    return min_machine;
}