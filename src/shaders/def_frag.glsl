#version 330

out vec4 fragColor;

in vec2 coords;
in vec4 posf;

uniform sampler2D tex;

void main() {
    const float fogZ = 95.0;

    //mix adds fog
    fragColor = mix(texture(tex, coords), vec4(0.2, 0.7, 1.0, 1.0), min(max(posf.z / fogZ, 0.0), 1.0));
}