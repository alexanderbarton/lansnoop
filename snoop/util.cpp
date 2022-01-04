#include <iostream>
#include <iomanip>

#include "util.hpp"


std::ostream& operator<<(std::ostream& o, const MacAddress& address)
{
    bool first = true;
    char prev_fill = o.fill('x');
    for (unsigned char c : address) {
        if (first)
            first = false;
        else
            o << ":";
        o << std::hex << std::setw(2) << std::setfill('0') << int(c) << std::dec;
    }
    o << std::setfill(prev_fill);
    return o;
}


std::ostream& operator<<(std::ostream& o, const IPV4Address& address)
{
    bool first = true;
    for (unsigned char c : address) {
        if (first)
            first = false;
        else
            o << ".";
        o << int(c);
    }
    return o;
}


std::ostream& operator<<(std::ostream& o, const IPV4SockAddress& sa)
{
    o << sa.address << ":" << sa.port;
    return o;
}


std::ostream& operator<<(std::ostream& o, const IPV4SessionKey& s)
{
    o << s.a << " - " << s.b;
    return o;
}
