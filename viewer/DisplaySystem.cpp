#include <iostream>
#include <sys/time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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


static void process_input(GLFWwindow* window, const Components& components)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        components.describe_entities();
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
        throw_gl_error("glCreateShader(GL_VERTEX_SHADER)");
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
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   float ambientStrength = 0.6;\n"
        "   float diffuseStrength = 0.4;\n"
        "   vec3 lightPos = vec3(15.0, -15.0, 50.0);\n"
        "   vec3 lightColor = vec3(0.75, 0.75, 0.75);\n"
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
        throw_gl_error("glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("object fragment shader compilation failed: ") + infoLog);
    }

    this->objectShader = glCreateProgram();
    if (!this->objectShader)
        throw_gl_error("glCreateProgram()");
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
        throw_gl_error("glCreateShader(GL_VERTEX_SHADER)");
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
        throw_gl_error("glCreateShader(GL_FRAGMENT_SHADER)");
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        throw std::runtime_error(std::string("line fragment shader compilation failed: ") + infoLog);
    }

    this->lineShader = glCreateProgram();
    if (!this->lineShader)
        throw_gl_error("glCreateProgram()");
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

    init_object_shaders();
    init_line_shaders();

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
    float vertices[] = {
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
    glGenBuffers(1, &this->cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, this->cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(this->cubeVAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
#endif

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

    //  Set up camera.
    //
    // this->lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    // this->lookFrom = glm::vec3(0.0f, -16.0f, 16.0f);
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


template <class T>
static T& find(std::vector<T>& component, int entity_id)
{
    for (T& c : component)
        if (c.entity_id == entity_id)
            return c;
    throw std::runtime_error(std::string("find component: entity not found: ") + std::to_string(entity_id));
    //  TODO: efficient lookup.
}


void DisplaySystem::update(Components& components)
{
    process_input(this->window, components);

    // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // glClearColor(0.1f, 0.15f, 0.15f, 1.0f);
    glClearColor(0.05f, 0.07f, 0.07f, 1.0f);
    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw our first triangle
    // glBindVertexArray(this->VAO);
    // seeing as we only have a single VAO there's no need to 
    // bind it every time, but we'll do so to keep things a bit more organized

    // glDrawArrays(GL_TRIANGLES, 0, 6);

    glm::mat4 model = glm::mat4(1.f);
    glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 lookFrom = glm::vec3(0.0f, 32*-0.707f, 32*0.707f);
    glm::mat4 view = glm::lookAt(lookAt + lookFrom, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.f*this->window_width/this->window_height, 0.1f, 100.0f);

    glUseProgram(this->objectShader);
    this->modelLoc = glGetUniformLocation(this->objectShader, "model");
    this->viewLoc = glGetUniformLocation(this->objectShader, "view");
    this->projectionLoc = glGetUniformLocation(this->objectShader, "projection");
    unsigned int objectColorLoc = glGetUniformLocation(this->objectShader, "objectColor");
    glUniformMatrix4fv(     this->modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(      this->viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(this->projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(glm::vec3(1.0, 0.5, 0.2)));

    for (const LocationComponent& c : components.location_components) {
        model = glm::mat4(1.f);
        model = glm::translate(model, glm::vec3(c.x, c.y, c.z));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
#if 0
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
#else
        // render the cube
        glBindVertexArray(this->cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
#endif
    }

#if 1
    glUseProgram(this->lineShader);
    this->viewLoc = glGetUniformLocation(this->lineShader, "view");
    this->projectionLoc = glGetUniformLocation(this->lineShader, "projection");
    unsigned int lineColorLoc = glGetUniformLocation(this->lineShader, "lineColor");
    glUniformMatrix4fv(      this->viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(this->projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    // glUniform3fv(lineColorLoc, 1, glm::value_ptr(glm::vec3(0.4f, 1.f, 0.4f)));

    for (InterfaceEdgeComponent& c : components.interface_edge_components) {
        const LocationComponent& a = find(components.location_components, c.entity_id);
        const LocationComponent& b = find(components.location_components, c.other_entity_id);
        float rb = 0.2f;
        float g = 0.2f;
        // if (c.glow > 0.1f) {
            // g += (c.glow-0.1f) / 10;
            g += c.glow / 5.f;
            if (g > 1.f) {
                rb += (g - 1.f) / 10;
                rb = std::min(rb, 1.f);
            }
            g = std::min(g, 1.f);
        // }
        c.glow *= 0.8;
        glUniform3fv(lineColorLoc, 1, glm::value_ptr(glm::vec3(rb, g, rb)));
        drawLine(a.x, a.y, 1.f, b.x, b.y, 1.f);
    }

    //  16x16 XY grid at Z=0.
    //
    glUniform3fv(lineColorLoc, 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.2f)));
    for (int x=-16; x<=16; ++x)
        drawLine(x, 16.f, 0.f, x, -16.f, 0.f);
    for (int y=-16; y<=16; ++y)
        drawLine(16.f, y, 0.f, -16.f, y, 0.f);
#endif

    // glBindVertexArray(0); // no need to unbind it every time

    glfwSwapBuffers(window);
    glfwPollEvents();

    display_fps();

    ++this->frames;
}
