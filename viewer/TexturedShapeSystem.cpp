#include <stdexcept>
#include <array>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "TexturedShapeSystem.hpp"
#include "DisplaySystem.hpp"
#include "Components.hpp"

#include "/home/abarton/debug.hpp"


//  TODO: refactor

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


void TexturedShapeSystem::init_shaders()
{
    char infoLog[512];
    int success;

    const char *vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "out vec2 TexCoord;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   Normal = aNormal;\n"
        "   TexCoord = aTexCoord;\n"
        "}";
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader)
        check_gl_error("glCreateShader(GL_VERTEX_SHADER)");
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("textured shape vertex shader compilation failed: ") + infoLog);
    }

    const char *fragmentShaderSource =
        "#version 330 core\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec2 TexCoord;\n"
        "uniform float ambientStrength;\n"
        "uniform float diffuseStrength;\n"
        "uniform sampler2D ourTexture;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   vec3 lightPos = vec3(15.0, -15.0, 50.0);\n"
        "   vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
        "   vec3 ambient = ambientStrength * lightColor;\n"
        "   vec3 norm = normalize(Normal);\n"
        "   vec3 lightDir = normalize(lightPos - FragPos);\n"
        "   float diff = diffuseStrength * max(dot(norm, lightDir), 0.0);\n"
        "   vec3 diffuse = diff * lightColor;\n"
        "   vec3 result = (ambient + diffuse);\n"
        "   FragColor = vec4(result, 1.0) * texture(ourTexture, TexCoord);\n"
        "}";
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader)
        check_gl_error("glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("textured shape fragment shader compilation failed: ") + infoLog);
    }

    this->shader = glCreateProgram();
    if (!this->shader)
        check_gl_error("glCreateProgram()");
    glAttachShader(this->shader, vertexShader);
    glAttachShader(this->shader, fragmentShader);
    glLinkProgram(this->shader);
    glGetProgramiv(this->shader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->shader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("textured shape shader program linking failed: ") + infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    this->shaderModelLoc = glGetUniformLocation(this->shader, "model");
    this->shaderViewLoc = glGetUniformLocation(this->shader, "view");
    this->shaderProjectionLoc = glGetUniformLocation(this->shader, "projection");
    this->shaderAmbientStrengthLoc = glGetUniformLocation(this->shader, "ambientStrength");
    this->shaderDiffuseStrengthLoc = glGetUniformLocation(this->shader, "diffuseStrength");
}


void TexturedShapeSystem::init_vao()
{
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::array vertices {
    //  position (x,y,z)      normal (x,y,z)       texture (s,t)
    //  ====================  ==================   =============
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    };
    // first, configure the cube's VAO (and VBO)
    glGenVertexArrays(1, &this->VAO);
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);
    this->VAOLength = vertices.size() / 8;

    glBindVertexArray(this->VAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void TexturedShapeSystem::init()
{
    init_shaders();
    init_vao();
}


void TexturedShapeSystem::update(Components& components, DisplaySystem& display)
{
    /*
     *  Render textured shapes.
     */
    glUseProgram(this->shader);

    glUniformMatrix4fv(      this->shaderViewLoc, 1, GL_FALSE, glm::value_ptr(display.get_view()));
    glUniformMatrix4fv(this->shaderProjectionLoc, 1, GL_FALSE, glm::value_ptr(display.get_projection()));
    glUniform1f(this->shaderDiffuseStrengthLoc, display.get_diffuse());
    glUniform1f(this->shaderAmbientStrengthLoc, display.get_ambient());
    glBindVertexArray(this->VAO);

    //  Draw all shape components.
    //
    auto query = Components::Join(components.location_components, components.textured_shape_components);
    for (const auto& [location, tshape] : query) {
        float scale = 2.f;
        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, glm::vec3(location.x, location.y, location.z));
        model = glm::scale(model, glm::vec3(scale, scale, scale));
        glUniformMatrix4fv(this->shaderModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, tshape.texture);
        glDrawArrays(GL_TRIANGLES, 0, this->VAOLength);
    }
    glBindVertexArray(0);
}
