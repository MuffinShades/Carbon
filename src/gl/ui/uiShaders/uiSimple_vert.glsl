#version 430 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 chroma;

uniform mat4 model_mat;

out vec4 frag_col;

void main() {
    frag_col = vec4(chroma, 1.0);
    gl_Position = model_mat * vec4(pos, 1.0);
}