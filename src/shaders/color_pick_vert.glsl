#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

out vec3 color;
out vec4 posf;

uniform mat4 proj_mat;
uniform mat4 model_mat;

void main() {
    posf = proj_mat * vec4(pos, 1.0);
    gl_Position = posf;
    color = col;
}