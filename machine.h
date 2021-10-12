#pragma once

extern "C"
{
    #include "recv_dbg.h"
}
#include "net_include.h"
#include "messages.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#define MACHINE_TIMEOUT 300000
#define TIMEOUT 300
#define MAX_RETRANSMIT 15

class Machine
{
    using RNG = std::mt19937;

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
    void update_window_counters(int sender);
    void deliver_messages();
    void send_empty();
    void send_packet(Message& msg);
    void send_packet(int index);
    void write_packet(int index, bool is_empty = false);
    int  find_next_to_deliver();
    void deliver_packet(Message&);

    uint32_t generate_magic_number();

    int rec_socket_;
    int send_socket_;
    sockaddr_in send_addr_;
    fd_set mask_;
    fd_set read_mask_;
    fd_set write_mask_;
    fd_set excep_mask_;
    timeval timeout_;

    // Protocol variables
    int timeout_counter_ = 0;
    int timestamp_ = 0;
    int n_packets_to_send_;
    int id_;
    int n_machines_;
    int last_sent_ = 0;
    AbsoluteTimestamp last_acked_all_;
    AbsoluteTimestamp delivered_;
    bool ready_to_deliver_;
    bool finished_sending_ = false;
    bool safe_to_leave_ = false;

    std::vector<std::vector<Message>> packets_;
    std::vector<AbsoluteTimestamp> last_acked_;
    std::vector<int> last_rec_cont_;
    std::vector<int> last_rec_;
    std::vector<int> last_delivered_;
    std::vector<bool> done_sending_;

    Message message_buf_;
    RNG generator_;
    std::uniform_int_distribution<uint32_t> rng_dst_;
};