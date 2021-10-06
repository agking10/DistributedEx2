#include <iostream>
#include "messages.h"
#include "net_include.h"

int main(int argc, char* argv[])
{
    sockaddr_in name;
    sockaddr_in send_addr;
    ip_mreq     mreq;
    unsigned char      ttl_val;
    int ss;

    mreq.imr_multiaddr.s_addr = htonl( MCAST_ADDR );
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    ss = socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
    if (ss<0) {
        perror("Mcast: socket");
        exit(1);
    }

    ttl_val = 1;
    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val, 
        sizeof(ttl_val)) < 0) 
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d \n", ttl_val );
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(MCAST_ADDR);  /* mcast address */
    send_addr.sin_port = htons(PORT);

    Message start;
    start.type = MessageType::START;

    sendto( ss, reinterpret_cast<char*>(&start), sizeof(Message)
        , 0, (sockaddr*)&send_addr, sizeof(send_addr));

    return 0;
}