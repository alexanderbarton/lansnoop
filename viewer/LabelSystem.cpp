#include <stdexcept>

#include <glad/glad.h>
// #include <GLFW/glfw3.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H 

#include "DisplaySystem.hpp"
#include "MouseSystem.hpp"
#include "LabelSystem.hpp"
#include "util.hpp"

#include "/home/abarton/debug.hpp"



//  Check for any pending OpenGL errors and if any throw.
//
static void check_gl_error(const std::string& context)
{
    std::string description = context;
    while (GLenum e = glGetError() != GL_NO_ERROR) {
        description += " ";
        description += (const char*)(glGetString(e));
    }
    throw std::runtime_error(description);
}


void LabelSystem::init()
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        throw std::runtime_error("FT_Init_FreeType(): Could not init FreeType Library");

    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", 0, &face))
        throw std::runtime_error("FT_New_Face(): Failed to load font");

    FT_Set_Pixel_Sizes(face, 0, 12);
    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
        throw std::runtime_error("FT_Load_Char(): Failed to load Glyph");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++) {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            throw std::runtime_error("FT_Load_Char(): Failed to load glyph");
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);


    char infoLog[512];
    int success;

    const char *vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
        "out vec2 TexCoords;\n"
        "\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
        "    TexCoords = vertex.zw;\n"
        "}";
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader)
        check_gl_error("LabelSystem: glCreateShader(GL_VERTEX_SHADER)");
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("object vertex shader compilation failed: ") + infoLog);
    }

    const char *fragmentShaderSource =
        "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 color;\n"
        "\n"
        "uniform sampler2D text;\n"
        "uniform vec3 textColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
        "    color = vec4(textColor, 1.0) * sampled;\n"
        "}";
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader)
        check_gl_error("LabelSystem: glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("object fragment shader compilation failed: ") + infoLog);
    }

    this->objectShader = glCreateProgram();
    if (!this->objectShader)
        check_gl_error("LabelSystem: glCreateProgram()");
    glAttachShader(this->objectShader, vertexShader);
    glAttachShader(this->objectShader, fragmentShader);
    glLinkProgram(this->objectShader);
    glGetProgramiv(this->objectShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->objectShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("object shader program linking failed: ") + infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void LabelSystem::render_text(const std::string& text, float x, float y, float scale, glm::vec3 color)
{
    glUniform3f(glGetUniformLocation(this->objectShader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(this->VAO);

    //  Adjust x so that text is centered on x.
    //
    long total_advance = 0;
    for (char c : text) {
        Character ch = Characters[(unsigned char)c];
        total_advance += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    x -= total_advance / 2;

    // iterate through all characters
    for (char c : text) {
        Character ch = Characters[(unsigned char)c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


void LabelSystem::update(Components& components, DisplaySystem& display, MouseSystem& mouse)
{
    glUseProgram(this->objectShader);
    float window_width = display.get_window_width();
    float window_height = display.get_window_height();
    glm::mat4 projection = glm::ortho(0.0f, window_width, 0.0f, window_height);
    glUniformMatrix4fv(glGetUniformLocation(this->objectShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

#if 1
    //  Render a label for the entity under the mouse.
    //
    int id = mouse.get_hover_id();
    if (id) {
        const LabelComponent* label = components.find(id, components.label_components);
        if (label) {
            const LocationComponent& location = components.get(id, components.location_components);
            this->render_label(*label, location, display);
        }
    }
#else
    //  Render labels for all entities.
    //
    for (const auto& [ location, label] : Components::Join(components.location_components, components.label_components))
        this->render_label(label, location, display);
#endif
}


void LabelSystem::render_label(const LabelComponent& label, const LocationComponent& location, DisplaySystem& display)
{
    glm::vec4 w(location.x, location.y, 0.5f, 1.f);
    w = w - glm::vec4(display.get_camera_front(), 1.f) / 2.f;
    w.w = 1.f;
    glm::vec4 v = display.get_view() * w;
    v.w = 1.f;
    glm::vec4 s = display.get_projection() * v;
    s = (s / s.w + glm::vec4(1.f, 1.f, 0.f, 0.f)) / 2.f;
    float x = s.x * display.get_window_width();
    float y = s.y * display.get_window_height();

    glm::vec3 color(0.5, 0.8f, 0.2f);
    glm::vec3 shadow(0.f, 0.f, 0.f);
    y -= 14.f;

    x = floorf(x);
    y = floorf(y);

    for (const std::string& l : label.labels) {
        render_text(l, x, y, 1.0f, color);
        render_text(l, x-1.0f, y-1.0f, 1.0f, shadow);
        y -= 14;
    }
}
