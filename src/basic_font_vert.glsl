#version 430 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 msdf_tex;

out vec2 posf;
out vec2 texp;

void main() {
    posf = pos;
    texp = msdf_tex;
    gl_Position = vec4(pos, 1.0);
}