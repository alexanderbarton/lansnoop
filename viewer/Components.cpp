#include <iostream>
#include <set>
#include "Components.hpp"


static void describe(const DescriptionComponent& c)
{
    std::cout << "    description: " << c.description << "\n";
}


static void describe(const LocationComponent& c)
{
    std::cout << "    location: (" << c.x << ", " << c.y << ", " << c.z << ")" << "\n";
}


static void describe(const FDGVertexComponent& c)
{
    std::cout << "    fdg_vertex: velocity: (" << c.vx << ", " << c.vy << ")" << "\n";
}


static void describe(const FDGEdgeComponent& c)
{
    std::cout << "    fdg_edge: vertex_ids: { " << c.vertex_ids[0] << ", " << c.vertex_ids[1] << " }" << "\n";
}


template <class T>
static void describe(int entity_id, const std::vector<T>& cs)
{
    for (const auto& c : cs)
        if (c.entity_id == entity_id) {
            describe(c);
            break;
        }
}


void Components::describe_entities() const
{
    //  Make a list of all entity IDs currently in use.
    std::set<int> entity_ids;
    for (const auto& c : this->description_components)
        entity_ids.insert(c.entity_id);
    for (const auto& c : this->location_components)
        entity_ids.insert(c.entity_id);
    for (const auto& c : this->fdg_vertex_components)
        entity_ids.insert(c.entity_id);
    for (const auto& c : this->fdg_edge_components)
        entity_ids.insert(c.entity_id);

    for (int entity_id : entity_ids) {
        std::cout << "Entity " << entity_id << "\n";
        describe(entity_id, this->description_components);
        describe(entity_id, this->location_components);
        describe(entity_id, this->fdg_vertex_components);
        describe(entity_id, this->fdg_edge_components);
    }
    std::cout << std::flush;
}
