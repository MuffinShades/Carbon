#version 430 core

out vec4 fragColor;

in vec3 pos;

uniform sampler2D tex;

void main() {
    fragColor = texture(tex, pos.xy);
}