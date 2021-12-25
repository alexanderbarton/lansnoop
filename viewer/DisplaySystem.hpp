#pragma once

#include <string>

#include "System.hpp"

struct GLFWwindow;


//  The DisplaySystem renders entities via OpenGL.
//
class DisplaySystem : public System {
public:
    ~DisplaySystem();

    void init();
    void update(Components& components);
    bool should_close();

private:
    std::string name = "Lansnoop Viewer";
    int window_width = 800, window_height = 600;
    GLFWwindow* window;
    unsigned int VBO, VAO, EBO;

    void init_shaders();
};
