#pragma once

#include <fstream>
#include <unordered_map>

#include "System.hpp"


//  The NetworkModelSystem reads snoop events from a pipe,
//  and maps network model objects to entities.
//
class NetworkModelSystem : public System {
public:
    void update(Components& components);

    void open(const std::string& path);

private:
    std::ifstream in;

    //  Maps snooper network IDs to entity IDs.
    std::unordered_map<int, int> net_to_entity_ids;
};
