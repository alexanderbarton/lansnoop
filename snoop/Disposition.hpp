#pragma once

#include <iosfwd>

enum class Disposition {
    TRUNCATED,
    ERROR,
    DISINTEREST,
    ETHERTYPE_BAD,
    ARP,
    ARP_DISINTEREST,
    ARP_ERROR,
    IPv4_FRAGMENT, // Discarded because we don't handle fragments yet.
    IPv4_BAD,
    IPv4_PROTOCOL,
    L4_PROTOCOL,
    UDP,
    DNS,
    DNS_ERROR,
    _MAX,
};

std::ostream& operator<<(std::ostream&, Disposition);
