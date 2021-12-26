#pragma once

//  Represents a position in world coordinates.
//
struct LocationComponent {
    int entity_id;
    float x, y, z;

    LocationComponent(int id, float x, float y, float z) : entity_id(id), x(x), y(y), z(z) {}
};
