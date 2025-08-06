#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;

out vec2 coords;

uniform mat4 proj_mat;
uniform mat4 cam_mat;
uniform mat4 model_mat;

void main() {
    gl_Position = proj_mat * cam_mat * model_mat * vec4(pos, 1.0);
    coords = tex;
}