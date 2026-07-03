#version 430 core

/*

MSDF generation excelerated with le gpu

This took forever to write

Programmed by muffinshades 2026

*/

struct Curve {
    vec2 p0;
    int padd0[2];
    vec2 p1;
    int padd1[2];
    vec2 p2;
    int padd2[2];
    vec3 chroma;
    int padd3;
    vec4 compute_base;
};

layout (std430, binding = 0) buffer GlyphCurves {
    Curve glyph_curves[];
};

out vec4 FragColor;

in vec2 posf;
in flat ivec2 curve_range;

const float f_inf = 1.0 / 0.0;

float cross2(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

vec2 dBdt3(vec2 p0, vec2 p1, vec2 p2, float t) {
    return 2.0 * (1.0 - t) * (p1 - p0) + 2.0 * t * (p2 - p1);
}

vec2 bz3(vec2 p0, vec2 p1, vec2 p2, float t) {
    vec2 i0 = mix(p0, p1, t),
         i1 = mix(p1, p2, t);

    return mix(i0, i1, t);
}

/*

Cubic solver functions

*/
const float one_third = 1.0 / 3.0;
const float i9 = 1.0 / 9.0, i54 = 1.0 / 54.0;
const float mu_pi = 3.1415926;
const float mu_epsil = 0.00001;

float nz_sign(float v) {
    if (v > 0)
        return 1.0;
    else
        return -1.0;
}

int solve_linear_32(float a, float b, out vec3 roots) {
    roots.x = -b/a;
    roots.y = 0;
    roots.z = 0;
    return 1;
}

int solve_re_quadratic_32(float a, float b, float c, out vec3 roots) {
    if (a == 0)
        return solve_linear_32(b, c, roots);

    const float i = b*b - 4.0*a*c;

    if (i < 0.0)
        return 0;
    
    if (i == 0.0) {
        roots.x = -b / (2.0*a);
        roots.y = 0;
        roots.z = 0;
        return 1;
    }

    const float r = sqrt(i);
    const float A = (2.0 * a);

    roots.x = (-b + r) / A;
    roots.y = (-b - r) / A;
    roots.z = 0;

    return 2;
}

/*

Compute the stuff here

*/
void main() {
    int i, count = 0;

    vec3 p0,p1,p2;

    Curve tCurve;

    for (i = curve_range.x; i < curve_range.y; i++) {
        tCurve = glyph_curves[i];

        p0 = tCurve.p0; p1 = tCurve.p1; p2 = tCurve.p2; 

        //easy kinda bulk check
        if (p0.y > p2.y) {
            if (
                (p0.y >= posf.y && p2.y > posf.y) ||
                (p0.y <= posf.y && p2.y < posf.y) ||
                (p0.x < posf.x && p2.x < posf.x)
            ) {
                continue;
            }
        } else {
            if (
                (p0.y > posf.y && p2.y >= posf.y) ||
                (p0.y < posf.y && p2.y <= posf.y) ||
                (p0.x < posf.x && p2.x < posf.x)
            ) {
                continue;
            }
        }

        vec3 rRoots = vec3(-1.0, -1.0, -1.0); //set by default to an invalid value
        int nRoots = solve_re_quadratic_32(p2.y - 2.0 * p1.y + p0.y, 2.0 * (p1.y - p0.y), p0.y - posf.y, rRoots);

        if (rRoots.x >= 0.0 && rRoots.x < 1.0 && bz3(p0, p1, p2, rRoots.y).x > posf.x) count++;
        if (rRoots.y >= 0.0 && rRoots.y < 1.0 && bz3(p0, p1, p2, rRoots.y).x > posf.x) count++;
    }

    if (count % 2 == 0) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    } else {   
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}