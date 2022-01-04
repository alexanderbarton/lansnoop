#include "UDPSession.hpp"
#include "Protocol.hpp"
#include "ProtocolDNS.hpp"
#include "ProtocolDiscard.hpp"


IPV4UDPSession::IPV4UDPSession(const IPV4UDPKey& key)
    : key(key)
{
    //  Just assume anything over UDP port 53 is DNS.
    if (key.a.port == 53 || key.b.port == 53)
        this->protocol = new ProtocolDNS();
    else
        this->protocol = new ProtocolDiscard();
}


Disposition IPV4UDPSession::put(Snoop& snoop, int dir, const unsigned char* payload, int length)
{
    return this->protocol->put(snoop, dir, payload, length);
}
