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
    return 1.0;
}

void main() {
    vec4 text_color = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 smp = texture(msdf_texture, texp).rgb;
    float sigDist = median(smp.r, smp.g, smp.b);
    float w = fwidth(sigDist);
    float opacity = smoothstep(-w, +w, sigDist - 0.5);
    FragColor = vec4(text_color.rgb, opacity);
}