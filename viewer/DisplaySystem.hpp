#pragma once

#include <string>
#include <glm/glm.hpp>

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
    // glm::vec3 lookAt;
    // glm::vec3 lookFrom; // Eye position relative to lookAt.

    int frames = 0;

    unsigned int modelLoc;
    unsigned int viewLoc;
    unsigned int projectionLoc;
    // unsigned int VBO, VAO, EBO;
    unsigned int cubeVBO, cubeVAO;

    unsigned int lineVAO, lineVBO;
    float line_vertices[6];

    unsigned int objectShader;
    unsigned int lineShader;

    void init_object_shaders();
    void init_line_shaders();
    void drawLine(float ax, float ay, float az, float bx, float by, float bz);
};
