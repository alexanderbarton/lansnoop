#pragma once

#include <GLFW/glfw3.h>

#include "Components.hpp"

class DisplaySystem;

class MouseSystem {
public:
    MouseSystem();
    void init(GLFWwindow* window);
    void update(Components& components, DisplaySystem& display);

    //  Returns the entity ID of the hoverable object under the mouse,
    //  or 0 if no hoverable object is under the mouse.
    //
    int get_hover_id() const { return hoverId; }

    void scroll_callback(double xoffset, double yoffset);
    void mouse_button_callback(int button, int action, int mods);

private:
    GLFWwindow* window;
    float displacement_y = 0.f;
    int hoverId = 0;
    int dragId = 0;
};
