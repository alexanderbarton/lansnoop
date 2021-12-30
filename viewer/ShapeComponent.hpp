#pragma once

#include <iostream>


//  Represents a simple shape that can be drawn at a location.
//
struct ShapeComponent {
    int entity_id;
    enum class Shape {
        BOX,
        CYLINDER,
    } shape;

    ShapeComponent(int id, Shape shape) : entity_id(id), shape(shape) {}
};


std::ostream& operator<<(std::ostream&, ShapeComponent::Shape);
