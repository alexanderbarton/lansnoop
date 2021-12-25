#include "event.pb.h"
#include "EventSerialization.hpp"
#include "Entities.hpp"

#include "NetworkModelSystem.hpp"

#include "/home/abarton/debug.hpp"


void NetworkModelSystem::update(Components& components)
{
    Lansnoop::Event event;
    while (in >> event) {
        switch (event.type_case()) {

            case Lansnoop::Event::kNetwork:
                if (!net_to_entity_ids.count(event.network().id())) {
                    LocationComponent component;
                    component.entity_id = generate_entity_id();
                    component.x = component.entity_id; //  TODO: randomize initial location
                    component.y = 0.;                  //  TODO: randomize initial location
                    component.z = 1.;
                    components.location_components.push_back(component);
                    net_to_entity_ids[event.network().id()] = component.entity_id;
                }
                break;

            case Lansnoop::Event::kInterface:
                if (!net_to_entity_ids.count(event.interface().id())) {
                    LocationComponent component;
                    component.entity_id = generate_entity_id();
                    component.x = component.entity_id; //  TODO: randomize initial location
                    component.y = 0.;                  //  TODO: randomize initial location
                    component.z = 1.;
                    components.location_components.push_back(component);
                    net_to_entity_ids[event.interface().id()] = component.entity_id;
                }
                break;

            case Lansnoop::Event::TYPE_NOT_SET:
            // default:
                break;
        }
    }
}


void NetworkModelSystem::open(const std::string& path)
{
    this->in.open(path, std::ifstream::binary);
    if (!in.good())
        throw std::invalid_argument("NetworkModelSystem: unable to open input path");
}
