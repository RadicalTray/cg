#include "resources.h"

#include <glad/gl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <optional>
#include <print>
#include <glm/glm.hpp>

#include <fstream>
#include <sstream>
#include <string>

std::optional<uint32_t> textureInit(const std::string& filename);
void textureDeinit(uint32_t* texture);
std::optional<Shaders> shadersInit();
void shadersDeinit(Shaders* shaders);
std::optional<Buffer> bufferInit();
void bufferDeinit(Buffer* buffer);

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

std::optional<Resources> resourcesInit(Config config) {
    auto texture = textureInit(config.picture);
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

    return Resources{
        .shaders = shaders.value(),
        .buffer = buffer.value(),
        .texture = texture.value(),
    };
}

void resourcesDeinit(Resources* resources) {
    bufferDeinit(&resources->buffer);
    shadersDeinit(&resources->shaders);
    textureDeinit(&resources->texture);
}

std::optional<uint32_t> textureInit(const std::string& filename) {
    uint32_t texture;
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

    stbi_image_free(image);
    return texture;
}

std::string readShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void textureDeinit(uint32_t* texture) {
    glDeleteTextures(1, texture);
}

std::optional<Shaders> shadersInit() {
    std::string vertexShaderCode = readShaderFile("src/dropletsVertex.h");
	if (vertexShaderCode.empty()) {
		std::println("ERR: Failed to read shader file");
		return std::nullopt;
	}
	const char* vertexShaderSource = vertexShaderCode.c_str();
        /*"#version 430 core\n"
        ""
        "layout (location = 0) in vec3 inPos;"
        "layout (location = 1) in vec2 inUV;"
        ""
        "layout (location = 0) out vec2 outUV;"
        ""
        "void main() {"
        "   gl_Position = vec4(inPos.x, inPos.y, inPos.z, 1.0);"
        "   outUV = inUV;"
        "}";*/
	std::string fragmentShaderCode = readShaderFile("src/droplets.h");
	if (fragmentShaderCode.empty()) {
		std::println("ERR: Failed to read shader file");
		return std::nullopt;
	}
	const char* fragmentShaderSource = fragmentShaderCode.c_str();
        /*"#version 430 core\n"
        ""
        "layout (location = 0) in vec2 inUV;"
        ""
        "layout (location = 0) out vec4 FragColor;"
        ""
        "uniform sampler2D sampler;"
        ""
        "void main() {"
        "   FragColor = texture(sampler, inUV);"
        "}";*/

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
    const Vertex vertices[] = {
        {{ 0.5f,  0.5f, 0.0f}, {1.0, 1.0}},
        {{ 0.5f, -0.5f, 0.0f}, {1.0, 0.0}},
        {{-0.5f, -0.5f, 0.0f}, {0.0, 0.0}},
        {{-0.5f,  0.5f, 0.0f}, {0.0, 1.0}},
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
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
