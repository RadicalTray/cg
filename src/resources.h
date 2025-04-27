#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <print>

struct Config {
    std::string picture;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

struct Shaders {
    GLuint texture;
    GLuint rain;
    GLuint screen;
};

struct Buffer {
    GLuint vert_arr;
    GLuint vert_buf;
    GLuint elem_buf;
};

struct RenderTarget {
    GLuint framebuffer;
    GLuint texture;
    int32_t width;
    int32_t height;
};

struct Resources {
    Shaders shaders;
    Buffer buffer;
    uint32_t texture;
    RenderTarget render_target;
};

std::optional<Resources> resourcesInit(Config config);
void resourcesDeinit(Resources* resources);
