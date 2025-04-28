#include "resources.h"

#include <glad/gl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <optional>
#include <print>

struct ShaderCodes {
    const GLchar* vert;
    const GLchar* frag;
};

void initRainArrays(
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
std::optional<GLuint> textureInit(const std::string& filename, int32_t* p_width, int32_t* p_height);
void textureDeinit(GLuint* p_texture);
std::optional<RenderTarget> renderTargetInit(int32_t width, int32_t height);
void renderTargetDeinit(RenderTarget* p_render_target);
std::optional<Shaders> shadersInit();
std::optional<GLuint> compileShader(const ShaderCodes shader_codes);
void shadersDeinit(Shaders* p_shaders);
std::optional<Buffers> bufferInit(const float width, const float height, const RainVertex* rain_vertices, const GLuint* rain_indices);
void initTextureVertexArray(const float width, const float height, const GLuint va, const GLuint vb, const GLuint eb);
void initRainVertexArray(const GLuint va, const GLuint vb, const GLuint eb, const RainVertex* vertices, const GLuint* indices);
void bufferDeinit(Buffers* p_buffer);

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
        dis,
        gen,
        width, height,
        0.01, 0.16,
        {config.color[0], config.color[1], config.color[2]},
        {config.color[3], config.color[4]}
    );

    auto shaders = shadersInit();
    if (!shaders) {
        textureDeinit(&texture.value());
        return std::nullopt;
    }
    resources.shaders = shaders.value();

    auto buffers = bufferInit(width, height, resources.rain_vertices, resources.rain_indices);
    if (!buffers) {
        shadersDeinit(&shaders.value());
        textureDeinit(&texture.value());
        return std::nullopt;
    }
    resources.buffers = buffers.value();

    auto render_target = renderTargetInit(width, height);
    if (!render_target) {
        bufferDeinit(&buffers.value());
        shadersDeinit(&shaders.value());
        textureDeinit(&texture.value());
        return std::nullopt;
    }
    resources.render_target = render_target.value();

    return resources;
}

void initRainArrays(
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
        RainQuad quad = RainQuad::init(rain_width*tex_height/tex_width, rain_height, color, alpha_top_bot);
        quad.setPosY(0, dis(gen));
        quad.randomPosX(0, dis, gen);

        for (size_t j = 0; j < 4; j++) vertices[i+j] = quad.v[j];
    }
    for (size_t i = 0; i < RAIN_PARTICLES_COUNT; i++) {
        GLuint arr[] = { 0, 1, 3, 1, 2, 3 };
        for (size_t j = 0; j < 6; j++) { (indices + 6*i)[j] = arr[j] + 4*i; }
    }
}

void resourcesDeinit(Resources* p_resources) {
    bufferDeinit(&p_resources->buffers);
    shadersDeinit(&p_resources->shaders);
    textureDeinit(&p_resources->texture);
    renderTargetDeinit(&p_resources->render_target);
}

std::optional<GLuint> textureInit(const std::string& filename, int32_t* p_width, int32_t* p_height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int32_t x, y, n;
    unsigned char *image = stbi_load(filename.c_str(), &x, &y, &n, 4);
    if (image == NULL) {
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

void textureDeinit(GLuint* p_texture) {
    glDeleteTextures(1, p_texture);
    *p_texture = 0;
}

std::optional<RenderTarget> renderTargetInit(int32_t width, int32_t height) {
    GLuint render_framebuffer = 0;
    glGenFramebuffers(1, &render_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, render_framebuffer);

    GLuint render_texture = 0;
    glGenTextures(1, &render_texture);
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_texture, 0);
    GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        std::println("ERR: framebuffer creation failed ({})", status);
        return std::nullopt;
    }

    return RenderTarget{
        .framebuffer = render_framebuffer,
        .texture = render_texture,
        .width = width,
        .height = height,
    };
}

void renderTargetDeinit(RenderTarget* p_render_target) {
    glDeleteTextures(1, &p_render_target->texture);
    glDeleteFramebuffers(1, &p_render_target->framebuffer);

    p_render_target->texture = 0;
    p_render_target->framebuffer = 0;
    p_render_target->width = 0;
    p_render_target->height = 0;
}

std::optional<Shaders> shadersInit() {
    const ShaderCodes texture_shader_codes = {
        .vert =
        "#version 430 core\n"
        ""
        "layout (location = 0) in vec2 in_pos;"
        "layout (location = 1) in vec2 in_uv;"
        ""
        "layout (location = 0) out vec2 out_uv;"
        ""
        "void main() {"
            "gl_Position = vec4(in_pos, 0.0, 1.0);"
            "out_uv = in_uv;"
        "}",
        .frag =
        "#version 430 core\n"
        ""
        "layout (location = 0) in vec2 in_uv;"
        ""
        "layout (location = 0) out vec4 frag_color;"
        ""
        "uniform sampler2D sampler;"
        ""
        "void main() {"
            "frag_color = texture(sampler, in_uv);"
        "}",
    };
    const ShaderCodes rain_shader_codes = {
        .vert =
        "#version 430 core\n"
        ""
        "layout (location = 0) in vec2 in_pos;"
        "layout (location = 1) in vec4 in_color;"
        ""
        "layout (location = 0) out vec4 out_color;"
        ""
        "void main() {"
            "gl_Position = vec4(in_pos, 0.0, 1.0);"
            "out_color = in_color;"
        "}",
        .frag =
        "#version 430 core\n"
        ""
        "layout (location = 0) in vec4 in_color;"
        ""
        "layout (location = 0) out vec4 frag_color;"
        ""
        "void main() {"
            "frag_color = in_color;"
        "}",
    };
    const ShaderCodes screen_shader_codes = {
        .vert =
        "#version 430 core\n"
        ""
        "layout (location = 0) in vec2 in_pos;"
        "layout (location = 1) in vec2 in_uv;"
        ""
        "layout (location = 0) out vec2 out_uv;"
        ""
        "layout (location = 0) uniform vec2 scale;"
        "layout (location = 1) uniform vec2 cam_pos;"
        ""
        "void main() {"
            "gl_Position = vec4(scale*in_pos + cam_pos, 0.0, 1.0);"
            "out_uv = in_uv;"
        "}",
        .frag =
        "#version 430 core\n"
        ""
        "layout (location = 0) in vec2 in_uv;"
        ""
        "layout (location = 0) out vec4 frag_color;"
        ""
        "uniform sampler2D sampler;"
        ""
        "void main() {"
            "frag_color = texture(sampler, in_uv);"
        "}",
    };

    auto texture_program = compileShader(texture_shader_codes);
    if (!texture_program) return std::nullopt;

    auto rain_program = compileShader(rain_shader_codes);
    if (!rain_program) return std::nullopt;

    auto screen_program = compileShader(screen_shader_codes);
    if (!rain_program) return std::nullopt;

    return Shaders{
        .texture = texture_program.value(),
        .rain = rain_program.value(),
        .screen = screen_program.value(),
    };
}

std::optional<GLuint> compileShader(const ShaderCodes shader_codes) {
    GLint success;
    GLchar info[512];

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &shader_codes.vert, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info);
        std::println("ERR: Shader vertex compilation failed: {}", info);
        return std::nullopt;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &shader_codes.frag, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info);
        std::println("ERR: Shader fragment compilation failed: {}", info);
        glDeleteShader(vertex_shader);
        return std::nullopt;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, info);
        std::println("ERR: Shader program linking failed: {}", info);
        return std::nullopt;
    }

    return program;
}

void shadersDeinit(Shaders* p_shaders) {
    glDeleteProgram(p_shaders->texture);
    glDeleteProgram(p_shaders->rain);
    p_shaders->texture = 0;
    p_shaders->rain = 0;
}

// currently no error checking
std::optional<Buffers> bufferInit(const float width, const float height, const RainVertex* rain_vertices, const GLuint* rain_indices) {
    GLuint VAs[2] = { 0, 0 };
    GLuint VBs[2] = { 0, 0 };
    GLuint EBs[2] = { 0, 0 };
    glGenVertexArrays(2, VAs);
    glGenBuffers(2, VBs);
    glGenBuffers(2, EBs);

    initTextureVertexArray(width, height, VAs[0], VBs[0], EBs[0]);
    initRainVertexArray(VAs[1], VBs[1], EBs[1], rain_vertices, rain_indices);

    return Buffers{
        .vert_arr = VAs[0],
        .vert_buf = VBs[0],
        .elem_buf = EBs[0],
        .rain_vert_arr = VAs[1],
        .rain_vert_buf = VBs[1],
        .rain_elem_buf = EBs[1],
    };
}

void initTextureVertexArray(const float width, const float height, const GLuint va, const GLuint vb, const GLuint eb) {
    const TextureVertex vertices[] = {
        // whole viewport
        {{ 1.0f,  1.0f}, {1.0, 1.0}},
        {{ 1.0f, -1.0f}, {1.0, 0.0}},
        {{-1.0f, -1.0f}, {0.0, 0.0}},
        {{-1.0f,  1.0f}, {0.0, 1.0}},

        // middle of viewport
        {{ 0.5*width/height,  0.5f}, {1.0, 1.0}},
        {{ 0.5*width/height, -0.5f}, {1.0, 0.0}},
        {{-0.5*width/height, -0.5f}, {0.0, 0.0}},
        {{-0.5*width/height,  0.5f}, {0.0, 1.0}},
    };
    const GLuint indices[] = {
        0, 1, 3,
        1, 2, 3,

        4, 5, 7,
        5, 6, 7,
    };

    glBindVertexArray(va);

    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*)(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*)(offsetof(TextureVertex, uv)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void initRainVertexArray(const GLuint va, const GLuint vb, const GLuint eb, const RainVertex* vertices, const GLuint* indices) {
    glBindVertexArray(va);

    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, RAIN_VERTICES_COUNT*sizeof(RainVertex), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, RAIN_INDICES_COUNT*sizeof(GLuint), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void*)(0));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void*)(offsetof(RainVertex, clr)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void bufferDeinit(Buffers* p_buffer) {
    GLuint VAs[2] = {p_buffer->vert_arr, p_buffer->rain_vert_arr};
    GLuint VBs[2] = {p_buffer->vert_buf, p_buffer->rain_vert_buf};
    GLuint EBs[2] = {p_buffer->elem_buf, p_buffer->rain_elem_buf};
    glDeleteVertexArrays(2, VAs);
    glDeleteBuffers(2, VBs);
    glDeleteBuffers(2, EBs);

    p_buffer->vert_arr = 0;
    p_buffer->vert_buf = 0;
    p_buffer->elem_buf = 0;
    p_buffer->rain_vert_arr = 0;
    p_buffer->rain_vert_buf = 0;
    p_buffer->rain_elem_buf = 0;
}
