#include "ShapeComponent.hpp"


std::ostream& operator<<(std::ostream& o, ShapeComponent::Shape s)
{
    switch (s) {
        case ShapeComponent::Shape::BOX:
            return o << "BOX";
        case ShapeComponent::Shape::CYLINDER:
            return o << "CYLINDER";
    }
    return o;
}
