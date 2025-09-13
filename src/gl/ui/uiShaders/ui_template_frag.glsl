/* Fragment template for all ui shaders */
#version 330

out vec4 fragColor;

in vec3 color;
in vec3 posf;

void main() {
    fragColor = color;
}