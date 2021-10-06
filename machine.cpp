#include "machine.h"

Machine::Machine(int n_packets
        , int machine_index
        , int n_machines
        , int loss_rate) 
        : n_packets(n_packets)
        , id(machine_index)
        , n_machines(n_machines)
    {
        recv_dbg_init(loss_rate, machine_index);
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
    //TODO
}

void Machine::start_protocol()
{
    //TODO
}