#include <stdfloat>
#include <print>
#include <optional>

#include "window.h"

using std::float32_t;

struct Shaders {
    uint32_t main;
};

struct Buffer {
    uint32_t vert_arr;
    uint32_t vert_buf;
    uint32_t elem_buf;
};

struct Resources {
    Shaders shaders;
    Buffer buffer;

    void doStuff() {
        glUseProgram(shaders.main);
        glBindVertexArray(buffer.vert_arr);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
};

std::optional<Shaders> shadersInit();
void shadersDeinit(Shaders* shaders);
std::optional<Resources> resourcesInit();
void resourcesDeinit(Resources* resources);
std::optional<Buffer> bufferInit();
void bufferDeinit(Buffer* buffer);

// GOAL: render "rain"
// TODO:
//  - [ ] Render texture
//  - [ ] Mouse movement
//  - [ ] "Rain"

const char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;"
    "void main() {"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);"
    "}";
const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;"
    "void main() {"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);"
    "}";

int main() {
    GLFWwindow* window = windowInit();
    if (window == nullptr) return -1;

    auto resource_init_result = resourcesInit();
    if (!resource_init_result) return -1;

    Resources resources = resource_init_result.value();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        resources.doStuff();

        glfwSwapBuffers(window);
    }

    resourcesDeinit(&resources);
    windowDeinit(&window);
}

std::optional<Resources> resourcesInit() {
    auto shaders = shadersInit();
    if (!shaders) return std::nullopt;

    auto buffer = bufferInit();
    if (!buffer) return std::nullopt;

    return Resources{
        .shaders = shaders.value(),
        .buffer = buffer.value(),
    };
}

void resourcesDeinit(Resources* resources) {
    shadersDeinit(&resources->shaders);
    bufferDeinit(&resources->buffer);
}

std::optional<Shaders> shadersInit() {
    int32_t success;
    char infoLog[512];

    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::println("ERR: Shader vertex compilation failed: {}", infoLog);
        return std::nullopt;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::println("ERR: Shader fragment compilation failed: {}", infoLog);
        return std::nullopt;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::println("ERR: Shader program linking failed: {}", infoLog);
        return std::nullopt;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return Shaders{
        .main = shaderProgram,
    };
}

void shadersDeinit(Shaders* shaders) {
    glDeleteProgram(shaders->main);
    shaders->main = 0;
}

// currently no error checking
std::optional<Buffer> bufferInit() {
    const float32_t vertices[] = {
         0.5f,  0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
    };
    const uint32_t indices[] = {
        0, 1, 3,
        1, 2, 3,
    };

    uint32_t VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return Buffer{
        .vert_arr = VAO,
        .vert_buf = VBO,
        .elem_buf = EBO,
    };
}

void bufferDeinit(Buffer* buffer) {
    glDeleteVertexArrays(1, &buffer->vert_arr);
    glDeleteBuffers(1, &buffer->vert_buf);
    glDeleteBuffers(1, &buffer->elem_buf);
}
