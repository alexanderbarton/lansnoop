#pragma once

#include "Protocol.hpp"

class Snoop;

class ProtocolDiscard : public Protocol
{
public:
    Disposition put(Snoop&, int dir, const unsigned char* payload, int length) override
        { return Disposition::L4_PROTOCOL; };

    ~ProtocolDiscard() override {};
};
