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

    const vec3 k = vec3(0.0, 0.0, 1.0);

    vec3 D = 2.0 * i_t * (p1 - p0) + 2.0 * t * (p2 - p1);
    D.z = 0;

    vec3 floor_pos = vec3(floor(posf.x), floor(posf.y), 0.0);
    const vec3 mid = floor_pos + vec3(0.5,0.5,0.0);
    const float phi_Fac = dot(cross(D, k), (vec3(posf.xy, 0.0) - mid));

    vec2 adj = vec2(0.0, 0.0);

    if (phi_Fac < 0.0) {
        float six = sign(posf.x - mid.x), siy = sign(posf.y - mid.y);
        if (six == siy) {
            adj.y = -siy;
        } else {
            adj.x = -six;
            adj.y = -siy;
        }
    }

    vec4 fposf = vec4(floor_pos.xy + adj, posf.zw);
    gl_Position = fposf * vec4(2.0 / o_dim.x, 2.0 / o_dim.y, 1.0, 1.0) - vec4(1.0, 1.0, 0.0, 0.0);
}