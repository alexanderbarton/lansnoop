#pragma once

#include <iostream>
#include <glm/glm.hpp>


//  Represents a simple shape that can be drawn at a location.
//
struct ShapeComponent {
    int entity_id;
    enum class Shape {
        BOX,
        CYLINDER,
    } shape;
    glm::vec3 color;

    ShapeComponent(int id, Shape shape, const glm::vec3& color) : entity_id(id), shape(shape), color(color) {}
};


std::ostream& operator<<(std::ostream&, ShapeComponent::Shape);
