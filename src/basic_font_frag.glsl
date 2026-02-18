#version 430 core

/*

Basic font renderer

*/

uniform float font_weight;
uniform int font_style; //format: 
uniform sampler2D msdf_texture;

out vec4 FragColor;
in vec2 posf;
in vec2 texp;

float median(float a, float b, float c) {
    return max(min(a, b), min(max(a, b), c));
}

void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}