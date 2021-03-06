#pragma once

#include <fstream>
#include <unordered_map>

#include "event.pb.h"
#include "System.hpp"


//  The NetworkModelSystem reads snoop events from a pipe,
//  and maps network model objects to entities.
//
class NetworkModelSystem : public System {
public:
    void init();
    void update(Components& components);

    void open(const std::string& path);

private:
    std::ifstream in;

    //  Maps snooper IDs to entity IDs.
    std::unordered_map<int, int> network_to_entity_ids;
    std::unordered_map<int, int> interface_to_entity_ids;
    std::unordered_map<int, int> ipaddress_to_entity_ids;
    std::unordered_map<int, int> cloud_to_entity_ids;

    //  Map snooper object ID's to packet counts.
    std::unordered_map<int, long> interface_packet_counts;
    std::unordered_map<int, long> cloud_packet_counts;
    std::unordered_map<int, long> ipaddress_packet_counts;

    void receive(Components&, const Lansnoop::Network&);
    void receive(Components&, const Lansnoop::Interface&);
    void receive(Components&, const Lansnoop::Traffic&);
    void receive(Components&, const Lansnoop::IPAddress&);
    void receive(Components&, const Lansnoop::Cloud&);
};
