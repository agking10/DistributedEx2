#pragma once

#include "net_include.h"

#define MAX_NACKS 9

enum class MessageType : char
{
    START = 's',
    DATA = 'd',
    EMPTY = 'e',
};

// Struct used for providing absolute ordering to timestamps
struct AbsoluteTimestamp
{
    int timestamp;
    int machine;

    AbsoluteTimestamp()
    : timestamp(0)
    , machine(0)
    {}

    AbsoluteTimestamp(int timestamp, int machine_index)
    : timestamp(timestamp)
    , machine(machine_index)
    {}
    
    friend bool operator<(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2);
    friend bool operator>(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2);
    friend bool operator<=(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2);
    friend bool operator>=(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2);
};

struct RetransmitRequest
{
    int packet_no;
    int machine;
};

struct Message
{
    MessageType type;
    AbsoluteTimestamp stamp;
    AbsoluteTimestamp ready_to_deliver;
    int n_retrans;
    RetransmitRequest retrans_req[MAX_NACKS];
    int index;
    int magic_number; // random number to verify packets are the same
    char data[MAX_MESS_LEN];
};

