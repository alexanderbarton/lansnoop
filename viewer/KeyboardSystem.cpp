#include <iostream>

#include "KeyboardSystem.hpp"
#include "FDGSystem.hpp"
#include "DisplaySystem.hpp"


static KeyboardSystem* system_ptr;
extern "C" {
    static void character_callback(GLFWwindow* window, unsigned int codepoint)
    {
        system_ptr->character_callback(codepoint);
    }
}


void KeyboardSystem::init(GLFWwindow* window)
{
    this->window = window;
    system_ptr = this;
    glfwSetCharCallback(window, ::character_callback);
}


void KeyboardSystem::update(Components& components, FDGSystem& fdg, DisplaySystem& display)
{
    for (unsigned int codepoint : this->pending_chars) {
        switch (codepoint) {

            // case GLFW_KEY_ESCAPE:
            case 'Q':
            case 'q':
                glfwSetWindowShouldClose(window, true);
                break;

            case 'T':
            case 't':
                components.describe_entities();
#if 0
                for (const auto& [location, description] : Components::Join(components.location_components, components.description_components)) {
                    std::cout << location.entity_id << " " << description.description << "\n";
                }
#endif
                break;

            case '>':
                this->current_parameter = Parameter(int(this->current_parameter) + 1);
                if (int(this->current_parameter) >= int(Parameter::NONE))
                    this->current_parameter = Parameter::FDG_REPULSION;
                std::cout << "Current Tweakable parameter: " << to_s(this->current_parameter)
                    // << " (now " << this->to_s(current_parameter) << ")"
                    << std::endl;
                break;

            case '<':
                if (int(this->current_parameter) <= 0)
                    this->current_parameter = Parameter::NONE;
                this->current_parameter = Parameter(int(this->current_parameter) - 1);
                std::cout << "Current tweakable parameter: " << to_s(this->current_parameter)
                    // << " (now " << this->to_s(current_parameter) << ")"
                    << std::endl;
                break;

            case '+':
            case '-': {
                if (Parameter::NONE == this->current_parameter)
                    break;
                float factor = 1.2599; //  Third root of 2.
                if (codepoint == '-')
                    factor = 1.f/factor;
                std::cout << "parameter " << this->to_s(current_parameter) << " now ";
                switch (this->current_parameter) {
                    case Parameter::FDG_REPULSION:
                        std::cout << (fdg.k_repulsion *= factor);
                        break;
                    case Parameter::FDG_EDGE_ATTRACTION:
                        std::cout << (fdg.k_link_attraction *= factor);
                        break;
                    case Parameter::FDG_ORIGIN:
                        std::cout << (fdg.k_origin *= factor);
                        break;
                    case Parameter::FDG_DRAG:
                        fdg.k_inverse_drag = 1.f-(1.f-fdg.k_inverse_drag)*factor;
                        std::cout << fdg.k_inverse_drag;
                        break;
                    case Parameter::FDG_INERTIA:
                        std::cout << (fdg.k_vertex_inertia *= factor);
                        break;
                    case Parameter::LIGHTING_DIFFUSE:
                        display.set_diffuse(display.get_diffuse() * factor);
                        std::cout << display.get_diffuse();
                        break;
                    case Parameter::LIGHTING_AMBIENT:
                        display.set_ambient(display.get_ambient() * factor);
                        std::cout << display.get_ambient();
                        break;
                    case Parameter::NONE:
                        break;
                }
                std::cout << std::endl;
                break;
            }

            case '?':
            case 'h':
                std::cout << "Keybaord commands:\n";
                std::cout << "  Q    quit\n";
                std::cout << "  A,D  navigate left and right\n";
                std::cout << "  W,S  navigate fore and back\n";
                // std::cout << "  D    dump all entities\n";
                std::cout << "  T    list all entity descriptions\n";
                std::cout << "  <,>  select a parameter to be adjusted\n";
                std::cout << "  +,-  make the selected parameter larger or smaller\n";
                std::cout << "  ?,h  show this help\n";
                std::cout << std::flush;
                break;

            default:
                break;
        }
    }
    this->pending_chars.clear();


    //  First person movement by holding down the WASD keys
    //
    float x = 0.f, y = 0.f;
    bool moved = false;
    if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_D)) {
        x += 1.f;
        moved = true;
    }
    if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_A)) {
        x -= 1.f;
        moved = true;
    }
    if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_W)) {
        y += 1.f;
        moved = true;
    }
    if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_S)) {
        y -= 1.f;
        moved = true;
    }
    if (moved) {
        //  Make movement speed a function of field of view.
        glm::vec3 focus = display.get_camera_focus();
        const float dt = 1.f / 60.f; // Assume 60 fps.
        float speed = glm::length(focus - display.get_look_from());
        glm::vec3 displacement = speed * dt * (display.get_camera_right() * x + display.get_camera_front() * y);
        display.set_camera(focus + displacement, display.get_camera_distance());
    }
}


void KeyboardSystem::character_callback(unsigned int codepoint)
{
    this->pending_chars.push_back(codepoint);
}


const char* KeyboardSystem::to_s(Parameter p)
{
    switch (p) {
        case Parameter::FDG_REPULSION:       return "FDG_REPULSION";
        case Parameter::FDG_EDGE_ATTRACTION: return "FDG_EDGE_ATTRACTION";
        case Parameter::FDG_ORIGIN:          return "FDG_ORIGIN";
        case Parameter::FDG_DRAG:            return "FDG_DRAG";
        case Parameter::FDG_INERTIA:         return "FDG_INERTIA";
        case Parameter::LIGHTING_DIFFUSE:    return "LIGHTING_DIFFUSE";
        case Parameter::LIGHTING_AMBIENT:    return "LIGHTING_AMBIENT";
        case Parameter::NONE:                return "NONE";
    }
    return "(invalid)";
}
