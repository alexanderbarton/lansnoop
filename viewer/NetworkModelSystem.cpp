#include <cstdlib>

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
    while (in >> event) {
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
                        int edge_entity_id = generate_entity_id();
                        int network_entity_id = network_to_entity_ids[event.interface().network_id()];
                        components.description_components.push_back(DescriptionComponent(edge_entity_id, "edge"));
                        components.fdg_edge_components.push_back(FDGEdgeComponent(edge_entity_id, entity_id, network_entity_id));
                    }
                }
                else {
                    //  Check for changes to the assignment of this interface to a different network.
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
}
