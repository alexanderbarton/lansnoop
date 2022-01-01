#include <cmath>

#include "MouseSystem.hpp"
#include "DisplaySystem.hpp"

#include "/home/abarton/debug.hpp"


static MouseSystem* msp;

extern "C" {
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        msp->scroll_callback(xoffset, yoffset);
    }
}


MouseSystem::MouseSystem()
{
    msp = this;
}


void MouseSystem::init(GLFWwindow* window)
{
    this->window = window;
    glfwSetScrollCallback(window, ::scroll_callback);
}


void MouseSystem::update(Components& components, DisplaySystem& display)
{
    // Six mouse wheel clicks doubles (or halves) our viewing distance.
    if (this->displacement_y > 0.5f || this->displacement_y < -0.5f) {
        constexpr float sixth = 1.122462f;  //  Sixth root of two.
        display.set_camera(display.get_camera_focus(), std::pow(sixth, displacement_y) * display.get_camera_distance());
        this->displacement_y = 0.f;
    }
}


void MouseSystem::scroll_callback(double xoffset, double yoffset)
{
    this->displacement_y += -yoffset;
}
