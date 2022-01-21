#pragma once

#include <string>

//  Provides a short description of an entity.
//
struct LabelComponent {
    int entity_id;
    std::vector<std::string> labels;
    float fade = 2.f;

    LabelComponent(int id) : entity_id(id) {}

    LabelComponent(int id, const std::vector<std::string>& labels) : entity_id(id), labels(labels) {}

    LabelComponent(int id, const std::string& label) : entity_id(id)
    {
        labels.push_back(label);
    }
};
