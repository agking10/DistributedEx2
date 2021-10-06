extern "C"
{
    #include "recv_dbg.h"
}
#include "net_include.h"

#include <iostream>

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

    int n_packets;
    int id;
    int n_machines;
    int rec_socket_;
    int send_socket_;
    fd_set mask_;
    fd_set read_mask_;
    fd_set write_mask_;
    fd_set excep_mask_;
    timeval timeout_;
};