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
    vec4 base = texture(base_msdf, tex_pos);

    float mx = max(max(co.x, co.y), co.z);

    int nc = (abs(co.x - mx) < 0.001 ? 1 : 0) + (abs(co.y - mx) < 0.001 ? 1 : 0) + (abs(co.z - mx) < 0.001 ? 1 : 0);
    
    if (co.w > 1.0 && nc < 2) {
        FragColor = vec4(1.0, 1.0, 1.0, length(co.xy) * 0.707106781);
    } else {
       FragColor = base;
    }

    FragColor = base;

    //float q = texture(correction_map, tex_pos).w > 0.0 ? 1.0 : 0.0;

    //FragColor = mix(texture(base_msdf, tex_pos), vec4(q,q,q, 1.0), 0.5);
    //FragColor.w = 1.0;
}