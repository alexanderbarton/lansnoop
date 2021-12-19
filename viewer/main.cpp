#include <iostream>
#include <stdexcept>

#include <sys/time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "/home/abarton/debug.hpp"


class Viewer {
public:
    void run(const char*);
private:
    void process_input(GLFWwindow* window);
    void init_shaders();
};



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


void Viewer::process_input(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


void Viewer::init_shaders()
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


#if 0
void render_something()
{
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    // ..:: Initialization code (done once (unless your object frequently changes)) :: ..
    // 1. bind Vertex Array Object
    glBindVertexArray(VAO);
    // 2. copy our vertices array in a buffer for OpenGL to use
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 3. then set our vertex attributes pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    [...]

    // ..:: Drawing code (in render loop) :: ..
    // 4. draw the object
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    someOpenGLFunctionThatDrawsOurTriangle();
}
#endif


void Viewer::run(const char* argv0)
{
    int window_width = 800, window_height = 600;

    if (!glfwInit())
        throw_glfw_error("glfwInit()");
    try {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(window_width, window_height, argv0, NULL, NULL);
        if (!window)
            throw_glfw_error("glfwCreateWindow()");
        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("Failed to initialize GLAD");

        glViewport(0, 0, window_width, window_height);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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
        unsigned int VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
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


        while (!glfwWindowShouldClose(window))
        {
            this->process_input(window);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // draw our first triangle
            // glUseProgram(shaderProgram);
            glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to
            // bind it every time, but we'll do so to keep things a bit more organized
            //glDrawArrays(GL_TRIANGLES, 0, 6);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            // glBindVertexArray(0); // no need to unbind it every time

            glfwSwapBuffers(window);
            glfwPollEvents();

            display_fps();
        }
        std::cerr << "\n";

        glfwTerminate();
    }
    catch (...) {
        glfwTerminate();
        throw;
    }
}


int main(int argc, char **argv)
{
    int ret = 0;
    try {
        Viewer viewer;
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
