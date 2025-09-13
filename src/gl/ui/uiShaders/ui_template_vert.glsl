/* Vertex template for all ui shaders */
#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

out vec3 color;
out vec4 posf;

uniform mat4 ui_proj;
uniform mat4 ui_model;

void main() {
    posf = ui_proj * ui_model * vec4(pos, 1.0);
    gl_Position = posf;
    color = col;
}