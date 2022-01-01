#pragma once

#include <GLFW/glfw3.h>

#include "Components.hpp"

class DisplaySystem;

class MouseSystem {
public:
    MouseSystem();
    void init(GLFWwindow* window);
    void update(Components& components, DisplaySystem& display);

    void scroll_callback(double xoffset, double yoffset);

private:
    GLFWwindow* window;
    float displacement_y = 0.f;
};
