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

    //  Maps snooper IDs to entity IDs.
    std::unordered_map<int, int> network_to_entity_ids;
    std::unordered_map<int, int> interface_to_entity_ids;

    //  Map snooper interface ID's to packet counts.
    std::unordered_map<int, long> interface_packet_counts;
};
