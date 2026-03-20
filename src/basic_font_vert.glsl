#version 430 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 msdf_tex;

out vec2 posf;
out vec2 texp;

//uniform vec4 glf_transform_ext; //xy -> translation, zw -> scaling
//uniform mat2 glf_transform;
uniform mat4 screen_project;

void main() {
    posf = pos.xy;
    texp = msdf_tex;
    //gl_Position = screen_project * vec4((glf_transform * pos.xy) * glf_transform_ext.zw + glf_transform_ext.xy, pos.z, 1.0);

    gl_Position = screen_project * vec4(pos, 1.0);
    //gl_Position = vec4(pos, 1.0);
}