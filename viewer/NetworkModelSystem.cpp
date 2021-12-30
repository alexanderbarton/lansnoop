#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <string>
#include <fcntl.h>

#include "event.pb.h"
#include "EventSerialization.hpp"
#include "Entities.hpp"

#include "NetworkModelSystem.hpp"

#include "/home/abarton/debug.hpp"


//  Returns a random value between -1.0 and 1.0.
static float rng()
{
    return (rand() - RAND_MAX/2) * 1.0f / (RAND_MAX/2);
}


static std::string bytes_to_mac(const std::string& bytes)
{
    std::stringstream sstream;
    sstream << std::hex << std::setw(2) << std::setfill('0');
    bool first = true;
    for (char c : bytes) {
        if (first)
            first = false;
        else
            sstream << ':';
        sstream << int((unsigned char)c);
    }
    std::string result = sstream.str();
    return result;
}


static std::string bytes_to_ip_address(const std::string& bytes)
{
    std::stringstream sstream;
    bool first = true;
    for (char c : bytes) {
        if (first)
            first = false;
        else
            sstream << '.';
        sstream << int((unsigned char)c);
    }
    std::string result = sstream.str();
    return result;
}


void NetworkModelSystem::update(Components& components)
{
    Lansnoop::Event event;
    // while (in >> event) {
    while (read_event_nb(in, event)) {
        switch (event.type_case()) {

            case Lansnoop::Event::kNetwork:
                if (!network_to_entity_ids.count(event.network().id())) {
                    int entity_id = generate_entity_id();
                    std::string description("network ");
                    description += std::to_string(event.network().id());
                    components.description_components.push_back(DescriptionComponent(entity_id, description));
                    components.location_components.push_back(LocationComponent(entity_id, 16*rng(), 16*rng(), 1.0f));
                    // components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::CYLINDER));
                    components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
                    this->network_to_entity_ids[event.network().id()] = entity_id;
                }
                break;

            case Lansnoop::Event::kInterface:
                if (!interface_to_entity_ids.count(event.interface().id())) {
                    int entity_id = generate_entity_id();
                    std::string description("interface ");
                    description += bytes_to_mac(event.interface().address());
                    components.description_components.push_back(DescriptionComponent(entity_id, description));
                    components.location_components.push_back(LocationComponent(entity_id, 16*rng(), 16*rng(), 1.0f));
                    components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::BOX));
                    components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
                    this->interface_to_entity_ids[event.interface().id()] = entity_id;

                    if (!network_to_entity_ids.count(event.interface().network_id())) {
                        marks("oops"); // We should already have a mapping for this network.
                    } else {
                        int network_entity_id = network_to_entity_ids[event.interface().network_id()];
                        components.fdg_edge_components.push_back(FDGEdgeComponent(entity_id, network_entity_id));
                        components.interface_edge_components.push_back(InterfaceEdgeComponent(entity_id, network_entity_id));
                    }
                }
                else {
                    //  Check for changes to the assignment of this interface to a different network.
                }
                break;

            case Lansnoop::Event::kInterfaceTraffic:
                for (const auto& e : event.interface_traffic().packet_counts()) {
                    long dp = e.second - this->interface_packet_counts[e.first];
                    this->interface_packet_counts[e.first] = e.second;
                    if (dp) {
                        long entity_id = this->interface_to_entity_ids.at(e.first);
                        InterfaceEdgeComponent& iec = find(entity_id, components.interface_edge_components);
                        iec.glow += dp;
                    }
                }
                break;

            case Lansnoop::Event::kIpaddress:
                if (!ipaddress_to_entity_ids.count(event.ipaddress().id())) {
                    int entity_id = generate_entity_id();
                    std::string description("IP address ");
                    description += bytes_to_ip_address(event.ipaddress().address());
                    components.description_components.push_back(DescriptionComponent(entity_id, description));
                    components.location_components.push_back(LocationComponent(entity_id, 16*rng(), 16*rng(), 1.0f));
                    components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::CYLINDER));
                    components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
                    this->ipaddress_to_entity_ids[event.ipaddress().id()] = entity_id;

                    if (event.ipaddress().interface_id()) {
                        if (!interface_to_entity_ids.count(event.ipaddress().interface_id())) {
                            marks("oops"); // We should already have a mapping for this interface.
                        } else {
                            int interface_entity_id = interface_to_entity_ids[event.ipaddress().interface_id()];
                            components.fdg_edge_components.push_back(FDGEdgeComponent(entity_id, interface_entity_id));
                            components.interface_edge_components.push_back(InterfaceEdgeComponent(entity_id, interface_entity_id));
                        }
                    }
                    //  TODO: deal with changing interface_id()'s.
                }
                break;

            case Lansnoop::Event::TYPE_NOT_SET:
            // default:
                break;
        }
    }

    // components.describe_entities();
}


void NetworkModelSystem::open(const std::string& path)
{
    this->in.open(path, std::ifstream::binary);
    if (!in.good())
        throw std::invalid_argument("NetworkModelSystem: unable to open input path");
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
}
