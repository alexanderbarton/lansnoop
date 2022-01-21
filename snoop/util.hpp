#pragma once

#include <array>
#include <iosfwd>

class MacAddress : public std::array<unsigned char, 6> {};  // Network byte order.
class IPV4Address : public std::array<unsigned char, 4> {};  // Network byte order.

struct IPV4SockAddress {
    IPV4Address address;
    unsigned short port;

    bool operator <(const IPV4SockAddress& rhs) const {
        if (address == rhs.address)
            return port < rhs.port;
        return address < rhs.address;
    }

    bool operator ==(const IPV4SockAddress& rhs) const {
        return address == rhs.address && port == rhs.port;
    }

    IPV4SockAddress& operator=(const IPV4SockAddress&) = default;
};

//  Represents a network "four tuple".
//  By convention, a<=b.
//
struct IPV4SessionKey {
    IPV4SockAddress a, b;

    explicit IPV4SessionKey(const IPV4SessionKey&) = default;

    explicit IPV4SessionKey(const IPV4SockAddress& a, const IPV4SockAddress& b) {
        if (a<b) {
            this->a = a;
            this->b = b;
        } else {
            this->a = b;
            this->b = a;
        }
    }

    bool operator <(const IPV4SessionKey& rhs) const {
        if (a == rhs.a)
            return b < rhs.b;
        return a < rhs.a;
    }

    IPV4SessionKey& operator=(const IPV4SessionKey&) = default;
};

std::ostream& operator<<(std::ostream&, const MacAddress&);
std::ostream& operator<<(std::ostream&, const IPV4Address&);
std::ostream& operator<<(std::ostream&, const IPV4SockAddress&);
std::ostream& operator<<(std::ostream&, const IPV4SessionKey&);
