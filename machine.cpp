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
    std::cout << "I started!" << std::endl;
}