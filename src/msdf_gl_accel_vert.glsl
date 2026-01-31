#version 430 core

layout(location = 0) in vec3 m_pos;
layout(location = 1) in vec2 region_pos;
layout(location = 2) in ivec2 t_curves;

out vec2 posf;
out ivec2 curve_range;

void main() {
    posf = region_pos.xy;
    curve_range = t_curves;
    gl_Position = vec4(m_pos, 1.0);
}