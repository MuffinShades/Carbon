#version 330 core

uniform int nCurves;

struct Curve {
    vec2 p0, p1, p2;
    vec3 chroma;
    vec4 compute_base;
};

layout (std430, binding = 0) uniform GlyphCurves {
    Curve curves[];
}

uniform vec4 padding;
uniform vec4 region;
uniform vec2 compositeRatio; //wc and hc
uniform vec4 glyphDim;

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

f32 curveOrtho(BCurve c, Point p, f32 t) {
    const Point b = bezier3(c.p[0], c.p[1], c.p[2], t);
    return abs(pointCross(pointNormalize(
        dBdt3(c.p[0], c.p[1], c.p[2], t)
    ), pointNormalize(
        {p.x-b.x,p.y-b.y}
    )));
}

/*

Cubic solver functions

*/
const float one_third = 1.0 / 3.0;
const float i9 = 1.0 / 9.0, i54 = 1.0 / 54.0;
const float mu_pi = 3.14159265358979323846;

float cube_root_32(float v) {
    const float m = -(2.0 * (i32) (v < 0) - 1.0);
    return m * pow(m * v, one_third);
}

int solve_linear_32(float a, float b, float roots[1]) {
    roots[0] = -b/a;
    return 1;
}

int solve_re_quadratic_32(float a, float b, float c, float re_roots[2]) {
    if (a == 0)
        return solve_linear_32(b, c, re_roots);

    const float i = b*b - 4.0*a*c;

    if (i < 0.0)
        return 0;
    
    if (i == 0.0) {
        re_roots[1] = -b / (2.0*a);
        return 1;
    }

    const float r = sqrt(i);
    const float i2a = 1.0 / (2.0 * a);

    re_roots[0] = (-b + r) * i2a;
    re_roots[1] = (-b - r) * i2a;

    return 2;
}

int solve_re_cubic_32_a(float a, float b, float c, float d, float re_roots[3]) {
    if (!re_roots) return 0;
    if (a == 0.0) return solve_re_quadratic_32(b, c, d, re_roots);

    const float ia = 1.0 / a;

    b *= ia; c *= ia; d *= ia;

    float bs = b * b, 
            q = (3.0 * c - bs) * i9,
            r = (-(27.0 * d) + b * (9.0 * c - 2.0 * bs)) * i54,
            q3 = q*q*q,
            dis = q3 + r*r;

    const float xi = (b * one_third);

    if (dis > 0) {
        const float root_dis = sqrtf(dis);

        float s = cube_root_32(r + root_dis);
        float t = cube_root_32(r - root_dis);

        re_roots[0] = -xi + s + t;
        re_roots[1] = 0.0;
        re_roots[2] = 0.0;

        return 1;
    }

    if (dis == 0.0) {
        const float crt_r = cube_root_32(r);
        re_roots[0] = -xi + 2.0 * crt_r;
        re_roots[2] = (re_roots[1] = (
            -(crt_r + xi)
        ));
        return 2;
    }

    q *= -1.0;

    const float eta = acosf(r / sqrtf(-q3)) * one_third;
    const float r13 = 2.0 * sqrt(q);

    const float rot_1 = 2.0 * mu_pi * one_third,
              rot_2 = 4.0 * mu_pi * one_third;

    re_roots[0] = -xi + r13*cos(eta + 0);
    re_roots[1] = -xi + r13*cos(eta + rot_1);
    re_roots[2] = -xi + r13*cos(eta + rot_2);

    return 3;
}

/*

EdgePointSignedDist but with curves cause yeah

*/
//TODO: consider removing the base precalculations since glsl is fast as fuck at computing those so the extra memory
//to the gpu might actually slow stuff down
float CurvePointSignedDist(vec2 p, Curve c) {
    float roots[3];

    vec2 p0 = c.p0, p1 = c.p1, p2 = c.p2;

    vec4 computeBase = GlyphCurves.compute_base[i];

    const int nRoots = solve_re_cubic_32_a(
            computeBase.x, 
            computeBase.y,
            computeBase.z, 
                - 4.0 * (p0*p.y + p0.x*p.x)
                + 8.0 * (p1.y*p.y + p1.x*p.x)
                - 4.0 * (p2.y*p.y + p2.x*p.x),
            computeBase.w 
                - 4.0 * (p1.y*p.y + p1.x*p.x) 
                + 4.0 * (p0.y*p.y + p0.x*p.x)
    );

    //check the roots
    int j;
    float t, t_i, alpha, beta, gamma, dx, dy, D, d_best = f_inf, dx_best, dy_best, t_best;
    for (j = 0; j < nRoots; j++) {
        t = roots[j];

        if (t > 1.0 || t < 0.0) continue;

        //check root compatibility
        t_i = 1.0 - t;
        alpha = t_i * t_i;
        beta = 2.0 * t_i * t;
        gamma = t * t;
        dx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p.x;
        dy = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p.y;
        D = dx*dx + dy*dy;

        if (D < d) {
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

        if (D < d) {
            d_best = D;
            dx_best = dx;
            dy_best = dy;
            t_best = t;
        }
    }

    //return computed distance
    return sign(dBdt3(), vec2(dx_best, dy_best)) * sqrt(d_best);
}

/*

Compute the stuff here

*/
void main() {
    const vec2 p = vec2(
        floor((posf.x - padding.x) + 0.5) * compositeRatio.x + glyphDim.x,
        floor((posf.y - padding.y) + 0.5) * compositeRatio.y + glyphDim.y
    );

    int i;

    float dr = f_inf,dg = f_inf,db = f_inf;
    int cr, cg, cb;

    const float mu_epsil = 0.01;

    Curve tCurve;

    for (i = 0; i < nCurves; i++) {
        tCurve = GlyphCurves.curves[i];
        float d = CurvePointSignedDist(p, tCurve);

        float ddr = d - dr, ddg = d - dg, ddb = d - db;

        if (tCurve.chroma.x && (
                ddr < -mu_epsil ||
                (ddr > -mu_epsil && ddr < mu_epsil && 

                )
        )) {
            dr = d;
            cr = i;
        }
    }

    //now compute the final color and output it
}


i32 xScanMin = 0, xScanMax = regionW, scanDx = 1;
    for (y = 0; y < regionH; ++y) {
        for (x = xScanMin; abs(xScanMax - x) > 0; x += scanDx) {
            p.x = floor(((f32)x - paddingLeft) + 0.5f) * wc + (tGlyph.xMin);
            p.y = floor(((f32)y - paddingTop) + 0.5f) * hc + (tGlyph.yMin);
            
            dr.d = dg.d = db.d = INFINITY;

            //Edge e = glyphEdges[0];

            for (Edge e : glyphEdges) {
                //big optimizing :3
                if (testRadius > 0 &&
                    (p.x - e.bounds.center.x) * (p.x - e.bounds.center.x) +
                    (p.y - e.bounds.center.y) * (p.y - e.bounds.center.y)
                > (testRadius + e.bounds.r) * (testRadius + e.bounds.r)) {
                    //continue;
                }

                //d = FancyEdgePointSignedDist(p, e, testRadius);
                d = EdgePointSignedDist(p,e);

                if (d.d == INFINITY)
                    continue;

                if (e.color.x) {
                    if (abs(abs(d.d) - abs(dr.d)) <= 0.01f) {
                        //check orthoginality
                        f32 o1 = curveOrtho(d.curve, p, d.t),
                            o2 = curveOrtho(dr.curve, p, dr.t);

                        if (o2 < o1) goto set_r;
                    } else if (abs(d.d) < abs(dr.d)) {
                     set_r:  
                        dr = d;
                        er = e;
                    }
                }

                if (e.color.y) {
                    if (abs(abs(d.d) - abs(dg.d)) <= 0.01f) {
                        //check orthoginality
                        f32 o1 = curveOrtho(d.curve, p, d.t),
                            o2 = curveOrtho(dg.curve, p, dg.t);

                        if (o2 < o1) goto set_g;
                    } else if (abs(d.d) < abs(dg.d)) {
                    set_g:  
                        dg = d;
                        eg = e;
                    }
                }

                if (e.color.z) {
                    if (abs(abs(d.d) - abs(db.d)) <= 0.01f) {
                        //check orthoginality
                        f32 o1 = curveOrtho(d.curve, p, d.t),
                            o2 = curveOrtho(db.curve, p, db.t);

                        if (o2 < o1) goto set_b;
                    } else if (abs(d.d) < abs(db.d)) {
                    set_b:  
                        db = d;
                        eb = e;
                    }
                }
            }

            //auto cmp_dist = MinEdgeDist(p, glyphEdges);

            distIdx = x+y*regionW;
            const size_t mp = distIdx << 2;

            /*EdgeDistToPsuedoDist(dr);
            EdgeDistToPsuedoDist(dg);
            EdgeDistToPsuedoDist(db);*/
            d_cmp = mu_max(mu_max(abs(dr.d), abs(dg.d)), abs(db.d)); 
            testRadius = d_cmp + wc * 1.5f;

            dr = EdgePseudoDist(p, er);
            dg = EdgePseudoDist(p, eg);
            db = EdgePseudoDist(p, eb);           

            distBuffer[x + y * regionW] = vec3(dr.d,dg.d,db.d);

            //this is why i need that damn buffer
            //maxDists.x = mu_max(maxDists.x, abs(dr.d));
            //maxDists.y = mu_max(maxDists.y, abs(dg.d));
            //maxDists.z = mu_max(maxDists.z, abs(db.d));
        }

        //DO NOT FLIP THE ORDERS OF THESE OR SHIT WILL BREAK
        scanDx = (2 * !!xScanMin) - 1;
        xScanMax = regionW * !!xScanMin - 1 * !!xScanMax;
        xScanMin = regionW * !xScanMin - 1 * !xScanMin;
    }

    vec3 dv;

    //output the msdf to the bitmap
    for (y = 0; y < regionH; ++y) {
        for (x = 0; x < regionW; ++x) {

            //add regionX and regionY for the location
            distIdx = (x) + (y)*regionW;
            dv = distBuffer[distIdx];

            const size_t mp = ((x + regionX) + (y + regionY) * map->header.w) * nChannels;

            //make sure we aren't gonna render out of bounds
            if (mp + 3 >= map->header.fSz || mp < 0 || (x + regionX) < 0 || (y + regionY) < 0 || (x + regionX) >= map->header.w || (y + regionY) >= map->header.h)
                continue;

            constexpr f32 blend_after = 0.0f, blend_amount = 1.0f;

            dv.x -= blend_after;
            dv.y -= blend_after;
            dv.z -= blend_after;

            if (dv.x < 0)
                map->data[mp+0] = mu_max(mu_min((((dv.x) / blend_amount) * 0.5f + 0.5f) * 255.0f, 255.0f),0.0f);
            else
                map->data[mp+0] = 255;

            if (dv.y < 0)
                map->data[mp+1] = mu_max(mu_min((((dv.y) / blend_amount) * 0.5f + 0.5f) * 255.0f, 255.0f),0.0f);
            else
                map->data[mp+1] = 255;

            if (dv.z < 0)
                map->data[mp+2] = mu_max(mu_min((((dv.z) / blend_amount) * 0.5f + 0.5f) * 255.0f, 255.0f),0.0f);
            else
                map->data[mp+2] = 255;
            
            if (nChannels == 4)
                map->data[mp+3] = 255;
        }
    }

    delete[] distBuffer; //wow!