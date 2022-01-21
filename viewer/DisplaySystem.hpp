#pragma once

#include <string>
#include <glm/glm.hpp>

#include "System.hpp"

struct GLFWwindow;
class MouseSystem;


//  The DisplaySystem renders entities via OpenGL.
//
class DisplaySystem : public System {
public:
    ~DisplaySystem();

    void init();
    void update(Components& components, MouseSystem& mouse);
    bool should_close();
    GLFWwindow* get_window() { return window; };

    int get_window_width() const { return window_width; }
    int get_window_height() const { return window_height; }
    const glm::mat4& get_view() const { return view; }
    const glm::mat4& get_projection() const { return projection; }
    const glm::mat4& get_view_inverse() const { return view_inverse; }
    const glm::mat4& get_projection_inverse() const { return projection_inverse; }
    const glm::vec3 get_look_from() const { return lookFrom; }

    //  Returns a unit vector in world space in the direction of the viewport's right and front.
    const glm::vec3& get_camera_right() const { return cameraRight; }
    const glm::vec3& get_camera_front() const { return cameraFront; }

    //  Get the point on the XY plane that the camera's focusing on.
    glm::vec3 get_camera_focus() { return cameraFocus; }

    //  Get the distance the camera is from the focus point.
    float get_camera_distance() { return cameraDistance; }

    void set_camera(const glm::vec3& focus, float distance);

    float get_ambient() const { return objectAmbientStrength; }
    void set_ambient(float f) { objectAmbientStrength = f; }
    float get_diffuse() const { return objectDiffuseStrength; }
    void set_diffuse(float f) { objectDiffuseStrength = f; }

private:
    std::string name = "Lansnoop Viewer";
    int window_width = 800, window_height = 600;
    GLFWwindow* window;
    glm::vec3 cameraFocus; //  The point in the model that the camera is looking at.
    glm::vec3 cameraRight;
    glm::vec3 cameraFront;
    glm::vec3 lookFrom; // Eye position relative to lookAt.
    float cameraDistance; // Distance from camera to the focus.
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 view_inverse;
    glm::mat4 projection_inverse;

    int frames = 0;

    unsigned int objectShaderModelLoc;
    unsigned int objectShaderViewLoc;
    unsigned int objectShaderProjectionLoc;
    unsigned int objectShaderColorLoc;
    unsigned int objectShaderAmbientStrengthLoc;
    unsigned int objectShaderDiffuseStrengthLoc;
    float objectDiffuseStrength = 0.75f;
    float objectAmbientStrength = 0.2f;

#if 0
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    unsigned int depthMap;
    unsigned int shadowShader;
#endif

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

#if 0
    void init_shadow_shaders();
    void shadow_buffer_init();
    void render_shadow_map(Components& components);
#endif

    void init_object_shaders();
    void init_line_shaders();
    void init_cube_vao();
    void init_cylinder_vao();
    void drawLine(float ax, float ay, float az, float bx, float by, float bz);
};
