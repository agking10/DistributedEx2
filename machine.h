extern "C"
{
    #include "recv_dbg.h"
}

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
    int n_packets;
    int id;
    int n_machines;
    int loss_rate;
};