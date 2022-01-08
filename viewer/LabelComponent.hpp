#pragma once

#include <string>

//  Provides a short description of an entity.
//
struct LabelComponent {
    int entity_id;
    std::string label;

    LabelComponent(int id, const std::string& label) : entity_id(id), label(label) {}
};
