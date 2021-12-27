#pragma once

//  This component represents an edge connecting an interface to its network.
//
struct InterfaceEdgeComponent {
    int entity_id;     // This entity ID.
    int other_entity_id;
    float glow;

    InterfaceEdgeComponent(int entity_id, int other_entity_id)
        : entity_id(entity_id), other_entity_id(other_entity_id), glow(0.f)
    {};
};
