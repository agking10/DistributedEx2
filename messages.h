#pragma once

#include "net_include.h"

#define MAX_NACKS 10

enum class MessageType : char
{
    START = 's',
    DATA = 'd',
    EMPTY = 'e',
};

struct Timestamp
{
    int ready_timestamp;
    int ready_machine;
};

struct RetransmitRequest
{
    int packet_no;
    int machine;
};

struct Message
{
    MessageType type;
    int pid;
    Timestamp ready_to_deliver;
    int n_retrans;
    RetransmitRequest retrans_req[MAX_NACKS];
    char data[MAX_MESS_LEN];
};

