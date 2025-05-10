#version 330 core
layout(location = 0) in vec2 aPos;

void main() {
    if (gl_VertexID % 2 == 0) {
        gl_Position = vec4(aPos, 0.0, 1.0);
    } else {
        gl_Position = vec4(aPos.x, aPos.y - 0.05, 0.0, 1.0);
    }
}