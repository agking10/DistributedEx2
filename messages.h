#pragma once

#include "net_include.h"

#define MAX_NACKS 9

enum class MessageType : char
{
    START = 's',
    DATA = 'd',
    EMPTY = 'e',
};

struct AbsoluteTimestamp
{
    int timestamp;
    int machine;

    AbsoluteTimestamp()
    : timestamp(0)
    , machine(0)
    {}
    
    friend bool operator<(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2);
    friend bool operator>(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2);
};

bool operator<(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2)
{
    if (t1.timestamp < t2.timestamp) 
    {
        return true;
    }
    else if (t1.timestamp == t2.timestamp 
        && t1.machine < t2.machine) return true;
    return false;
}

bool operator>(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2)
{
    if (t1.timestamp > t2.timestamp)
    {
        return true;
    }
    else if (t1.timestamp == t2.timestamp
        && t1.machine > t2.machine) return true;
    return false;
}

struct RetransmitRequest
{
    int packet_no;
    int machine;
};

struct Message
{
    MessageType type;
    int pid;
    int timestamp;
    AbsoluteTimestamp ready_to_deliver;
    int n_retrans;
    RetransmitRequest retrans_req[MAX_NACKS];
    int index;
    char data[MAX_MESS_LEN];
};

