#include <iostream>
#include <array>
#include <sys/time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "DisplaySystem.hpp"
#include "MouseSystem.hpp"
#include "util.hpp"

#include "/home/abarton/debug.hpp"



DisplaySystem::~DisplaySystem()
{
    glfwTerminate();
}



//  Display FPS to stderr once a second.
//
static void display_fps()
{
    static long frame_count = 0, previous_frame_count = 0;
    static timeval now { 0, 0 };
    ++frame_count;

    //  First time through initialization.
    if (!now.tv_sec) {
        gettimeofday(&now, nullptr);
        return;
    }
    timeval new_now;
    gettimeofday(&new_now, nullptr);
    if (new_now.tv_sec != now.tv_sec) {
        long dt_us = (new_now.tv_sec - now.tv_sec) * 1000000 + new_now.tv_usec - now.tv_usec;
        long frame_rate = 1000000 * (frame_count-previous_frame_count) / dt_us;
        std::cerr
            // << new_now.tv_sec << "s"
            // << ", " << frame_count << " frames"
            // << ", "
            << frame_rate << " fps"
            // ", " << dt_us << " us"
            << "       \r" << std::flush;
        now = new_now;
        previous_frame_count = frame_count;
    }
}


extern "C" {
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        show(width);
        show(height);
        glViewport(0, 0, width, height);
    }
}


//  Check for any pending GLFW errors and if any throw.
//
static void check_glfw_error(std::string context)
{
    const char* description;
    int code = glfwGetError(&description);
    if (code != GLFW_NO_ERROR) {
        context += ": GLFW error ";
        context += std::to_string(code);
        if (description) {
            context += ": ";
            context += description;
        }
        throw std::runtime_error(context);
    }
}


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


void DisplaySystem::init_object_shaders()
{
    char infoLog[512];
    int success;

    const char *vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   Normal = aNormal;\n"
        "}";
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader)
        check_gl_error("glCreateShader(GL_VERTEX_SHADER)");
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("object vertex shader compilation failed: ") + infoLog);
    }

    const char *fragmentShaderSource =
        "#version 330 core\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "uniform vec3 objectColor;\n"
        "uniform float ambientStrength;\n"
        "uniform float diffuseStrength;\n"
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
        "   vec3 result = (ambient + diffuse) * objectColor;\n"
        "   FragColor = vec4(result, 1.0);\n"
        "}";
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader)
        check_gl_error("glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("object fragment shader compilation failed: ") + infoLog);
    }

    this->objectShader = glCreateProgram();
    if (!this->objectShader)
        check_gl_error("glCreateProgram()");
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
}


#if 0
void DisplaySystem::init_shadow_shaders()
{
    char infoLog[512];
    int success;

    const char *vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "\n"
        "uniform mat4 lightSpaceMatrix;\n"
        "uniform mat4 model;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);\n"
        "}\n";
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader)
        check_gl_error("glCreateShader(GL_VERTEX_SHADER)");
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("shadow vertex shader compilation failed: ") + infoLog);
    }

    const char *fragmentShaderSource =
        "#version 330 core\n"
        "\n"
        "void main()\n"
        "{\n"
        "    // gl_FragDepth = gl_FragCoord.z;\n"
        "}\n";
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader)
        check_gl_error("glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("shadow fragment shader compilation failed: ") + infoLog);
    }

    this->shadowShader = glCreateProgram();
    if (!this->shadowShader)
        check_gl_error("glCreateProgram()");
    glAttachShader(this->shadowShader, vertexShader);
    glAttachShader(this->shadowShader, fragmentShader);
    glLinkProgram(this->shadowShader);
    glGetProgramiv(this->shadowShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->shadowShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("shadow shader program linking failed: ") + infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}


void DisplaySystem::shadow_buffer_init()
{
    glGenFramebuffers(1, &this->depthMapFBO);

    glGenTextures(1, &this->depthMap);
    glBindTexture(GL_TEXTURE_2D, this->depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 this->SHADOW_WIDTH, this->SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, this->depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void DisplaySystem::render_shadow_map(Components& components)
{
    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(glm::vec3(15.0, -15.0, 50.0),  //  eye
                                      // glm::vec3(-2.0f, 4.0f, -1.0f),  //  eye
                                      glm::vec3( 0.0f, 0.0f,  0.0f),  //  target
                                      glm::vec3( 0.0f, 1.0f,  0.0f)); //  up
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glUseProgram(this->shadowShader);
    unsigned int lightSpaceMatrixLoc = glGetUniformLocation(this->objectShader, "lightSpaceMatrix");
    unsigned int modelLoc = glGetUniformLocation(this->objectShader, "model");
    glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glViewport(0, 0, this->SHADOW_WIDTH, this->SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, this->depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    //  Draw all shape components.
    //
    auto query = Components::Join(components.location_components, components.shape_components);
    for (const auto& [location, shape] : query) {
        glm::mat4 model(1.f);
        model = glm::translate(model, glm::vec3(location.x, location.y, location.z));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        switch (shape.shape) {
            case ShapeComponent::Shape::BOX:
                glBindVertexArray(this->cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, this->cubeVAOLength);
                break;
            case ShapeComponent::Shape::CYLINDER:
                glBindVertexArray(this->cylinderVAO);
                glDrawArrays(GL_TRIANGLES, 0, this->cylinderVAOLength);
                break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif


void DisplaySystem::init_line_shaders()
{
    char infoLog[512];
    int success;

    const char *vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * vec4(aPos, 1.0);\n"
        "}";
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader)
        check_gl_error("glCreateShader(GL_VERTEX_SHADER)");
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("line vertex shader compilation failed: ") + infoLog);
    }

    const char *fragmentShaderSource =
        "#version 330 core\n"
        "uniform vec3 lineColor;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(lineColor, 1.0);\n"
        "}";
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader)
        check_gl_error("glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("line fragment shader compilation failed: ") + infoLog);
    }

    this->lineShader = glCreateProgram();
    if (!this->lineShader)
        check_gl_error("glCreateProgram()");
    glAttachShader(this->lineShader, vertexShader);
    glAttachShader(this->lineShader, fragmentShader);
    glLinkProgram(this->lineShader);
    glGetProgramiv(this->lineShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->lineShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("line shader program linking failed: ") + infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}


bool DisplaySystem::should_close()
{
    return glfwWindowShouldClose(window);
}


void DisplaySystem::init_cube_vao()
{
#if 0
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
         0.5f,  0.5f,  0.5f,  // top right
         0.5f, -0.5f,  0.5f,  // bottom right
        -0.5f, -0.5f,  0.5f,  // bottom left
        -0.5f,  0.5f,  0.5f,  // top left
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // +Z side
        1, 2, 3,  // +Z side
        4, 5, 7,  // -Z side
        5, 6, 7,  // -Z side
        4, 0, 1,  // +X side
        1, 5, 4,  // +X side
        3, 2, 6,  // -X side
        2, 6, 7,  // -X side
        0, 4, 3,  // +Z side
        4, 7, 3,  // +Z side
        1, 5, 2,  // -Z side
        5, 6, 2,  // -Z side
    };
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s),
    // and then configure vertex attributes(s).
    glBindVertexArray(this->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
#else
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::array vertices {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
    };
    // first, configure the cube's VAO (and VBO)
    glGenVertexArrays(1, &this->cubeVAO);
    unsigned int cubeVBO;
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);
    this->cubeVAOLength = vertices.size() / 6;

    glBindVertexArray(this->cubeVAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


//  Build a cylinder of height 1.0  diameter 1 on the Z axis ceneted on the origin.
//
void DisplaySystem::init_cylinder_vao()
{
    std::vector<float> vertices; // Six floats per vertex.
    const int sides = 16;
    const float radius = 0.5f;

    auto add_vertex = [&vertices](const std::array<float, 6>& vs) {
        for (const float v : vs)
            vertices.push_back(v);
    };

    for (int side=0; side<sides; ++side) {
        float ax = radius * cos(2.f * 3.14159f / sides * side);
        float bx = radius * cos(2.f * 3.14159f / sides * (1+side));
        float ay = radius * sin(2.f * 3.14159f / sides * side);
        float by = radius * sin(2.f * 3.14159f / sides * (1+side));

        //  Top.
        add_vertex({ 0.f, 0.f, 0.5f, 0.f, 0.f, 1.f });
        add_vertex({ ax, ay, 0.5f, 0.f, 0.f, 1.f });
        add_vertex({ bx, by, 0.5f, 0.f, 0.f, 1.f });

        //  Bottom.
        add_vertex({ 0.f, 0.f, -0.5f, 0.f, 0.f, -1.f });
        add_vertex({ ax, ay, -0.5f, 0.f, 0.f, -1.f });
        add_vertex({ bx, by, -0.5f, 0.f, 0.f, -1.f });

        //  Side.
        add_vertex({ ax, ay,  0.5f, ax/radius, ay/radius, 0.f });
        add_vertex({ bx, by,  0.5f, bx/radius, by/radius, 0.f });
        add_vertex({ ax, ay, -0.5f, ax/radius, ay/radius, 0.f });
        add_vertex({ ax, ay, -0.5f, ax/radius, ay/radius, 0.f });
        add_vertex({ bx, by, -0.5f, bx/radius, by/radius, 0.f });
        add_vertex({ bx, by,  0.5f, bx/radius, by/radius, 0.f });
    }

    glGenVertexArrays(1, &this->cylinderVAO);
    unsigned int cylinderVBO;
    glGenBuffers(1, &cylinderVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
    this->cylinderVAOLength = vertices.size() / 6;

    glBindVertexArray(this->cylinderVAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void DisplaySystem::init()
{
    if (!glfwInit())
        check_glfw_error("glfwInit()");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    this->window = glfwCreateWindow(this->window_width, this->window_height, this->name.c_str(), NULL, NULL);
    if (!this->window)
        check_glfw_error("glfwCreateWindow()");
    glfwMakeContextCurrent(this->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");

    glViewport(0, 0, this->window_width, window_height);
    glfwSetFramebufferSizeCallback(this->window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    init_object_shaders();
    init_line_shaders();
    init_cube_vao();
    init_cylinder_vao();

    // note that this is allowed, the call to glVertexAttribPointer registered
    // VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound
    // element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally
    // modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't
    // unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glEnable(GL_MULTISAMPLE);


    /*  Set up for line drawing.
     */
    for (float &v : this->line_vertices)
        v = 0.0;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_DYNAMIC_DRAW);
    glBindVertexArray(lineVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    // glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glLineWidth(1.f);

    glEnable(GL_DEPTH_TEST);

    this->objectShaderModelLoc = glGetUniformLocation(this->objectShader, "model");
    this->objectShaderViewLoc = glGetUniformLocation(this->objectShader, "view");
    this->objectShaderProjectionLoc = glGetUniformLocation(this->objectShader, "projection");
    this->objectShaderColorLoc = glGetUniformLocation(this->objectShader, "objectColor");
    this->objectShaderAmbientStrengthLoc = glGetUniformLocation(this->objectShader, "ambientStrength");
    this->objectShaderDiffuseStrengthLoc = glGetUniformLocation(this->objectShader, "diffuseStrength");

    this->lineShaderViewLoc = glGetUniformLocation(this->lineShader, "view");
    this->lineShaderProjectionLoc = glGetUniformLocation(this->lineShader, "projection");
    this->lineShaderLineColorLoc = glGetUniformLocation(this->lineShader, "lineColor");

    this->set_camera(glm::vec3(0.0f, 0.0f, 0.0f), 32.f);
}


void DisplaySystem::drawLine(float ax, float ay, float az, float bx, float by, float bz)
{
    glBindVertexArray(lineVAO);
    line_vertices[0] = ax;
    line_vertices[1] = ay;
    line_vertices[2] = az;
    line_vertices[3] = bx;
    line_vertices[4] = by;
    line_vertices[5] = bz;
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}


void DisplaySystem::update(Components& components, MouseSystem& mouse_system)
{
    /*
     *  Render objects.
     */
    glViewport(0, 0, this->window_width, this->window_height);
    glClearColor(0.05f, 0.07f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glBindTexture(GL_TEXTURE_2D, this->depthMap);
    glUseProgram(this->objectShader);

    glm::mat4 model = glm::mat4(1.f);
    glUniformMatrix4fv(     this->objectShaderModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(      this->objectShaderViewLoc, 1, GL_FALSE, glm::value_ptr(this->view));
    glUniformMatrix4fv(this->objectShaderProjectionLoc, 1, GL_FALSE, glm::value_ptr(this->projection));
    glUniform1f(this->objectShaderDiffuseStrengthLoc, this->objectDiffuseStrength);
    glUniform1f(this->objectShaderAmbientStrengthLoc, this->objectAmbientStrength);
    glLineWidth(1.f);

    //  Draw all shape components.
    //
    // glBindVertexArray(this->cubeVAO);
    for (const auto& [location, shape] : Components::Join(components.location_components, components.shape_components)) {
        model = glm::mat4(1.f);
        model = glm::translate(model, glm::vec3(location.x, location.y, location.z));
        glUniformMatrix4fv(this->objectShaderModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        float brightness = shape.entity_id == mouse_system.get_hover_id() ? 1.5f : 1.0f;
        switch (shape.shape) {
            case ShapeComponent::Shape::BOX:
                glUniform3fv(this->objectShaderColorLoc, 1, glm::value_ptr(shape.color * brightness));
                glBindVertexArray(this->cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, this->cubeVAOLength);
                break;
            case ShapeComponent::Shape::CYLINDER:
                glUniform3fv(this->objectShaderColorLoc, 1, glm::value_ptr(shape.color * brightness));
                glBindVertexArray(this->cylinderVAO);
                glDrawArrays(GL_TRIANGLES, 0, this->cylinderVAOLength);
                break;
        }
    }


#if 1
    //  Draw a cross at the mouse position on the z=0 plane.
    //
    double xpos = 0.0, ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    check_glfw_error("glfwGetCursorPos()");
    float x = (2.f * xpos) / this->window_width - 1.f;
    float y = 1.f - (2.f * ypos) / this->window_height;
    glm::vec3 ray_nds(x, y, 1.f);

    if (ray_nds.x > -1.f && ray_nds.x < 1.f)
        if (ray_nds.y > -1.f && ray_nds.y < 1.f) {
            glm::vec4 ray_clip(ray_nds.x, ray_nds.y, -1.f, 1.f);
            glm::vec4 ray_eye = this->projection_inverse * ray_clip;
            ray_eye.z = -1.f;
            ray_eye.w = 0.f;
            glm::vec3 ray_wor = this->view_inverse * ray_eye;
            ray_wor = normalize(ray_wor);
            // TODO: bail out if ray_wor.z is near 0.0.
            float t = -this->lookFrom.z / ray_wor.z;
            glm::vec3 z0 = lookFrom + ray_wor * t;

            glUseProgram(this->lineShader);
            glUniformMatrix4fv(      this->lineShaderViewLoc, 1, GL_FALSE, glm::value_ptr(this->view));
            glUniformMatrix4fv(this->lineShaderProjectionLoc, 1, GL_FALSE, glm::value_ptr(this->projection));

            glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f) * 0.5f;
            glUniform3fv(this->lineShaderLineColorLoc, 1, glm::value_ptr(color));
            glLineWidth(3.f);
            drawLine(z0.x-0.5f, z0.y,      z0.z, z0.x+0.5f, z0.y,      z0.z);
            drawLine(z0.x,      z0.y-0.5f, z0.z, z0.x,      z0.y+0.5f, z0.z);
            glLineWidth(3.f);
        }
#endif

#if 0
    //  Draw a box at the mouse position on the z=0 plane.
    //
    double xpos = 0.0, ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    check_glfw_error("glfwGetCursorPos()");
    float x = (2.f * xpos) / this->window_width - 1.f;
    float y = 1.f - (2.f * ypos) / this->window_height;
    glm::vec3 ray_nds(x, y, 1.f);

    if (ray_nds.x > -1.f && ray_nds.x < 1.f)
        if (ray_nds.y > -1.f && ray_nds.y < 1.f) {
            glm::vec4 ray_clip(ray_nds.x, ray_nds.y, -1.f, 1.f);
            glm::vec4 ray_eye = this->projection_inverse * ray_clip;
            ray_eye.z = -1.f;
            ray_eye.w = 0.f;
            glm::vec3 ray_wor = this->view_inverse * ray_eye;
            ray_wor = normalize(ray_wor);
            // TODO: bail out if ray_wor.z is near 0.0.
            float t = -this->lookFrom.z / ray_wor.z;
            glm::vec3 z0 = lookFrom + ray_wor * t;

            model = glm::mat4(1.f);
            model = glm::translate(model, z0);
            glUniformMatrix4fv(this->objectShaderModelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(this->objectShaderColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 0.5f, 0.2f)));
            glBindVertexArray(this->cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, this->cubeVAOLength);
        }
#endif


#if 1
    //  Render the interface_edge_components.
    //
    glUseProgram(this->lineShader);
    glUniformMatrix4fv(      this->lineShaderViewLoc, 1, GL_FALSE, glm::value_ptr(this->view));
    glUniformMatrix4fv(this->lineShaderProjectionLoc, 1, GL_FALSE, glm::value_ptr(this->projection));

    for (const auto [location, edge] : Components::Join(components.location_components, components.interface_edge_components)) {
        const LocationComponent& other = components.get(edge.other_entity_id, components.location_components);
        float rb = 0.2f;
        float g = 0.2f;
        g += edge.glow / 5.f;
        if (g > 1.f) {
            rb += (g - 1.f) / 10;
            rb = std::min(rb, 1.f);
        }
        g = std::min(g, 1.f);
        if (edge.glow >= 1.f)
            glLineWidth(3.f);
        else
            glLineWidth(1.f);
        glUniform3fv(this->lineShaderLineColorLoc, 1, glm::value_ptr(glm::vec3(rb, g, rb)));
        drawLine(location.x, location.y, 1.f, other.x, other.y, 1.f);
        edge.glow *= 0.8; // Exponential decay.
    }
    glLineWidth(1.f);

    //  16x16 XY grid at Z=0.
    //
    glUniform3fv(this->lineShaderLineColorLoc, 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.2f)));
    for (int x=-16; x<=16; ++x)
        drawLine(x, 16.f, 0.f, x, -16.f, 0.f);
    for (int y=-16; y<=16; ++y)
        drawLine(16.f, y, 0.f, -16.f, y, 0.f);

    //  Mark the origin.
    //
    glLineWidth(3.f);
    drawLine(0.5f, 0.f, 0.f, -0.5f, 0.f, 0.f);
    drawLine(0.f, 0.5f, 0.f, 0.f, -0.5f, 0.f);
    glLineWidth(3.f);
#endif

    // glBindVertexArray(0); // no need to unbind it every time

    // glfwSwapBuffers(window);
    // glfwPollEvents();

    if (false)
        display_fps();

    ++this->frames;
}


void DisplaySystem::set_camera(const glm::vec3& focus, float distance)
{
    this->cameraFocus = focus;
    this->cameraDistance = distance;
    // glm::vec3 lookDir = normalize(glm::vec3(0.0f, -0.707f, 0.707f));
    // this->lookFrom = this->cameraFocus + lookDir * this->cameraDistance;
    glm::vec3 lookDir = glm::vec3(0.0f, -0.707f * 32, 2.f+distance);
    this->lookFrom = this->cameraFocus + lookDir;

    this->view = glm::lookAt(this->lookFrom, this->cameraFocus, glm::vec3(0.0f, 1.0f, 0.0f));
    // this->projection = glm::perspective(glm::radians(45.0f), 1.f*this->window_width/this->window_height, 0.1f, 3.f * this->cameraDistance);
    this->projection = glm::perspective(glm::radians(45.0f), 1.f*this->window_width/this->window_height, 0.1f, 600.f);
    this->view_inverse = inverse(this->view);
    this->projection_inverse = inverse(this->projection);

    this->cameraFront = -normalize(glm::vec3(lookDir.x, lookDir.y, 0.f));
    this->cameraRight = cross(this->cameraFront, glm::vec3(0.f, 0.f, 1.f));
}
