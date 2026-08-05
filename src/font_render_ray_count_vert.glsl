#version 430 core

layout(location = 0) in vec3 m_pos;
layout(location = 1) in vec2 region_pos;
layout(location = 2) in ivec2 t_curves;
layout(location = 3) in vec2 del;

out vec2 poss;
out flat ivec2 curve_range;
out vec2 delta;

uniform mat4 screen_project;

void main() {
    poss = region_pos;
    delta = del;
    curve_range = t_curves;
    gl_Position = screen_project * vec4(m_pos, 1.0);
}