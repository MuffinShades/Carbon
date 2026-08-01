#version 430 core

/*

Raycount with automatic countour adjustment

Programmed by muffinshades 2026

*/

struct Curve {
    vec2 p0;
    //int padd0[2];
    vec2 p1;
    //int padd1[2];
    vec2 p2;

    float minW;
    int cu_connect;
};

layout (std430, binding = 0) buffer GlyphCurves {
    Curve glyph_curves[];
};

out vec4 FragColor;

in vec2 poss;
in flat ivec2 curve_range;
in vec2 delta;

uniform vec3 font_color;

const float f_inf = 1.0 / 0.0;

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

int solve_linear_32(float a, float b, out vec3 roots) {
    roots.x = -b/a;
    roots.y = -1.0;
    roots.z = -1.0;
    return 1;
}

int solve_re_quadratic_32(float a, float b, float c, out vec3 roots) {
    if (a >= -mu_epsil && a < mu_epsil)
        return solve_linear_32(b, c, roots);

    const float i = b*b - 4.0*a*c;

    if (i < -mu_epsil)
        return 0;
    
    if (i >= -mu_epsil && i < mu_epsil) {
        roots.x = -b / (2.0*a);
        roots.y = -1.0;
        roots.z = -1.0;
        return 1;
    }

    const float r = sqrt(i);
    const float A = (2.0 * a);

    roots.x = (-b + r) / A;
    roots.y = (-b - r) / A;
    roots.z = -1.0;

    return 2;
}

float lumaTransform(float luma) {
    return clamp((
        -(cos(mu_pi*luma)-1) * 0.5
    ), 0.0, 1.0);
}

struct cuBuddy {
    int i0,i1;
    bool rp00, rp01;
};

cuBuddy decode_buddy_inf(int connect_inf) {
    cuBuddy res;

    res.rp01 = (((connect_inf >> 15) & 1) == 0);
    res.i1 = connect_inf & 0x7FFF;
    connect_inf >>= 16;
    res.rp00 = (((connect_inf >> 15) & 1) == 0);
    res.i0 = connect_inf & 0x7FFF;

    return res;
};

/*

Compute the stuff here

*/
void main() {
    const float boldness = 9.0;
    const float blurr = 0.27;

    int i, j, count = 0;

    int counts[9] = {0,0,0,0,0,0,0,0,0};

    vec2 offsets[9] = {vec2(-blurr, -blurr),vec2(-blurr, 0.0),vec2(-blurr, blurr),
                       vec2(0.0, -blurr),vec2(0.0, 0.0),vec2(0.0, blurr),
                       vec2(blurr, -blurr),vec2(blurr, 0.0),vec2(blurr, blurr)};

    vec2 rpos = vec2(floor(poss.x / delta.x) * delta.x, floor(poss.y / delta.y) * delta.y);

    vec2 p0,p1,p2,posf;

    Curve tCurve, tCurve0, tCurve1;

    float ys,xs,w;

    //adjust curves first
    for (i = curve_range.x; i <= curve_range.y; i++) {
        tCurve = glyph_curves[i];

        if (tCurve.minW == 0.0)
           continue;
        
        //decode width stuff
        xs = sign(tCurve.minW);
        w = tCurve.minW * xs;
        if (w >= 1.0842022e-19) {
            ys = 1.0;
            w *= 5.4210109e20;
        } else {
            ys = 0.0;
        }
        w *= 1.0633824e37;

        cuBuddy bDat = decode_buddy_inf(tCurve.cu_connect);

        const float sscalee = 4.0, scl = delta.x * sscalee;

        //determine what point is connected
        /*if (bDat.rp00) {
            if (xs < 0.0)
                glyph_curves[i].p0.x = (floor(glyph_curves[i].p0.x / scl) * scl);
            else
                glyph_curves[i].p0.x = (ceil(glyph_curves[i].p0.x / scl) * scl);
            glyph_curves[bDat.i0].p0.x = glyph_curves[i].p0.x;
        } else {
            if (xs < 0.0)
                glyph_curves[i].p0.x = (floor(glyph_curves[i].p0.x / scl) * scl);
            else
                glyph_curves[i].p0.x = (ceil(glyph_curves[i].p0.x / scl) * scl);
            glyph_curves[bDat.i0].p2.x = glyph_curves[i].p0.x;
        }

        if (bDat.rp01) {
            if (xs < 0.0)
                glyph_curves[i].p2.x = (floor(glyph_curves[i].p2.x / scl) * scl);
            else
                glyph_curves[i].p2.x = (ceil(glyph_curves[i].p2.x / scl) * scl);
            glyph_curves[bDat.i1].p0.x = glyph_curves[i].p2.x;
        } else {
            if (xs < 0.0)
                glyph_curves[i].p2.x = (floor(glyph_curves[i].p2.x / scl) * scl);
            else
                glyph_curves[i].p2.x = (ceil(glyph_curves[i].p2.x / scl) * scl);
            glyph_curves[bDat.i1].p2.x = glyph_curves[i].p2.x;
        }*/
    }

    //now render the curves
    for (i = curve_range.x; i <= curve_range.y; i++) {
        tCurve = glyph_curves[i];

        p0 = tCurve.p0; p1 = tCurve.p1; p2 = tCurve.p2; 

        //easy kinda bulk check
        for (j = 0; j < offsets.length; j++) {
            posf = rpos + (offsets[j] + vec2(0.5, 0.5)) * delta;

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

            if (rRoots.x >= 0.0 && rRoots.x < 1.0 && bz3(p0, p1, p2, rRoots.x).x > posf.x) counts[j]++;
            if (rRoots.y >= 0.0 && rRoots.y < 1.0 && bz3(p0, p1, p2, rRoots.y).x > posf.x) counts[j]++;
        }
    }

    int c = (counts[0] % 2) + (counts[1] % 2) + (counts[2] % 2) + 
            (counts[3] % 2) + (counts[4] % 2) + (counts[5] % 2) + 
            (counts[6] % 2) + (counts[7] % 2) + (counts[8] % 2);

    float luma_scale = min(c / boldness, 1.0);

    //luma_scale = lumaTransform(luma_scale);

    FragColor = vec4(font_color, luma_scale);
}