#pragma once

//  This component represents an edge participating in a force directed graph.
//
//  An edge exerts an attractive force between a pair FDGVertexComponents.
//
struct FDGEdgeComponent {
    int entity_id;     // This entity ID.
    int vertex_ids[2]; // IDs of a pair of FDGVertexComponents.

    FDGEdgeComponent(int id, int node_a_id, int node_b_id) : entity_id(id), vertex_ids { node_a_id, node_b_id } {};
};
