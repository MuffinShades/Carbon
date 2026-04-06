#version 430 core

/*

Call this shader with tex and position coords: (0,0 --> 1,1)
and then bind the base msdf to binding 0 and the correction 
map to binding 1

*/

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 txp;

out vec2 tex_pos;

void main() {
    tex_pos = txp;
    gl_Position = vec4(pos, 1.0);
}