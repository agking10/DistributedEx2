#pragma once

extern "C"
{
    #include "recv_dbg.h"
}
#include "net_include.h"
#include "messages.h"

#include <iostream>
#include <vector>

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

    int rec_socket_;
    int send_socket_;
    fd_set mask_;
    fd_set read_mask_;
    fd_set write_mask_;
    fd_set excep_mask_;
    timeval timeout_;

    // Protocol variables
    int timestamp = 0;
    int n_packets_to_send;
    int id;
    int n_machines;
    int last_sent = 0;
    int last_acked_all = 0;
    bool finished_sending = false;
    bool safe_to_leave = false;

    std::vector<std::vector<Message>> packets;
    std::vector<int> last_acked;
    std::vector<int> last_rec_cont;
    std::vector<int> last_rec;
    std::vector<int> last_delivered;
    std::vector<bool> done_sending;

    Message message_buf_;
};