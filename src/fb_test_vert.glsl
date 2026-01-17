#version 430 core

layout(location = 0) in vec3 posf;

out vec3 pos;

void main() {
    pos = posf;
    gl_Position = vec4(posf, 1.0);
}