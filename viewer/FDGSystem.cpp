#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "FDGSystem.hpp"


static LocationComponent& find_location(Components& components, int entity_id, int& index)
{
    while (size_t(index) < components.location_components.size() && entity_id > components.location_components[index].entity_id)
        ++index;
    if (size_t(index) < components.location_components.size() && entity_id == components.location_components[index].entity_id)
        return components.location_components[index];
    throw std::runtime_error(std::string("FDGSystem::find_location(): entity ID ") + std::to_string(entity_id) + " not found in location components table");
}


void FDGSystem::update(Components& components)
{
    const float dt = 1.0f/60; //  Assume dt is 1/60 seconds.

    struct Node {
        int entity_id;
        float px;
        float py;
        float vx;
        float vy;
        float fx = 0.0f;
        float fy = 0.0f;
    };

    //  Load vertex location and other data into a temporary workspace.
    //
    std::vector<Node> nodes;
    nodes.reserve(components.fdg_vertex_components.size());
    int lc_index = 0;
    for (const FDGVertexComponent& v : components.fdg_vertex_components) {
        const LocationComponent& lc = find_location(components, v.entity_id, lc_index);
        Node node;
        node.entity_id = v.entity_id;
        node.px = lc.x;
        node.py = lc.y;
        node.vx = v.vx;
        node.vy = v.vy;
        nodes.push_back(node);
    }

#if 1
    //  Compute intervertex repulsion forces.
    //  Repulsion is inversely porportional to distance squared.
    //
    const float kr = 3.0f;
    for (Node& a : nodes)
        for (Node& b : nodes)
            if (a.entity_id != b.entity_id) {
                float d_square = (a.px-b.px)*(a.px-b.px)+(a.py-b.py)*(a.py-b.py);
                d_square = std::max(d_square, 0.125f);
                float d = sqrt(d_square);
                a.fx -= kr / d_square * (b.px-a.px) / d;
                a.fy -= kr / d_square * (b.py-a.py) / d;
            }
#endif

#if 1
    //  Compute intervertex attraction forces.
    //  Attration is proportional to distance.
    //
    const float ka = 1.f / 16;
    std::unordered_map<int, int> entity_to_node_index;
    entity_to_node_index.reserve(nodes.size());
    for (size_t ix=0; ix<nodes.size(); ++ix)
        entity_to_node_index[nodes[ix].entity_id] = ix;
    for (const FDGEdgeComponent& edge : components.fdg_edge_components) {
        if (!entity_to_node_index.count(edge.entity_id))
            throw std::runtime_error(std::string("FDGSystem::update(): entity ID ") + std::to_string(edge.entity_id) + " not found in entity_to_node_index");
        if (!entity_to_node_index.count(edge.other_entity_id))
            throw std::runtime_error(std::string("FDGSystem::update(): entity ID ") + std::to_string(edge.other_entity_id) + " not found in entity_to_node_index");
        Node& a = nodes[entity_to_node_index[edge.entity_id]];
        Node& b = nodes[entity_to_node_index[edge.other_entity_id]];
        // float d = sqrt((a.px-b.px)*(a.px-b.px)+(a.py-b.py)*(a.py-b.py));
        a.fx += ka * (b.px-a.px);
        a.fy += ka * (b.py-a.py);
        b.fx += ka * (a.px-b.px);
        b.fy += ka * (a.py-b.py);
    }
#endif

#if 1
    //  Compute attraction-to-origin force.
    //  Attration is proportional to distance.
    //
    const float kc = 0.005f;
    for (Node& node : nodes) {
        float d_square = node.px*node.px + node.py*node.py;
        // d_square = std::max(d_square, 1.0f);
        float d = sqrt(d_square);
        node.fx -= kc * d * node.px;
        node.fy -= kc * d * node.py;
    }
#endif

#if 0
    //  Compute a drag force acting against velocity.
    //
    const float kd = 50.0f; //  Drag constant.
    for (Node& node : nodes) {
        float speed = sqrt(node.vx*node.vx + node.vy*node.vy);
        if (speed > 1.0f) {
            node.fx -= node.vx / speed * kd;
            node.fy -= node.vy / speed * kd;
        }
    }
#endif

#if 1
    //  
    //
    for (Node& node : nodes) {
        node.vx *= 0.99f;
        node.vy *= 0.99f;
    }
#endif

    //  Apply forces to vertices and update positions.
    //
    const float ki = 1.0; // Node inertia.
    for (Node& node : nodes) {
        float new_vx = node.vx + dt * node.fx / ki;
        float new_vy = node.vy + dt * node.fy / ki;
        node.px += (new_vx + node.vx) / 2.0f * dt;
        node.py += (new_vy + node.vy) / 2.0f * dt;
        node.vx = new_vx;
        node.vy = new_vy;
    }

    //  Update the location component and the FDG vertex component.
    //
    for (size_t i=0; i<nodes.size(); ++i) {
        components.fdg_vertex_components[i].vx = nodes[i].vx;
        components.fdg_vertex_components[i].vy = nodes[i].vy;
    }
    lc_index = 0;
    for (Node& node : nodes) {
        LocationComponent& lc = find_location(components, node.entity_id, lc_index);
        lc.x = node.px;
        lc.y = node.py;
    }
}
