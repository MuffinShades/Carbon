#version 330

out vec4 fragColor;

in vec2 coords;
in vec4 posf;
in vec3 n;

uniform sampler2D tex;

float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

vec3 globalLight = vec3(50.0, 20.0, 50.0);

void main() {
    const float fogZ = 100.0;

    float ambient = 0.4;

    //mix adds fog
    float fogEf = fade(posf.z / fogZ);
    vec3 lightDir = normalize(globalLight - posf.xyz);
    fragColor = mix(texture(tex, coords) * (1.0 - fogEf) * (max(dot(n, lightDir), 0.0) + ambient), vec4(0.4, 0.7, 1.0, 1.0), min(max(fogEf, 0.0), 1.0));
}