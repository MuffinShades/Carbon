#version 430 core

layout(location = 0) in vec3 m_pos;

out vec2 posf;

void main() {
    posf = m_pos.xy;
    gl_Position = vec4(m_pos, 1.0);
}