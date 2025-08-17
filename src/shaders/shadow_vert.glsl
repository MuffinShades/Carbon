#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform mat4 lightLook;
uniform mat4 lightProj;
uniform mat4 model_mat;

void main() {
    gl_Position = lightProj * lightLook * model_mat * vec4(pos, 1.0);
}