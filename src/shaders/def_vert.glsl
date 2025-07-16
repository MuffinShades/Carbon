#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texPos;
layout(location = 2) in vec4 modColor;
layout(location = 3) in float texID;

out vec2 coords;

uniform mat4 proj_mat;

void main() {
    gl_Position = vec4(pos, 1.0) * proj_mat;
    coords = texPos;
}