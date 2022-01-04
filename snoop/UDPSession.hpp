#pragma once

#include "util.hpp"
#include "Disposition.hpp"

class Protocol;
class Snoop;


struct IPV4UDPKey : public IPV4SessionKey {
    explicit IPV4UDPKey(const IPV4SockAddress& a, const IPV4SockAddress& b)
        : IPV4SessionKey(a, b)
    {}
};


class IPV4UDPSession {
public:
    IPV4UDPSession(const IPV4UDPKey& key);
    Disposition put(Snoop&, int dir, const unsigned char* payload, int length);

private:
    IPV4UDPKey key;
    Protocol* protocol; //  TODO: use unique_ptr<>
};
