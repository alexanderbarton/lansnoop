#pragma once

#include "System.hpp"


//  The NetworkModelSystem reads snoop events from a pipe,
//  and maps network model objects to entities.
//
class FDGSystem : public System {
public:
    void update(Components& components);

private:
};
