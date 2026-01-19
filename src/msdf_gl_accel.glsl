#version 430 core

struct Curve {
    vec2 p0, p1, p2;
    vec3 chroma;
    vec4 compute_base;
};

layout (std140, binding = 0) buffer GlyphCurves {
    Curve glyph_curves[];
};

uniform int nCurves;
uniform vec4 padding;
uniform vec4 region;
uniform vec4 glyphDim;

out vec4 FragColor;

in vec2 posf;

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

float curveOrtho(Curve c, vec2 p, float t) {
    const vec2 b = vec2(
        (1.0 * t) * (1.0 - t) * c.p0.x + 2.0 * (1.0 - t) * t * c.p1.x + t * t * c.p2.x,
        (1.0 * t) * (1.0 - t) * c.p0.y + 2.0 * (1.0 - t) * t * c.p1.y + t * t * c.p2.y
    );
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
const float mu_pi = 3.14159265358979323846;

struct root_sln {
    int n;
    float roots[3];
};

//TODO: check cube root
float cube_root_32(float v) {
    if (v < 0.0)
        return -pow(-v, one_third);
    else
        return pow(v, one_third);
}

root_sln solve_linear_32(float a, float b) {
    root_sln res;
    res.n = 1;
    res.roots[0] = -b/a;
    return res;
}

root_sln solve_re_quadratic_32(float a, float b, float c) {
    if (a == 0)
        return solve_linear_32(b, c);

    root_sln res;

    const float i = b*b - 4.0*a*c;

    if (i < 0.0) {
        res.n = 0;
        return res;
    }
    
    if (i == 0.0) {
        res.roots[1] = -b / (2.0*a);
        res.n = 1;
        return res;
    }

    const float r = sqrt(i);
    const float i2a = 1.0 / (2.0 * a);

    res.roots[0] = (-b + r) * i2a;
    res.roots[1] = (-b - r) * i2a;
    res.n = 2;
    return res;
}

root_sln solve_re_cubic_32_a(float a, float b, float c, float d) {
    if (a == 0.0) return solve_re_quadratic_32(b, c, d);

    root_sln res;

    const float ia = 1.0 / a;

    b *= ia; c *= ia; d *= ia;

    float bs = b * b, 
            q = (3.0 * c - bs) * i9,
            r = (-(27.0 * d) + b * (9.0 * c - 2.0 * bs)) * i54,
            q3 = q*q*q,
            dis = q3 + r*r;

    const float xi = (b * one_third);

    if (dis > 0) {
        const float root_dis = sqrt(dis);

        float s = cube_root_32(r + root_dis);
        float t = cube_root_32(r - root_dis);

        res.roots[0] = -xi + s + t;
        res.roots[1] = 0.0;
        res.roots[2] = 0.0;
        res.n = 1;
        return res;
    }

    if (dis == 0.0) {
        const float crt_r = cube_root_32(r);
        res.roots[0] = -xi + 2.0 * crt_r;
        res.roots[2] = (res.roots[1] = (
            -(crt_r + xi)
        ));
        res.n = 2;
        return res;
    }

    q *= -1.0;

    const float eta = acos(r / sqrt(-q3)) * one_third;
    const float r13 = 2.0 * sqrt(q);

    const float rot_1 = 2.0 * mu_pi * one_third,
              rot_2 = 4.0 * mu_pi * one_third;

    res.roots[0] = -xi + r13*cos(eta + 0);
    res.roots[1] = -xi + r13*cos(eta + rot_1);
    res.roots[2] = -xi + r13*cos(eta + rot_2);
    res.n = 3;

    return res;
}

/*

EdgePointSignedDist but with curves cause yeah

*/
//TODO: consider removing the base precalculations since glsl is fast as fuck at computing those so the extra memory
//to the gpu might actually slow stuff down
struct c_dist {
    float d;
    float t;
    float extT[3];
    int nExtT;
};

c_dist CurvePointSignedDist(vec2 p, Curve c) {
    vec2 p0 = c.p0, p1 = c.p1, p2 = c.p2;

    vec4 computeBase = c.compute_base;

    const root_sln roots = solve_re_cubic_32_a(
            computeBase.x, 
            computeBase.y,
            computeBase.z 
                - 4.0 * (p0.y*p.y + p0.x*p.x)
                + 8.0 * (p1.y*p.y + p1.x*p.x)
                - 4.0 * (p2.y*p.y + p2.x*p.x),
            computeBase.w 
                - 4.0 * (p1.y*p.y + p1.x*p.x) 
                + 4.0 * (p0.y*p.y + p0.x*p.x)
    );

    c_dist rd;

    //check the roots
    int j;
    float t, t_i, alpha, beta, gamma, dx, dy, D, d_best = f_inf, dx_best, dy_best, t_best;
    for (j = 0; j < roots.n; j++) {
        t = roots.roots[j];

        if (t > 1.0 || t < 0.0) {
            rd.extT[rd.nExtT++] = t;
            continue;
        }

        //check root compatibility
        t_i = 1.0 - t;
        alpha = t_i * t_i;
        beta = 2.0 * t_i * t;
        gamma = t * t;
        dx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p.x;
        dy = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p.y;
        D = dx*dx + dy*dy;

        if (D < d_best) {
            d_best = D;
            dx_best = dx;
            dy_best = dy;
            t_best = t;
        }
    }

    //check le endpoints
    const float ePoints[2] = {0.0, 1.0};

    for (j = 0; j < 2; j++) {
        t = ePoints[j];
        t_i = 1.0 - t;
        alpha = t_i * t_i;
        beta = 2.0 * t_i * t;
        gamma = t * t;
        dx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p.x;
        dy = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p.y;
        D = dx*dx + dy*dy;

        if (D < d_best) {
            d_best = D;
            dx_best = dx;
            dy_best = dy;
            t_best = t;
        }
    }

    //return computed distance
    float s_dist = sign(cross2(dBdt3(c.p0, c.p1, c.p2, t_best), vec2(dx_best, dy_best))) * sqrt(d_best);

    rd.d = s_dist;
    rd.t = t_best;

    return rd;
}

c_dist ConvertToPseudoDist(c_dist d, Curve c, vec2 p) {
    if (d.nExtT <= 0) return d;

    if (d.nExtT > 3) d.nExtT = 3;

    float t, t_i, alpha, beta, gamma, dx, dy, D, d_best = d.d * d.d, dx_best, dy_best, t_best;

    for (int i = 0; i < d.nExtT; i++) {
        t_i = 1.0 - t;
        alpha = t_i * t_i;
        beta = 2.0 * t_i * t;
        gamma = t * t;
        dx = (alpha * c.p0.x + beta * c.p1.x + gamma * c.p2.x) - p.x;
        dy = (alpha * c.p0.y + beta * c.p1.y + gamma * c.p2.y) - p.y;
        D = dx*dx + dy*dy;

        if (D < d_best) {
            d_best = D;
            dx_best = dx;
            dy_best = dy;
            t_best = t;
        }
    }

    float s_dist = sign(cross2(dBdt3(c.p0, c.p1, c.p2, t_best), vec2(dx_best, dy_best))) * sqrt(d_best);

    d.d = s_dist;
    d.t = t_best;

    return d;
}

/*

Compute the stuff here

*/
void main() {
    const float gw = glyphDim.z - glyphDim.x,
                gh = glyphDim.w - glyphDim.y;

    vec2 padd_const = vec2(1.0 + padding.x + padding.z, 1.0 + padding.y + padding.w);

    vec2 p = vec2(
        (posf.x - padding.x - region.x) * (gw * padd_const.x) + glyphDim.x,
        (posf.y - padding.y - region.y) * (gh * padd_const.y) + glyphDim.y
    );

    int i;

    c_dist dr,dg,db;
    dr.d = f_inf;
    dg.d = f_inf;
    db.d = f_inf;

    int cr, cg, cb;

    const float mu_epsil = 0.01;

    Curve tCurve;

    for (i = 0; i < nCurves; i++) {
        tCurve = glyph_curves[i];
        c_dist d = CurvePointSignedDist(p, tCurve);

        float ddr = d.d - dr.d, ddg = d.d - dg.d, ddb = d.d - db.d;

        if (tCurve.chroma.x != 0 && (
                ddr < -mu_epsil ||
                (ddr > -mu_epsil && ddr < mu_epsil && 
                    curveOrtho(tCurve, p, d.t) > curveOrtho(tCurve, p, dr.t)
                )
        )) {
            dr = d;
            cr = i;
        }

        if (tCurve.chroma.y != 0 && (
                ddg < -mu_epsil ||
                (ddg > -mu_epsil && ddg < mu_epsil && 
                    curveOrtho(tCurve, p, d.t) > curveOrtho(tCurve, p, dg.t)
                )
        )) {
            dg = d;
            cg = i;
        }

        if (tCurve.chroma.z != 0 && (
                ddb < -mu_epsil ||
                (ddb > -mu_epsil && ddb < mu_epsil && 
                    curveOrtho(tCurve, p, d.t) > curveOrtho(tCurve, p, db.t)
                )
        )) {
            db = d;
            cb = i;
        }
    }

    //now compute the final color and output it
    dr = ConvertToPseudoDist(dr, glyph_curves[cr], p);
    dg = ConvertToPseudoDist(dg, glyph_curves[cg], p);
    db = ConvertToPseudoDist(db, glyph_curves[cb], p);

    float blend_amount = 1.0;
 
    /*FragColor = vec4(
       (((dr.d) / blend_amount) * 0.5 + 0.5),
        (((dg.d) / blend_amount) * 0.5 + 0.5),
        (((db.d) / blend_amount) * 0.5 + 0.5),
        1.0
    );*/


    /*vec4 colors[] = {
        vec4(1.0, 0.0, 0.0, 1.0),
        vec4(0.0, 1.0, 0.0, 1.0),
        vec4(0.0, 0.0, 1.0, 1.0),
        vec4(1.0, 1.0, 0.0, 1.0)
    };

    root_sln dbg_sln = solve_re_cubic_32_a(17, 2, 12, 15);

    FragColor = vec4(abs(dbg_sln.roots[0]), 1.0, 1.0, 1.0);*/
}