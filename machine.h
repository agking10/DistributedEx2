#pragma once

extern "C"
{
    #include "recv_dbg.h"
}
#include "net_include.h"
#include "messages.h"

#include <algorithm>
#include <iostream>
#include <vector>

#define MACHINE_TIMEOUT 3000

class Machine
{
public:
    Machine(int n_packets
        , int machine_index
        , int n_machines
        , int loss_rate
    );

    void start();

private:
    void wait_for_start_signal();
    void start_protocol();
    void handle_packet_in();
    void send_new_packets();
    void send_undelivered_packets();
    bool all_empty();

    int rec_socket_;
    int send_socket_;
    sockaddr_in send_addr;
    fd_set mask_;
    fd_set read_mask_;
    fd_set write_mask_;
    fd_set excep_mask_;
    timeval timeout_;

    // Protocol variables
    int timestamp_ = 0;
    int n_packets_to_send_;
    int id_;
    int n_machines_;
    int last_sent_ = 0;
    AbsoluteTimestamp last_acked_all_;
    bool finished_sending_ = false;
    bool safe_to_leave_ = false;

    std::vector<std::vector<Message>> packets_;
    std::vector<AbsoluteTimestamp> last_acked_;
    std::vector<int> last_rec_cont_;
    std::vector<int> last_rec_;
    std::vector<int> last_delivered_;
    std::vector<bool> done_sending_;

    Message message_buf_;
};