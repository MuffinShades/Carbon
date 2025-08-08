#version 330

out vec4 fragColor;

in vec2 coords;
in vec3 posf;

uniform sampler2D tex;

void main() {
    fragColor = texture(tex, coords);
    //fragColor = vec4(1.0,0.0,0.0,1.0);
}