#version 430 core

layout (isolines) in;

out float t;

void main() {
    t = gl_TessCoord.x;

    vec3 p0 = gl_in[0].gl_Position.xyz,
         p1 = gl_in[1].gl_Position.xyz,
         p2 = gl_in[2].gl_Position.xyz;

    float i_t = 1.0 - t;

    gl_Position = vec4((i_t * i_t) * p0 + (2.0 * i_t * t) * p1 + (t * t) * p2, 1.0);
}