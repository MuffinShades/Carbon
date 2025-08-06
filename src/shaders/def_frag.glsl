#version 330

out vec4 fragColor;

in vec2 coords;

void main() {
    fragColor = vec4(coords.x,0.0,coords.y,1.0);
    //fragColor = vec4(1.0,0.0,0.0,1.0);
}