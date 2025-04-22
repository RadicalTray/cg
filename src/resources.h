#include <glad/gl.h>

#include <optional>

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
};

std::optional<Shaders> shadersInit();
void shadersDeinit(Shaders* shaders);
std::optional<Buffer> bufferInit();
void bufferDeinit(Buffer* buffer);
std::optional<Resources> resourcesInit();
void resourcesDeinit(Resources* resources);
