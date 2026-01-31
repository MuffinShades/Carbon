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

uniform int nCurves;

out vec4 FragColor;

in vec2 posf;
in ivec2 curve_range;

const float f_inf = 1.0 / 0.0;

float cross2(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

vec2 dBdt3(vec2 p0, vec2 p1, vec2 p2, float t) {
    return vec2(
        2.0 * ((1.0 - t) * (p1.x - p0.x) + t * (p2.x - p1.x)),
        2.0 * ((1.0 - t) * (p1.y - p0.y) + t * (p2.y - p1.y))
    );
}

vec2 bz3(vec2 p0, vec2 p1, vec2 p2, float t) {
    vec2 i0 = mix(p0, p1, t),
         i1 = mix(p1, p2, t);

    return mix(i0, i1, t);
}

float curveOrtho(Curve c, vec2 p, float t) {
    const vec2 b = bz3(c.p0, c.p1, c.p2, t);
    return abs(cross2(normalize(
        dBdt3(c.p0, c.p1, c.p2, t)
    ), normalize(
        vec2(p.x-b.x,p.y-b.y)
    )));
}

/*

Cubic solver functions

*/
const float one_third = 1.0 / 3.0;
const float i9 = 1.0 / 9.0, i54 = 1.0 / 54.0;
const float mu_pi = 3.1415926;

float cube_root_32(float v) {
    return sign(v) * pow(abs(v), 1.0/3.0);
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

int solve_re_cubic_32_a(float a, float b, float c, float d, out vec3 roots) {
    if (a == 0.0) return solve_re_quadratic_32(b, c, d, roots);

    b /= a; c /= a; d /= a;

    float bs = b * b, 
            q = (3.0 * c - bs) / 9.0,
            r = (-(27.0 * d) + b * (9.0 * c - 2.0 * bs)) / 54.0,
            q3 = q*q*q,
            dis = q3 + r*r;

    const float xi = (b / 3.0);

    if (dis > 0) {
        const float root_dis = sqrt(dis);

        float s = cube_root_32(r + root_dis);
        float t = cube_root_32(r - root_dis);

        roots.x = -xi + s + t;
        roots.y = 0;
        roots.z = 0;

        return 1;
    }

    if (dis == 0.0) {
        const float crt_r = cube_root_32(r);
        roots.x = -xi + 2.0 * crt_r;
        roots.y = -(crt_r + xi);
        roots.z = roots.y;
        return 2;
    }

    q *= -1.0;

    const float eta = acos(r / sqrt(-q3)) * one_third;
    const float r13 = 2.0 * sqrt(q);

    const float rot_1 = 2.0 * mu_pi * one_third,
              rot_2 = 4.0 * mu_pi * one_third;

    roots.x = -xi + r13*cos(eta + 0);
    roots.y = -xi + r13*cos(eta + rot_1);
    roots.z = -xi + r13*cos(eta + rot_2);

    return 3;
}

/*

EdgePointSignedDist but with curves cause yeah

*/
//TODO: consider removing the base precalculations since glsl is fast as fuck at computing those so the extra memory
//to the gpu might actually slow stuff down
struct c_dist {
    float d;
    float t;
};

c_dist CurvePointSignedDist(vec2 p, Curve c) {
    vec2 p0 = c.p0, p1 = c.p1, p2 = c.p2;

    vec4 computeBase = c.compute_base;

    vec3 root_vec = vec3(0,0,0);

    const int nRoots = solve_re_cubic_32_a(
            computeBase.x, 
            computeBase.y,
            computeBase.z 
                - 4.0 * dot(p0, p)
                + 8.0 * dot(p1, p)
                - 4.0 * dot(p2, p),
            computeBase.w 
                - 4.0 * dot(p1, p) 
                + 4.0 * dot(p0, p),
            root_vec
    );

    float roots[3] = {root_vec.x, root_vec.y, root_vec.z};

    c_dist rd;
    //rd.nExtT = 0;

    //check the roots
    int j;
    float t, t_i, alpha, beta, gamma, D, d_best, t_best, t_out;
    vec2 d_vec, m;

    //check endpoints
    //basically just computes the time for the end pionts based on a pseudo dist and the dist based on normal dist
    vec2 ip = c.p0 - p, fp = c.p2 - p, d0 = dBdt3(c.p0, c.p1, c.p2, 0.0), d1 = dBdt3(c.p0, c.p1, c.p2, 1.0), 
         ese0 = d0 - c.p0, ese1 = c.p2 - d1;
    
    //t = 0
    //this works by just finding the perpendicular to a line segment going from p0 to p0  + derivative of curve at t = 0
    //if t < 0 then we know it is a pseudo distance, the exact value of t doesnt matter in this case rather it is just used as a sign
    d_best = dot(ip,ip);
    t_out = -dot(ip, ese0) / dot(ese0, ese0);
    t_best = 0.0;
    d_vec = ip;

    //t = 1
    //same thing goes as previous but instead it's a curve / line segment defined from p2 - its derivative -> p2
    //this time checking for t > 1.0 since if it is greater than 1 than it is again a pseudo distance
    float eDist = dot(fp, fp);
    if (eDist < d_best) {
        d_best = eDist;
        t_best = 1.0;
        t_out = dot(fp, ese1) / dot(ese1, ese1);
        d_vec = fp;
    }

    for (j = 0; j < nRoots; j++) {
        t = roots[j];

        if (t > 1.0 || t < 0.0)
            continue;

        //check root compatibility
        t_i = 1.0 - t;
        alpha = t_i * t_i;
        beta = 2.0 * t_i * t;
        gamma = t * t;
        //dx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p.x;
        //dy = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p.y;
        m = alpha * p0 + beta * p1 + gamma * p2 - p;
        //D = dx*dx + dy*dy;
        D = dot(m,m);

        if (D < d_best) {
            d_best = D;
            d_vec = m;
            t_best = t;
            t_out = t;
        }
    }

    //return computed distance
    float s_dist = sign(cross2(dBdt3(c.p0, c.p1, c.p2, t_best), d_vec)) * sqrt(d_best);
    
    rd.d = s_dist;
    rd.t = t_out;

    return rd;
}

c_dist ConvertToPseudoDist(c_dist d, Curve c, vec2 p) {
    //p0
    //basically extend using ray method and find distance
    //checks for the weird t values from the CurvePointSignedDist then solves for the actual time and distance from point to extended curve instead of a end point
    //TODO: optimize ts
    if (d.t < 0.0) {
        //derivative is p1, p0 is p0
        vec2 dr = dBdt3(c.p0, c.p1, c.p2, 0.0);
        vec2 p1 = c.p0 + dr;
        vec2 p0 = c.p0;
        vec2 e = p1 - p0;
        float t = -dot(p - p0, e) / dot(e, e);

        if (t < 0.0) {
            float pd = abs(cross2(p, e) - cross2(p0, p1)) / e.length();

            if (pd < abs(d.d)) {
                d.d = sign(cross2(e, (p0 + dr * t) - p)) * pd;
                d.t = t;
            }
        }
    } 
    //p1
    else if (d.t > 1.0) {
        //derivative is p0, p2 is p1
        vec2 dr = dBdt3(c.p0, c.p1, c.p2, 0.0);
        vec2 p0 = c.p2 - dr;
        vec2 p1 = c.p2;
        vec2 e = p1 - p0;
        float t = -dot(p - p0, e) / dot(e, e);

        if (t > 1.0) {
            float pd = abs(cross2(p, e) - cross2(p0, p1)) / e.length();

            if (pd < abs(d.d)) {
                d.d = sign(cross2(e, (p0 + dr * t) - p)) * pd;
                d.t = t;
            }
        }
    }

    return d;
}

/*

Compute the stuff here

*/
void main() {
    /*const float gw = glyphDim.z - glyphDim.x,
                gh = glyphDim.w - glyphDim.y;

    vec2 padd_const = vec2(1.0 + padding.x + padding.z, 1.0 + padding.y + padding.w);

    vec2 p = vec2(
        (posf.x - padding.x - region.x) / (region.z - region.x) * (gw * padd_const.x) + glyphDim.x,
        (posf.y - padding.y - region.y) / (region.w - region.y) * (gh * padd_const.y) + glyphDim.y
    );

    p = vec2(
        (posf.x - region.x) / (region.z - region.x) * gw + glyphDim.x,
        (posf.y - region.y) / (region.w - region.y) * gh + glyphDim.y
    );*/

    const vec2 p = posf;

    int i;

    c_dist dr,dg,db, d_test;
    dr.d = f_inf;
    dg.d = f_inf;
    db.d = f_inf;
    d_test.d = f_inf;

    int cr, cg, cb;

    const float mu_epsil = 0.01;

    Curve tCurve;

    for (i = curve_range.x; i < curve_range.y; i++) {
        tCurve = glyph_curves[i];
        c_dist d = CurvePointSignedDist(p, tCurve);

        float ddr = abs(d.d) - abs(dr.d), ddg = abs(d.d) - abs(dg.d), ddb = abs(d.d) - abs(db.d), ddd = abs(d.d) - abs(d_test.d);

        if (tCurve.chroma.x != 0 && (
                ddr < -mu_epsil ||
                (ddr > -mu_epsil && ddr < mu_epsil && 
                    curveOrtho(tCurve, p, d.t) > curveOrtho(glyph_curves[cr], p, dr.t)
                )
        )) {
            dr = d;
            cr = i;
        }

        if (tCurve.chroma.y != 0 && (
                ddg < -mu_epsil ||
                (ddg > -mu_epsil && ddg < mu_epsil && 
                    curveOrtho(tCurve, p, d.t) > curveOrtho(glyph_curves[cg], p, dg.t)
                )
        )) {
            dg = d;
            cg = i;
        }

        if (tCurve.chroma.z != 0 && (
                ddb < -mu_epsil ||
                (ddb > -mu_epsil && ddb < mu_epsil && 
                    curveOrtho(tCurve, p, d.t) > curveOrtho(glyph_curves[cb], p, db.t)
                )
        )) {
            db = d;
            cb = i;
        }

        if (ddd < -mu_epsil) {
            d_test = d;
        }
    }

    vec4 colors[] = {
        vec4(1.0, 0.0, 0.0, 1.0),
        vec4(0.0, 1.0, 0.0, 1.0),
        vec4(0.0, 0.0, 1.0, 1.0),
        vec4(1.0, 1.0, 0.0, 1.0)
    };

    //now compute the final color and output it
    dr = ConvertToPseudoDist(dr, glyph_curves[cr], p);
    dg = ConvertToPseudoDist(dg, glyph_curves[cg], p);
    db = ConvertToPseudoDist(db, glyph_curves[cb], p);

    float blend_amount = 1.0;
 
    FragColor = vec4(
        dr.d > 0 ? (((dr.d) / blend_amount) * 0.5 + 0.5) : 0,
        dg.d > 0 ? (((dg.d) / blend_amount) * 0.5 + 0.5) : 0,
        db.d > 0 ? (((db.d) / blend_amount) * 0.5 + 0.5) : 0,
        1.0
    );
}