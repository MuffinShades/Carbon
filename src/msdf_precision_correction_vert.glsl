#version 430 core

layout (location = 0) in vec2 pos;
layout(location = 1) in int curve;

out flat int tCurve;
out vec2 txp;

void main() {
    tCurve = curve;
    txp = pos.xy;
    gl_Position = vec4(pos, 0.0, 1.0);
}