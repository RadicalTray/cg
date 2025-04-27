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

std::optional<GLuint> textureInit(const std::string& filename, int32_t* p_width, int32_t* p_height);
void textureDeinit(GLuint* texture);
std::optional<RenderTarget> renderTargetInit(int32_t width, int32_t height);
void renderTargetDeinit(RenderTarget* p_render_target);
std::optional<Shaders> shadersInit();
std::optional<GLuint> compileShader(const ShaderCodes shader_codes);
void shadersDeinit(Shaders* shaders);
std::optional<Buffer> bufferInit();
void bufferDeinit(Buffer* buffer);

std::optional<Resources> resourcesInit(Config config) {
    int32_t width = 0;
    int32_t height = 0;
    auto texture = textureInit(config.picture, &width, &height);
    if (!texture) return std::nullopt;

    auto shaders = shadersInit();
    if (!shaders) {
        textureDeinit(&texture.value());
        return std::nullopt;
    }

    auto buffer = bufferInit();
    if (!buffer) {
        shadersDeinit(&shaders.value());
        textureDeinit(&texture.value());
        return std::nullopt;
    }

    auto render_target = renderTargetInit(width, height);
    if (!render_target) {
        bufferDeinit(&buffer.value());
        shadersDeinit(&shaders.value());
        textureDeinit(&texture.value());
        return std::nullopt;
    }

    return Resources{
        .shaders = shaders.value(),
        .buffer = buffer.value(),
        .texture = texture.value(),
        .render_target = render_target.value(),
    };
}

void resourcesDeinit(Resources* resources) {
    bufferDeinit(&resources->buffer);
    shadersDeinit(&resources->shaders);
    textureDeinit(&resources->texture);
    renderTargetDeinit(&resources->render_target);
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
    unsigned char *image = stbi_load(filename.c_str(), &x, &y, &n, 3);
    if (image == NULL) {
        std::println("ERR: Failed to load image \"{}\": {}", filename, stbi_failure_reason());
        return std::nullopt;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    *p_width = x;
    *p_height = y;

    stbi_image_free(image);
    return texture;
}

void textureDeinit(GLuint* texture) {
    glDeleteTextures(1, texture);
}

std::optional<RenderTarget> renderTargetInit(int32_t width, int32_t height) {
    GLuint render_framebuffer = 0;
    glGenFramebuffers(1, &render_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, render_framebuffer);

    GLuint render_texture = 0;
    glGenTextures(1, &render_texture);
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
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
}

std::optional<Shaders> shadersInit() {
    const ShaderCodes texture_shader_codes = {
        .vert =
        "#version 430 core\n"
        ""
        "layout (location = 0) in vec3 in_pos;"
        "layout (location = 1) in vec2 in_uv;"
        ""
        "layout (location = 0) out vec2 out_uv;"
        ""
        "void main() {"
            "gl_Position = vec4(in_pos, 1.0);"
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
        "layout (location = 0) in vec3 in_pos;"
        "layout (location = 1) in vec4 in_color;"
        ""
        "layout (location = 0) out vec4 out_color;"
        ""
        "layout (location = 0) uniform vec2 scaler;"
        ""
        "void main() {"
            "gl_Position = vec4(scaler, 1.0, 1.0) * vec4(in_pos, 1.0);"
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
        "layout (location = 0) in vec3 in_pos;"
        "layout (location = 1) in vec2 in_uv;"
        ""
        "layout (location = 0) out vec2 out_uv;"
        ""
        "layout (location = 0) uniform vec2 scaler;"
        ""
        "void main() {"
            "gl_Position = vec4(scaler, 1.0, 1.0) * vec4(in_pos, 1.0);"
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

void shadersDeinit(Shaders* shaders) {
    glDeleteProgram(shaders->texture);
    glDeleteProgram(shaders->rain);
    shaders->texture = 0;
    shaders->rain = 0;
}

// currently no error checking
std::optional<Buffer> bufferInit() {
    const Vertex vertices[] = {
        // whole framebuffer
        {{ 1.0f,  1.0f, 0.0f}, {1.0, 1.0}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0, 0.0}},
        {{-1.0f, -1.0f, 0.0f}, {0.0, 0.0}},
        {{-1.0f,  1.0f, 0.0f}, {0.0, 1.0}},

        {{ 0.5f,  0.5f, 0.0f}, {1.0, 1.0}},
        {{ 0.5f, -0.5f, 0.0f}, {1.0, 0.0}},
        {{-0.5f, -0.5f, 0.0f}, {0.0, 0.0}},
        {{-0.5f,  0.5f, 0.0f}, {0.0, 1.0}},
    };
    const GLuint indices[] = {
        0, 1, 3,
        1, 2, 3,

        4, 5, 7,
        5, 6, 7,
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

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
