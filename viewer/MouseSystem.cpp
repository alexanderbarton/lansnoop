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
            float t = (0.5f - display.get_look_from().z) / ray_wor.z;
            glm::vec3 z05 = display.get_look_from() + ray_wor * t;
            t = (1.5f - display.get_look_from().z) / ray_wor.z;
            glm::vec3 z15 = display.get_look_from() + ray_wor * t;

            // for (const auto& [location, shape] : Components::Join(components.location_components, components.shape_components)) {
            for (const LocationComponent& location : components.location_components) {
                //  Check for intersection of the ray with the top of the
                //  box at Z=1.5 or the bottom of the box at Z=0.5.
                if ((location.x - 0.5f < z05.x
                    && z05.x < location.x + 0.5f
                    && location.y - 0.5f < z05.y
                    && z05.y < location.y + 0.5f)
                    || (location.x - 0.5f < z15.x
                    && z15.x < location.x + 0.5f
                    && location.y - 0.5f < z15.y
                    && z15.y < location.y + 0.5f)
                ) {
                    new_hover_id = location.entity_id;
                    break;
                }
            }
        }
#if 0
    if (new_hover_id != this->hoverId) {
        if (new_hover_id) {
            try {
                DescriptionComponent& description = components.get(this->hoverId, components.description_components);
                std::cout << "mouse hovered entity " << this->hoverId << ": " << description.description << std::endl;
            } catch (...) {
                std::cout << "mouse hovered entity " << this->hoverId << ": (no description)" << std::endl;
            }
        }
    }
#endif
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
