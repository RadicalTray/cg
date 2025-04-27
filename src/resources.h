#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <optional>
#include <string>
#include <print>

struct Config {
    std::string picture;
};

struct Shaders {
    uint32_t texture;
    uint32_t rain;

    void useTexture() const {
        glUseProgram(texture);
    }

    void useRain() const {
        glUseProgram(rain);
    }

    void updateScaler(GLFWwindow* window) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        const float ratio = float(height)/float(width);
        glUniform2f(0, 1, 1/ratio);
    }
};

struct Buffer {
    uint32_t vert_arr;
    uint32_t vert_buf;
    uint32_t elem_buf;
};

struct Resources {
    Shaders shaders;
    Buffer buffer;
    uint32_t texture;
};

std::optional<Resources> resourcesInit(Config config);
void resourcesDeinit(Resources* resources);
