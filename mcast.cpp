#include <iostream>

#include "machine.h"


int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        std::cout << "Usage: <num packets> <machine index> <num machines> <loss rate>"
        << std::endl;
    }

    int n_packets = std::stoi(argv[1]);
    int machine_index = std::stoi(argv[2]);
    int n_machines = std::stoi(argv[3]);
    int loss_rate = std::stoi(argv[4]);

    Machine m(n_packets
        , machine_index
        , n_machines
        , loss_rate
    );

    m.start();

    return 0;
}