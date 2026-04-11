#version 430 core

layout (isolines) in;

out float t;
out vec4 posf;

uniform vec2 output_dim;
out vec2 o_dim;

void main() {
    t = gl_TessCoord.x;

    vec3 p0 = gl_in[0].gl_Position.xyz,
         p1 = gl_in[1].gl_Position.xyz,
         p2 = gl_in[2].gl_Position.xyz;

    o_dim = vec2(184, 1100);

    float i_t = 1.0 - t;

    posf = vec4((i_t * i_t) * p0 + (2.0 * i_t * t) * p1 + (t * t) * p2, 1.0);
    vec4 fposf = vec4(round(posf.x), round(posf.y), posf.zw);
    gl_Position = fposf * vec4(2.0 / o_dim.x, 2.0 / o_dim.y, 1.0, 1.0) - vec4(1.0, 1.0, 0.0, 0.0);
}