#pragma once

//  This component represents a vertex participating in a force directed graph.
//  All vertices repel each other.
//
struct FDGVertexComponent {
    int entity_id;
    float vx, vy; // velocity

    FDGVertexComponent(int id) : vx(0), vy(0) {};
};
