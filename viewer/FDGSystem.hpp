#pragma once

#include "System.hpp"


//  The Force Directed Graph system alters locations of FDGVertex nodes
//  to try to make a pleasing arrangement.
//
class FDGSystem : public System {
public:
    float k_repulsion = 3.f;          // Intervertex repulsion.
    float k_link_attraction = 1.f/8;  // Attraction between linked vertices.
    float k_origin = 0.0001f;       // Attraction to origin.
    float k_inverse_drag = 0.99f;     // Drag reciprocal.
    float k_vertex_inertia = 0.25f;     // Vertex inertia.

    void init() {}
    void update(Components& components);

private:
};
