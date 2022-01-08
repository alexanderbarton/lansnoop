#pragma once

#include <map>

#include <glm/glm.hpp>
#include "System.hpp"

class Components;
class DisplaySystem;
class MouseSystem;

class LabelSystem : public System {
public:
    void init();
    void update(Components&, DisplaySystem&, MouseSystem&);

private:
    struct Character {
        unsigned int TextureID;  // ID handle of the glyph texture
        glm::ivec2   Size;       // Size of glyph
        glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
        long         Advance;    // Offset to advance to next glyph
    };
    std::map<char, Character> Characters;

    unsigned int objectShader;
    unsigned int VAO, VBO;

    void render_text(const std::string& text, float x, float y, float scale, glm::vec3 color);
    void render_label(const LabelComponent& label, const LocationComponent& location, DisplaySystem& display);
};
