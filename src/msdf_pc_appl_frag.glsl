#version 430 core

/*

Fragment shader to generate the composite msdf based on
the correction map

*/

uniform sampler2D base_msdf;
uniform sampler2D correction_map;

in vec2 tex_pos;

out vec4 FragColor;

void main() {
    vec4 co = texture(correction_map, tex_pos);
    
    if (co.w > 1.0)
        FragColor = vec4(1.0, 1.0, 1.0, length(co.xy) * 0.707106781);
    else
        FragColor = texture(base_msdf, tex_pos);
}