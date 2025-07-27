#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texPos;

out vec2 coords;

uniform mat4 proj_mat;
uniform mat4 cam_mat;

void main() {
    gl_Position = vec4(pos, 1.0) * proj_mat * cam_mat;
    coords = texPos;
}