#pragma once

//  This component represents an edge participating in a force directed graph.
//
//  An edge exerts an attractive force on another entity.
//  Both entitues should have the Location component.
//
struct FDGEdgeComponent {
    int entity_id;     // This entity ID.
    int other_entity_id;
    float length; // The length the edge wants to be when fully relaxed.

    FDGEdgeComponent(int entity_id, int other_entity_id, float length = 0.f)
        : entity_id(entity_id), other_entity_id(other_entity_id), length(length)
    {};
};
