#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_velocity;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

out vec4 frag_color;

void main() {
    vec3 updated_position = a_position + a_velocity * time;

    gl_Position = projection * view * model * vec4(updated_position, 1.0);
    
    frag_color = vec4(0.0, 0.7, 1.0, 1.0);
}
