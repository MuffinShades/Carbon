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
    /*ivec2 texDim = textureSize(msdf_texture, 0);
    
    int x, y;

    for (x = -1; x <= 1; x++) {
        for (y = -1; y <= 1; y++) {
            FragColor = FragColor + texture(msdf_texture, vec2(texp.x + x * 1.2 * (1.0 / texDim.x), texp.y + y * 1.7 * (1.0 / texDim.y)));
        }
    }*/

    FragColor = texture(msdf_texture, texp);

    //FragColor = mix(vec4(0.0,0.0,0.0,0.0), vec4(1.0, 1.0, 1.0, 1.0), FragColor.w);

    float sigDist = median( FragColor.x, FragColor.y, FragColor.z);
    float wo = fwidth( sigDist );
    float opacity = smoothstep( 0.5 - wo, 0.5 + wo, sigDist);

    FragColor = mix(vec4(0.0,0.0,0.0,0.0), vec4(1.0, 1.0, 1.0, 1.0), opacity);
}