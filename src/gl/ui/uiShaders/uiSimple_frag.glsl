#version 430 core

out vec4 FragColor;
in vec4 frag_col;

void main() {
    FragColor = frag_col;
}