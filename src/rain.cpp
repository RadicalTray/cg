#include "rain.h"
#include <glad/glad.h>
#include <cstdlib>
#include "shader_utils.h"

std::vector<Particle> particles;
GLuint rainVAO = 0, rainVBO = 0;
GLuint rainShaderProgram = 0;

void initParticles() {
    int rainCount = 500;
    particles.clear();
    for (int i = 0; i < rainCount; ++i) {
        particles.push_back({
            glm::vec2(
                (float(rand()) / RAND_MAX) * 2.0f - 1.0f,
                (float(rand()) / RAND_MAX) * 2.0f
            ),
            0.5f + float(rand()) / RAND_MAX * 0.5f
        });
    }
}

void updateParticles(float deltaTime) {
    for (auto& p : particles) {
        p.position.y -= p.speed * deltaTime;
        if (p.position.y < -1.0f) {
            p.position.y = 1.0f + float(rand()) / RAND_MAX;
            p.position.x = (float(rand()) / RAND_MAX) * 2.0f - 1.0f;
        }
    }
}

void initRainShader() {
    rainShaderProgram = loadShaderProgram("shaders/rain.vert", "shaders/rain.frag");

    glGenVertexArrays(1, &rainVAO);
    glGenBuffers(1, &rainVBO);

    glBindVertexArray(rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);

    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void updateRainBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());
}

void draw() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(rainShaderProgram);


    glBindVertexArray(rainVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, particles.size() * 2);
}

void rainDeinit() {
    glDeleteVertexArrays(1, &rainVAO);
    glDeleteBuffers(1, &rainVBO);
    glDeleteProgram(rainShaderProgram);
}
