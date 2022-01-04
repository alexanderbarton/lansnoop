#pragma once

#include "Protocol.hpp"

class ProtocolDNS : public Protocol
{
public:
    Disposition put(Snoop&, int dir, const unsigned char* payload, int length) override;
    ~ProtocolDNS() override {};
};
