#include "messages.h"

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

bool operator<=(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2)
{
    if (t1.timestamp < t2.timestamp) 
    {
        return true;
    }
    else if (t1.timestamp == t2.timestamp 
        && t1.machine <= t2.machine) return true;
    return false;
}

bool operator>=(const AbsoluteTimestamp& t1
        , const AbsoluteTimestamp& t2)
{
    if (t1.timestamp > t2.timestamp)
    {
        return true;
    }
    else if (t1.timestamp == t2.timestamp
        && t1.machine >= t2.machine) return true;
    return false;
}