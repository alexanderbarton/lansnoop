#pragma once

#include <GLFW/glfw3.h>

#include "Components.hpp"

class FDGSystem;


class KeyboardSystem {
public:
    void init(GLFWwindow* window);
    void update(Components& components, FDGSystem& fdg);

    void character_callback(unsigned int codepoint);

private:
    enum class Parameter {
        FDG_REPULSION,
        FDG_ATTRACTION,
        FDG_ORIGIN,
        FDG_DRAG,
        FDG_INERTIA,
        NONE, // Must be last.
    } current_parameter = Parameter::NONE;

    GLFWwindow* window;

    std::vector<unsigned int> pending_chars;

    static const char* to_s(Parameter);
};
