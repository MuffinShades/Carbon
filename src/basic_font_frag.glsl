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
    vec4 tex_color = vec4(1.0, 1.0, 1.0, 1.0);
    vec3 smp = texture(msdf_texture, texp).rgb;
    ivec2 sz = textureSize(msdf_texture, 0).xy;
    float dx = dFdx(posf.x) * sz.x; 
    float dy = dFdy(posf.y) * sz.y;
    float toPixels = 8.0 * inversesqrt(dx * dx + dy * dy);
    float sigDist = median(smp.r, smp.g, smp.b);
    float w = fwidth(sigDist);
    float opacity = smoothstep(0.5 - w, 0.5 + w, sigDist);
    FragColor = vec4(smp.rgb, 1.0);
}