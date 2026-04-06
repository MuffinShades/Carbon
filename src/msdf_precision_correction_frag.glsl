#version 430 core

struct Curve {
    vec2 p0;
    int padd0[2];
    vec2 p1;
    int padd1[2];
    vec2 p2;
    int padd2[2];
    vec3 chroma;
    int padd3;
    vec4 compute_base;
};

layout (std430, binding = 0) buffer GlyphCurves {
    Curve glyph_curves[];
};

in flat int tCurve;
in vec2 txp;
in vec4 posf;
in float t;

uniform usampler2D f_tex;
in vec2 o_dim;

//out uvec4 FragColor;
out vec4 FragColor;

void main() {
    const float i255 = 1.0 / 255.0;

    const float dx = (posf.x - floor(posf.x)) / o_dim.x;
    const float dy = (posf.y - floor(posf.y)) / o_dim.y;

    FragColor = vec4(dx, dy, 1.0, 1.0);
}