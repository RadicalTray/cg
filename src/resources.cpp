#include "resources.h"

#include <glad/gl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <optional>
#include <print>
#include <fstream>
#include <sstream>
#include <string>

// -----------------------------------------------------------------------------
// INTERNAL STRUCTS

struct ShaderCodes {
    const GLchar* vert;
    const GLchar* frag;
};

// -----------------------------------------------------------------------------
// FORWARD DECLARE FUNCTIONS (Static functions)

static void initRainArrays(
    RainVertex* vertices,
    GLuint* indices,
    std::uniform_real_distribution<>& dis,
    std::mt19937& gen,
    const float tex_width,
    const float tex_height,
    const float rain_width,
    const float rain_height,
    const glm::vec3 color,
    const glm::vec2 alpha_top_bot
);

static std::optional<GLuint> textureInit(const std::string& filename, int32_t* p_width, int32_t* p_height);
static void textureDeinit(GLuint* p_texture);

static std::optional<RenderTarget> renderTargetInit(int32_t width, int32_t height);
static void renderTargetDeinit(RenderTarget* p_render_target);

static std::optional<Shaders> shadersInit();
static std::optional<GLuint> compileShader(const ShaderCodes shader_codes);
static void shadersDeinit(Shaders* p_shaders);

static std::optional<Buffers> bufferInit(const float width, const float height, const RainVertex* rain_vertices, const GLuint* rain_indices);
static void initTextureVertexArray(const float width, const float height, const GLuint va, const GLuint vb, const GLuint eb);
static void initRainVertexArray(const GLuint va, const GLuint vb, const GLuint eb, const RainVertex* vertices, const GLuint* indices);
static void bufferDeinit(Buffers* p_buffer);

static std::string readShaderFile(const char* filePath);

// -----------------------------------------------------------------------------
// RESOURCES API

std::optional<Resources> resourcesInit(Config config, std::uniform_real_distribution<>& dis, std::mt19937& gen) {
    Resources resources = {};

    int32_t width = 0;
    int32_t height = 0;
    auto texture = textureInit(config.picture, &width, &height);
    if (!texture) return std::nullopt;
    resources.texture = texture.value();

    initRainArrays(
        resources.rain_vertices,
        resources.rain_indices,
        dis, gen,
        width, height,
        0.01f, 0.16f,
        {config.color[0], config.color[1], config.color[2]},
        {config.color[3], config.color[4]}
    );

    auto shaders = shadersInit();
    if (!shaders) return std::nullopt;
    resources.shaders = shaders.value();

    auto buffers = bufferInit(width, height, resources.rain_vertices, resources.rain_indices);
    if (!buffers) return std::nullopt;
    resources.buffers = buffers.value();

    auto render_target = renderTargetInit(width, height);
    if (!render_target) return std::nullopt;
    resources.render_target = render_target.value();

    auto droplet_render_target = renderTargetInit(width, height);
    if (!droplet_render_target) return std::nullopt;
    resources.droplet_render_target = droplet_render_target.value();

    return resources;
}

void resourcesDeinit(Resources* p_resources) {
    bufferDeinit(&p_resources->buffers);
    shadersDeinit(&p_resources->shaders);
    textureDeinit(&p_resources->texture);
    renderTargetDeinit(&p_resources->render_target);
    renderTargetDeinit(&p_resources->droplet_render_target);
}

// -----------------------------------------------------------------------------
// INIT HELPER FUNCTIONS

static void initRainArrays(
    RainVertex* vertices,
    GLuint* indices,
    std::uniform_real_distribution<>& dis,
    std::mt19937& gen,
    const float tex_width,
    const float tex_height,
    const float rain_width,
    const float rain_height,
    const glm::vec3 color,
    const glm::vec2 alpha_top_bot
) {
    for (size_t i = 0; i < RAIN_VERTICES_COUNT; i += 4) {
        RainQuad quad = RainQuad::init(rain_width * tex_height / tex_width, rain_height, color, alpha_top_bot);
        quad.setPosY(0, dis(gen));
        quad.randomPosX(0, dis, gen);

        for (size_t j = 0; j < 4; j++)
            vertices[i + j] = quad.v[j];
    }
    for (size_t i = 0; i < RAIN_PARTICLES_COUNT; i++) {
        GLuint arr[] = { 0, 1, 3, 1, 2, 3 };
        for (size_t j = 0; j < 6; j++)
            indices[6 * i + j] = arr[j] + 4 * i;
    }
}

// -----------------------------------------------------------------------------
// TEXTURE

static std::optional<GLuint> textureInit(const std::string& filename, int32_t* p_width, int32_t* p_height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int32_t x, y, n;
    unsigned char* image = stbi_load(filename.c_str(), &x, &y, &n, 4);
    if (!image) {
        std::println("ERR: Failed to load image \"{}\": {}", filename, stbi_failure_reason());
        return std::nullopt;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    *p_width = x;
    *p_height = y;
    stbi_image_free(image);

    return texture;
}

static void textureDeinit(GLuint* p_texture) {
    if (*p_texture) {
        glDeleteTextures(1, p_texture);
        *p_texture = 0;
    }
}

// -----------------------------------------------------------------------------
// FRAMEBUFFER

static std::optional<RenderTarget> renderTargetInit(int32_t width, int32_t height) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::println("ERR: framebuffer creation failed ({})", status);
        return std::nullopt;
    }

    return RenderTarget{framebuffer, texture, width, height};
}

static void renderTargetDeinit(RenderTarget* p_render_target) {
    if (p_render_target->texture)
        glDeleteTextures(1, &p_render_target->texture);
    if (p_render_target->framebuffer)
        glDeleteFramebuffers(1, &p_render_target->framebuffer);

    *p_render_target = {};
}

// -----------------------------------------------------------------------------
// SHADERS

static std::string readShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static std::optional<Shaders> shadersInit() {
    auto vert_code = readShaderFile("src/dropletsVertex.h");
    auto frag_code = readShaderFile("src/droplets.h");

    if (vert_code.empty() || frag_code.empty()) {
        std::println("ERR: Failed to read shader files");
        return std::nullopt;
    }

    ShaderCodes droplet_codes{ vert_code.c_str(), frag_code.c_str() };

    ShaderCodes texture_codes{ 
        R"(#version 430 core
layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 0) out vec2 out_uv;
void main() { gl_Position = vec4(in_pos, 0.0, 1.0); out_uv = in_uv; }
)",
        R"(#version 430 core
layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 frag_color;
uniform sampler2D sampler;
void main() { frag_color = texture(sampler, in_uv); }
)"
    };

    ShaderCodes rain_codes = { /* similar */ };
    ShaderCodes screen_codes = { /* similar */ };

    auto droplet_prog = compileShader(droplet_codes);
    auto texture_prog = compileShader(texture_codes);
    auto rain_prog = compileShader(rain_codes);
    auto screen_prog = compileShader(screen_codes);

    if (!droplet_prog || !texture_prog || !rain_prog || !screen_prog) {
        return std::nullopt;
    }

    return Shaders{texture_prog.value(), rain_prog.value(), screen_prog.value(), droplet_prog.value()};
}

static std::optional<GLuint> compileShader(const ShaderCodes shader_codes) {
    GLint success;
    GLchar info[512];

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader_codes.vert, nullptr);
    glCompileShader(vs);

    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, nullptr, info);
        std::println("ERR: Vertex Shader compilation error: {}", info);
        glDeleteShader(vs);
        return std::nullopt;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader_codes.frag, nullptr);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, nullptr, info);
        std::println("ERR: Fragment Shader compilation error: {}", info);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return std::nullopt;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, info);
        std::println("ERR: Shader program linking error: {}", info);
        glDeleteProgram(program);
        return std::nullopt;
    }

    return program;
}

static void shadersDeinit(Shaders* p_shaders) {
    if (p_shaders->texture) glDeleteProgram(p_shaders->texture);
    if (p_shaders->rain) glDeleteProgram(p_shaders->rain);
    if (p_shaders->screen) glDeleteProgram(p_shaders->screen);
    if (p_shaders->droplet) glDeleteProgram(p_shaders->droplet);

    *p_shaders = {};
}

// -----------------------------------------------------------------------------
// BUFFERS

static std::optional<Buffers> bufferInit(const float width, const float height, const RainVertex* rain_vertices, const GLuint* rain_indices) {
    GLuint VAs[2], VBs[2], EBs[2];
    glGenVertexArrays(2, VAs);
    glGenBuffers(2, VBs);
    glGenBuffers(2, EBs);

    initTextureVertexArray(width, height, VAs[0], VBs[0], EBs[0]);
    initRainVertexArray(VAs[1], VBs[1], EBs[1], rain_vertices, rain_indices);

    return Buffers{VAs[0], VBs[0], EBs[0], VAs[1], VBs[1], EBs[1]};
}

static void initTextureVertexArray(const float width, const float height, const GLuint va, const GLuint vb, const GLuint eb) {
    const TextureVertex vertices[] = { /* your vertex list */ };
    const GLuint indices[] = { /* your indices */ };

    glBindVertexArray(va);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*)offsetof(TextureVertex, pos));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*)offsetof(TextureVertex, uv));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

static void initRainVertexArray(const GLuint va, const GLuint vb, const GLuint eb, const RainVertex* vertices, const GLuint* indices) {
    glBindVertexArray(va);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, RAIN_VERTICES_COUNT * sizeof(RainVertex), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, RAIN_INDICES_COUNT * sizeof(GLuint), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void*)offsetof(RainVertex, pos));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void*)offsetof(RainVertex, clr));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

static void bufferDeinit(Buffers* p_buffer) {
    glDeleteVertexArrays(1, &p_buffer->vert_arr);
    glDeleteBuffers(1, &p_buffer->vert_buf);
    glDeleteBuffers(1, &p_buffer->elem_buf);

    glDeleteVertexArrays(1, &p_buffer->rain_vert_arr);
    glDeleteBuffers(1, &p_buffer->rain_vert_buf);
    glDeleteBuffers(1, &p_buffer->rain_elem_buf);

    *p_buffer = {};
}