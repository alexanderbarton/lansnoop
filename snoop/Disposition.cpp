#include "Disposition.hpp"

#include <iostream>

std::ostream& operator<<(std::ostream& o, Disposition d)
{
    switch (d) {
        case Disposition::TRUNCATED:       return o << "TRUNCATED";
        case Disposition::ERROR:           return o << "ERROR";
        case Disposition::DISINTEREST:     return o << "DISINTEREST";
        case Disposition::ETHERTYPE_BAD:   return o << "ETHERTYPE_BAD";
        case Disposition::ARP:             return o << "ARP";
        case Disposition::ARP_DISINTEREST: return o << "ARP_DISINTEREST";
        case Disposition::ARP_ERROR:       return o << "ARP_ERROR";
        case Disposition::IPv4_FRAGMENT:   return o << "IPv4_FRAGMENT";
        case Disposition::IPv4_BAD:        return o << "IPv4_BAD";
        case Disposition::IPv4_PROTOCOL:   return o << "IPv4_PROTOCOL";
        case Disposition::L4_PROTOCOL:     return o << "L4_PROTOCOL";
        case Disposition::UDP:             return o << "UDP";
        case Disposition::DNS:             return o << "DNS";
        case Disposition::DNS_ERROR:       return o << "DNS_ERROR";
        case Disposition::_MAX:            return o << "_MAX";
        default: return o << "(invalid)";
    };
}
