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
in float t;

uniform sampler2D f_tex;

out uvec4 FragColor;

void main() {
    const f32 i255 = 1.0 / 255.0;

    vec4 prev_samp = texture(f_tex, txp);

    
}