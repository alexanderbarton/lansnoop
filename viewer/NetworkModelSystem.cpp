#include <cstdlib>
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


void NetworkModelSystem::update(Components& components)
{
    Lansnoop::Event event;
    // while (in >> event) {
    while (read_event_nb(in, event)) {
        switch (event.type_case()) {

            case Lansnoop::Event::kNetwork:
                if (!network_to_entity_ids.count(event.network().id())) {
                    int entity_id = generate_entity_id();
                    components.description_components.push_back(DescriptionComponent(entity_id, "network"));
                    components.location_components.push_back(LocationComponent(entity_id, 16*rng(), 16*rng(), 1.0f));
                    components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
                    this->network_to_entity_ids[event.network().id()] = entity_id;
                }
                break;

            case Lansnoop::Event::kInterface:
                if (!interface_to_entity_ids.count(event.interface().id())) {
                    int entity_id = generate_entity_id();
                    components.description_components.push_back(DescriptionComponent(entity_id, "interface"));
                    components.location_components.push_back(LocationComponent(entity_id, 16*rng(), 16*rng(), 1.0f));
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
                    // show(dp);
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
