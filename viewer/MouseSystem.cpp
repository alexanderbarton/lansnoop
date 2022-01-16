#include <cmath>
#include <array>

#include "util.hpp"
#include "MouseSystem.hpp"
#include "DisplaySystem.hpp"

#include "/home/abarton/debug.hpp"


static MouseSystem* msp;

extern "C" {
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        msp->scroll_callback(xoffset, yoffset);
    }

    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
    {
        msp->mouse_button_callback(button, action, mods);
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
    glfwSetMouseButtonCallback(window, ::mouse_button_callback);
}


void MouseSystem::update(Components& components, DisplaySystem& display)
{
    //  Mouse wheel zoom
    //  Six mouse wheel clicks doubles (or halves) our viewing distance.
    //
    if (this->wheel_displacement_y > 0.5f || this->wheel_displacement_y < -0.5f) {
        constexpr float sixth = 1.122462f;  //  Sixth root of two.
        display.set_camera(display.get_camera_focus(), std::pow(sixth, wheel_displacement_y) * display.get_camera_distance());
        this->wheel_displacement_y = 0.f;
    }

    //  Detect mouse hovering over any location component.
    //
    int new_hover_id = 0;
    double xpos = 0.0, ypos = 0.0;
    glfwGetCursorPos(this->window, &xpos, &ypos);
    // check_glfw_error("glfwGetCursorPos()");
    float x = (2.f * xpos) / display.get_window_width() - 1.f;
    float y = 1.f - (2.f * ypos) / display.get_window_height();
    glm::vec3 ray_nds(x, y, 1.f);
    if (ray_nds.x > -1.f && ray_nds.x < 1.f)
        if (ray_nds.y > -1.f && ray_nds.y < 1.f) {
            glm::vec4 ray_clip(ray_nds.x, ray_nds.y, -1.f, 1.f);
            glm::vec4 ray_eye = display.get_projection_inverse() * ray_clip;
            ray_eye.z = -1.f;
            ray_eye.w = 0.f;
            glm::vec3 ray_wor = display.get_view_inverse() * ray_eye;
            ray_wor = normalize(ray_wor);
            // TODO: bail out if ray_wor.z is near 0.0.

            //  Find the intersection of the mouse ray with any of the six
            //  faces making up a cube centered at location.
            glm::vec3 A = display.get_look_from();
            glm::vec3 B = ray_wor;
            const float radius = 0.5f; //  Distance from center to face.
            const std::array face_normals {
                glm::vec3( 0.f,  0.f,  1.f),
                glm::vec3( 0.f,  0.f, -1.f),
                glm::vec3( 1.f,  0.f,  0.f),
                glm::vec3(-1.f,  0.f,  0.f),
                glm::vec3( 0.f,  1.f,  0.f),
                glm::vec3( 0.f, -1.f,  0.f),
            };
            float min_t = FLT_MAX;
            for (const LocationComponent& location : components.location_components) {
                for (const glm::vec3& N : face_normals) {
                    glm::vec3 P = glm::vec3(location.x, location.y, 1.f) + N * radius;
                    float t = glm::dot(P-A, N) / glm::dot(B, N);
                    //  TODO: bail if glm::dot(B, N) ~= 0
                    glm::vec3 d(A+B*t-P);
                    if (std::max(fabs(d.x), std::max(fabs(d.y), fabs(d.z))) <= radius) {
                        if (t > 0.0 && t < min_t) {
                            new_hover_id = location.entity_id;
                            min_t = t;
                        }
                    }
                }
            }
        }
    this->hoverId = new_hover_id;

    //  Slave dragged entity's location to the mouse.
    //
    if (this->dragId) {
        double xpos = 0.0, ypos = 0.0;
        glfwGetCursorPos(this->window, &xpos, &ypos);
        // check_glfw_error("glfwGetCursorPos()");
        float x = (2.f * xpos) / display.get_window_width() - 1.f;
        float y = 1.f - (2.f * ypos) / display.get_window_height();
        glm::vec3 ray_nds(x, y, 1.f);
        if (ray_nds.x > -1.f && ray_nds.x < 1.f)
            if (ray_nds.y > -1.f && ray_nds.y < 1.f) {
                glm::vec4 ray_clip(ray_nds.x, ray_nds.y, -1.f, 1.f);
                glm::vec4 ray_eye = display.get_projection_inverse() * ray_clip;
                ray_eye.z = -1.f;
                ray_eye.w = 0.f;
                glm::vec3 ray_wor = display.get_view_inverse() * ray_eye;
                ray_wor = normalize(ray_wor);
                // TODO: bail out if ray_wor.z is near 0.0.
                float t = (1.0f - display.get_look_from().z) / ray_wor.z;
                glm::vec3 z10 = display.get_look_from() + ray_wor * t;

                LocationComponent& location = components.get(this->dragId, components.location_components);
                location.x = z10.x;
                location.y = z10.y;
            }
    }
    //  TODO: refactor all this mouse picking math.
}


void MouseSystem::scroll_callback(double xoffset, double yoffset)
{
    this->wheel_displacement_y += -yoffset;
}


void MouseSystem::mouse_button_callback(int button, int action, int mods)
{
    if (GLFW_MOUSE_BUTTON_1 == button) {
        if (GLFW_PRESS == action && this->hoverId) //  Begin dragging if something's under the mouse.
            this->dragId = this->hoverId;
        else if (GLFW_RELEASE == action) // Stop dragging.
            this->dragId = 0;
    }
}
