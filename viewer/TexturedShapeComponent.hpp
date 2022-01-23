#pragma once

#include <iostream>
#include <glm/glm.hpp>


//  Represents a textured box that can be drawn at a location.
//
struct TexturedShapeComponent {
    int entity_id;
    unsigned int texture;

    TexturedShapeComponent(int entity_id, unsigned int texture) : entity_id(entity_id), texture(texture) {}
};
