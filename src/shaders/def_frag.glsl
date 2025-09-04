#version 330

out vec4 fragColor;

in vec2 coords;
in vec4 posf;
in vec3 n;
in vec3 camPos;
in vec3 worldPos;

uniform sampler2D tex;

float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

vec3 globalLight = vec3(50.0, 20.0, 50.0);

void main() {
    const float fogZ = 1000.0;

    float ambient = 0.4;

    vec4 tx_col;

    //get look direction
    if (posf.z < 20.0) {
    
    vec3 lookDir = normalize(camPos - posf.xyz);

    vec2 huv = coords;
    float h = 1.0 - texture(tex, huv).r, ch = 0.0; //h and cur h

    const float minLayers = 8.0, maxLayers = 64.0;
    float nLayers = mix(maxLayers, minLayers, abs(dot(n, lookDir)));
    float dh = 1.0 / nLayers;
    vec2 S = lookDir.xz / lookDir.y * 0.05;
    vec2 duv = S / nLayers;

    //our current height will be greater than the sample height when there is a hit
    while (ch < h) {
        h = 1.0 - texture(tex, huv).r;
        huv += duv;
        ch += dh;
    }

    if (huv.x < 0.0 || huv.y < 0.0 || huv.x > 1.0 || huv.y > 1.0)
        discard;

    vec2 prevTc = huv - duv;
    float afterDepth = h - ch;
    float beforeDepth = 1.0 - texture(tex, prevTc).r - ch + dh;
    float weight =  afterDepth / (afterDepth - beforeDepth);
    huv = prevTc * weight + huv * (1.0 - weight);

    tx_col = texture(tex, huv);

    } else {
        tx_col = texture(tex, coords);
    }

    //mix adds fog
    float fogEf = fade(posf.z / fogZ);
    vec3 lightDir = normalize(globalLight - posf.xyz);
    fragColor = mix(tx_col * (1.0 - fogEf) * (max(dot(n, lightDir), 0.0) + ambient), vec4(0.4, 0.7, 1.0, 1.0), min(max(fogEf, 0.0), 1.0));
}