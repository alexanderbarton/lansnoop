#include <iostream>
#include <stdexcept>

#include "Components.hpp"
#include "NetworkModelSystem.hpp"
#include "DisplaySystem.hpp"
#include "FDGSystem.hpp"
#include "KeyboardSystem.hpp"
#include "MouseSystem.hpp"


class Viewer {
public:
    void open(const std::string& path);
    void run(const char*);

private:
    Components components;
    NetworkModelSystem network_model_system;
    DisplaySystem display_system;
    FDGSystem fdg_system;
    KeyboardSystem keyboard_system;
    MouseSystem mouse_system;
};



void Viewer::open(const std::string& path)
{
    network_model_system.open(path);
}


void Viewer::run(const char* argv0)
{
    this->display_system.init();
    this->network_model_system.init();
    this->fdg_system.init();
    this->keyboard_system.init(display_system.get_window());
    this->mouse_system.init(display_system.get_window());

    while (!this->display_system.should_close())
    {
        this->network_model_system.update(this->components);
        this->fdg_system.update(this->components);
        this->keyboard_system.update(this->components, this->fdg_system, this->display_system);
        this->mouse_system.update(this->components, this->display_system);
        this->display_system.update(this->components);
    }
    std::cerr << "\n";
}


int main(int argc, char **argv)
{
    int ret = 0;
    try {
        Viewer viewer;

        int i = 1;
        while (i < argc) {
            viewer.open(argv[i++]);
        }

        viewer.run(argv[0]);
    }
    catch (const std::exception& e) {
        std::cerr << argv[0] << ": " << e.what() << std::endl;
        ret = 1;
    }
    catch (...) {
        std::cerr << argv[0] << ": unhandled exception" << std::endl;
        ret = 1;
    }
    return ret;
}
