#include <glad/gl.h>

#include <optional>
#include <string>

struct Config {
    std::string picture;
};

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
    uint32_t texture;
};

std::optional<Resources> resourcesInit(Config config);
void resourcesDeinit(Resources* resources);
