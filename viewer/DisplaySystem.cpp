#include <iostream>
#include <sys/time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "DisplaySystem.hpp"

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


static void throw_glfw_error(std::string context)
{
    const char* description;
    glfwGetError(&description);
    if (description) {
        context += ": ";
        context += description;
    }
    else
        context += " failed";
    throw std::runtime_error(context);
}


static void throw_gl_error(const std::string& context)
{
    std::string description = context;
    while (GLenum e = glGetError() != GL_NO_ERROR) {
        description += " ";
        description += (const char*)(glGetString(e));
    }
    throw std::runtime_error(description);
}


static void process_input(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


void DisplaySystem::init_shaders()
{
    char infoLog[512];
    int success;

    const char *vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}";
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader)
        throw_gl_error("glCreateShader(GL_VERTEX_SHADER)");
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("vertex shader compilation failed: ") + infoLog);
    }

    const char *fragmentShaderSource =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}";
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader)
        throw_gl_error("glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("fragment shader compilation failed: ") + infoLog);
    }

    unsigned int shaderProgram = glCreateProgram();
    if (!shaderProgram)
        throw_gl_error("glCreateProgram()");
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("shader program linking failed: ") + infoLog);
    }

    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}


bool DisplaySystem::should_close()
{
    return glfwWindowShouldClose(window);
}


void DisplaySystem::init()
{
    if (!glfwInit())
        throw_glfw_error("glfwInit()");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    this->window = glfwCreateWindow(window_width, window_height, this->name.c_str(), NULL, NULL);
    if (!this->window)
        throw_glfw_error("glfwCreateWindow()");
    glfwMakeContextCurrent(this->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");

    glViewport(0, 0, window_width, window_height);
    glfwSetFramebufferSizeCallback(this->window, framebuffer_size_callback);

    init_shaders();

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(this->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}


void DisplaySystem::update(Components& components)
{
    process_input(this->window);

    process_input(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw our first triangle
    // glUseProgram(shaderProgram);
    glBindVertexArray(this->VAO); // seeing as we only have a single VAO there's no need to
    // bind it every time, but we'll do so to keep things a bit more organized
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // glBindVertexArray(0); // no need to unbind it every time

    glfwSwapBuffers(window);
    glfwPollEvents();

    display_fps();
}
