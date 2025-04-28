#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Particle {
    glm::vec2 position;
    float speed;
};

extern std::vector<Particle> particles;
extern unsigned int rainVAO, rainVBO;
extern unsigned int rainShaderProgram;

void initParticles();
void updateParticles(float deltaTime);
void initRainShader();
void updateRainBuffer();
void draw();
void rainDeinit();