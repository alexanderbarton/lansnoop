#pragma once

#include <string>

//  Provides a short description of an entity.
//
struct DescriptionComponent {
    int entity_id;
    std::string description;

    DescriptionComponent(int id, const std::string& description) : entity_id(id), description(description) {}
};
