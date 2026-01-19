#version 430 core

struct Curve {
    vec2 p0, p1, p2;
    vec3 chroma;
    vec4 compute_base;
};


layout (std140, binding = 0) buffer GlyphCurves {
    Curve glyph_curves[];
};

uniform int nCurves;
uniform vec4 padding;
uniform vec4 region;
uniform vec4 glyphDim;

out vec4 FragColor;

in vec2 posf;

void main() {
    FragColor = vec4(posf, 0.0, 1.0);
}