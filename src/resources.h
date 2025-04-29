#include <glad/gl.h>
#include <glm/glm.hpp>

#include <optional>
#include <random>
#include <string>

#define RAIN_PARTICLES_COUNT 8192
#define RAIN_VERTICES_COUNT 4 * RAIN_PARTICLES_COUNT
#define RAIN_INDICES_COUNT 6 * RAIN_PARTICLES_COUNT

struct Config {
    std::string picture;
    uint32_t rain_count;
    float speed;
    glm::vec3 rgb;
    float color[5]; // R G B A_top A_bot
};

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 uv;
};

struct RainVertex {
    glm::vec2 pos;
    glm::vec4 clr;
};

struct RainQuad {
    RainVertex v[4];

    void randomPosX(const size_t idx, std::uniform_real_distribution<>& dis, std::mt19937& gen) {
        setPosX(idx, dis(gen));
    }

    void setPosX(const size_t idx, const float x) {
        const float dx = x - v[idx].pos.x;
        translatePosX(dx);
    }

    void translatePosX(const float dx) {
        for (int i = 0; i < 4; i++) v[i].pos.x += dx;
    }

    void setPosY(const size_t idx, const float y) {
        const float dy = y - v[idx].pos.y;
        translatePosY(dy);
    }

    void translatePosY(const float dy) {
        for (int i = 0; i < 4; i++) v[i].pos.y += dy;
    }

    float height() {
        return v[0].pos.y - v[1].pos.y;
    }

    static RainQuad init(const float width, const float height, const glm::vec3 rgb, const glm::vec2 alpha_top_bot) {
        return RainQuad{
            .v = {
                {{ width/2.0,  height/2.0}, {rgb, alpha_top_bot.x}},
                {{ width/2.0, -height/2.0}, {rgb, alpha_top_bot.y}},
                {{-width/2.0, -height/2.0}, {rgb, alpha_top_bot.y}},
                {{-width/2.0,  height/2.0}, {rgb, alpha_top_bot.x}},
            },
        };
    }
};

struct Shaders {
    GLuint texture;
    GLuint rain;
    GLuint screen;
    GLuint droplet;
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
    RenderTarget droplet_render_target;
    RainVertex rain_vertices[RAIN_VERTICES_COUNT];
};

std::optional<Resources> resourcesInit(Config config, std::uniform_real_distribution<>& dis, std::mt19937& gen);
void resourcesDeinit(Resources* p_resources);
