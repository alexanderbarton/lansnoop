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
    GLFWwindow* get_window() { return window; };

private:
    std::string name = "Lansnoop Viewer";
    int window_width = 800, window_height = 600;
    GLFWwindow* window;
    glm::vec3 lookAt;
    glm::vec3 lookFrom; // Eye position relative to lookAt.
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 view_inverse;
    glm::mat4 projection_inverse;

    int frames = 0;

    unsigned int objectShaderModelLoc;
    unsigned int objectShaderViewLoc;
    unsigned int objectShaderProjectionLoc;
    unsigned int objectShaderColorLoc;

    unsigned int lineShaderViewLoc;
    unsigned int lineShaderProjectionLoc;
    unsigned int lineShaderLineColorLoc;

    unsigned int cubeVAO;
    unsigned int cubeVAOLength;

    unsigned int cylinderVAO;
    unsigned int cylinderVAOLength;

    unsigned int lineVAO, lineVBO;
    float line_vertices[6];

    unsigned int objectShader;
    unsigned int lineShader;

    void init_object_shaders();
    void init_line_shaders();
    void init_cube_vao();
    void init_cylinder_vao();
    void drawLine(float ax, float ay, float az, float bx, float by, float bz);
};
