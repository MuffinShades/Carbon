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

uniform usampler2D f_tex;

//out uvec4 FragColor;
out vec4 FragColor;

void main() {
    const float i255 = 1.0 / 255.0;

    //uvec4 prev_samp = texture(f_tex, txp);

    //FragColor = uvec4(65535, 65535, 65535, 65535);

    FragColor = vec4(1.0, 1.0, 1.0, 1.0);

    //if (prev_samp.x > 0)
        //FragColor = uvec4(65535, 0, 65535, 65535);
    //else
        //FragColor = uvec4(65535, 0, 0, 65535);
}