#include <glad/gl.h>
#include <glm/glm.hpp>

#include <optional>
#include <string>

#define RAIN_PARTICLES_COUNT 1024
#define RAIN_VERTICES_COUNT 4 * RAIN_PARTICLES_COUNT
#define RAIN_INDICES_COUNT 6 * RAIN_PARTICLES_COUNT

struct Config {
    std::string picture;
};

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 uv;
};

struct RainVertex {
    glm::vec2 pos;
    glm::vec4 clr;
};

struct Shaders {
    GLuint texture;
    GLuint rain;
    GLuint screen;
};

struct Buffers {
    GLuint vert_arr;
    GLuint vert_buf;
    GLuint elem_buf;
    GLuint rain_vert_arr;
    GLuint rain_vert_buf;
    GLuint rain_elem_buf;
};

struct RenderTarget {
    GLuint framebuffer;
    GLuint texture;
    int32_t width;
    int32_t height;
};

struct Resources {
    Shaders shaders;
    Buffers buffers;
    GLuint texture;
    RenderTarget render_target;
    RainVertex rain_vertices[RAIN_VERTICES_COUNT];
    GLuint rain_indices[RAIN_INDICES_COUNT];
};

std::optional<Resources> resourcesInit(Config config);
void resourcesDeinit(Resources* p_resources);
