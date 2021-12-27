#pragma once

#include <vector>
#include "DescriptionComponent.hpp"
#include "LocationComponent.hpp"
#include "FDGVertexComponent.hpp"
#include "FDGEdgeComponent.hpp"
#include "InterfaceEdgeComponent.hpp"


//  Components are stored in vectors.
//  A component entry with ID==0 is an empty position.
//  Otherwise, nonempty components are ordered in the vector ascending by ID.
//  To add a component, push it onto the end of the array.
//  To remove a component, set its ID to 0.
//  TODO: *Something* will come along and remove the empty entries.
//  TODO: This scheme works only when a component is inserted at creation time.
//      Inserting after creation time makes it possible that its ID is less than
//      the current end of the vector.

struct Components {
    std::vector<DescriptionComponent> description_components;
    std::vector<LocationComponent> location_components;
    std::vector<FDGVertexComponent> fdg_vertex_components;
    std::vector<FDGEdgeComponent> fdg_edge_components;
    std::vector<InterfaceEdgeComponent> interface_edge_components;


    //  Write a description of all entities to stdout.
    //
    void describe_entities() const;
};


InterfaceEdgeComponent& find(long entity_id, std::vector<InterfaceEdgeComponent>& interface_edge_components);
