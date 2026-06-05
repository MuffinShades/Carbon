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

float spxd() {
    return 1;
}

void main() {
    vec4 tex_color = vec4(1.0, 1.0, 1.0, 1.0);
    vec3 smp = texture(msdf_texture, texp).rgb;
    float md = median(smp.r, smp.g, smp.b);
    float spd = spxd() * (md - 0.5);
    float opacity = clamp(spd + 0.5, 0.0, 1.0);
    FragColor = vec4(tex_color.rgb, opacity);
}