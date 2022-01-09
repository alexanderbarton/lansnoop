#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "util.hpp"


class IPV4PrefixTable {
public:
    struct Prefix {
        uint32_t address; // Addresses are all in host byte order.
        uint32_t netmask;
        uint32_t asn;

        bool operator <(const Prefix& rhs) {
            if (address == rhs.address)
                return netmask < rhs.netmask;
            return address < rhs.address;
        }
    };

    void load(const std::string& path, bool verbose = false);

    //  Returns the prefix and ASN of a given IP address, or nullptr if not found.
    //
    const Prefix* look_up(uint32_t address) const;
    const Prefix* look_up(const IPV4Address&) const;

private:
    std::vector<Prefix> prefixes;
};
