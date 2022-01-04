#pragma once

#include "Disposition.hpp"

class Snoop;

class Protocol
{
public:
    virtual Disposition put(Snoop&, int dir, const unsigned char* payload, int length) = 0;
    virtual ~Protocol() {};
};
