#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex;

out vec2 coords;
out vec4 posf;
out vec3 worldPos;
out vec3 n;
out vec3 camPos;

uniform vec3 cam_pos;

uniform mat4 proj_mat;
uniform mat4 cam_mat;
uniform mat4 model_mat;

void main() {
    worldPos = pos;
    posf = proj_mat * cam_mat * model_mat * vec4(pos, 1.0);
    gl_Position = posf;
    n = normalize(normal);
    coords = tex;
    camPos = cam_pos;
}