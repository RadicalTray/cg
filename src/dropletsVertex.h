#version 430 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragCoord;

void main() {
    gl_Position = vec4(inPos, 1.0);
    fragCoord = inPos.xy * 0.5 + 0.5; // Convert to [0,1] range
}