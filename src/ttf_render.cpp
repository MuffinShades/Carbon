#include "ttf_render.hpp"
#include "msutil.hpp"
#include "bitmap_render.hpp"
#include "logger.hpp"
#include "polynom.hpp"
#include "vec.hpp"
#include <vector>

#define MSFL_TTFRENDER_DEBUG

constexpr f32 smol_number = 1.175e-38f; //number that is smol

/**
 *
 * All le code for rendering dem glyphs
 *
 * Written by muffinshades 2024
 *
 */

Point pLerp(Point p0, Point p1, float t) {
    return {
        lerp(p0.x, p1.x, t),
        lerp(p0.y, p1.y, t)
    };
}

Point bezier3(Point p0, Point p1, Point p2, float t) {
    Point i0 = pLerp(p0, p1, t),
        i1 = pLerp(p1, p2, t);

    return pLerp(i0, i1, t);
}

Point bezier4(Point p0, Point p1, Point p2, Point p3, float t) {
    Point i0 = pLerp(p0, p1, t),
        i1 = pLerp(p1, p2, t),
        i2 = pLerp(p2, p3, t),
        ii0 = pLerp(i0, i1, t),
        ii1 = pLerp(i1, i2, t);

    return pLerp(ii0, ii1, t);
}

#include <cassert>

Point bezier(std::vector<Point> points, float t) {
    assert(points.size() >= 3);
    std::vector<Point>* i = new std::vector<Point>();
    std::vector<Point>* tg = &points;
    size_t itr = 0;
    do {
        const size_t tlp = tg->size() - 1;
        for (size_t p = 0; p < tlp; p++)
            i->push_back(
                pLerp((*tg)[p], (*tg)[p + 1], t)
            );
        if (itr > 0)
            delete tg;
        tg = i;
        i = new std::vector<Point>();
        itr++;
    } while (tg->size() > 1);

    Point r = (*tg)[0];
    delete tg, i;
    return r;
};

Point ScalePoint(Point p, float s) {
    return {
        p.x * s,
        p.y * s
    };
}

void DrawPoint(BitmapGraphics* g, float x, float y) {
    float xf = floor(x), xc = ceil(x),
        yf = floor(y), yc = ceil(y);

    float cIx = xc - x, fIx = x - xf,
        cIy = yc - y, fIy = y - yf;

    float i = ((cIx + fIx) / 2.0f + (cIy + fIy) / 2.0f) / 2.0f;

    i *= 255.0f;

    fIx *= 255.0f;
    fIy *= 255.0f;

    g->SetColor(fIx, fIx, fIx, 255);
    g->DrawPixel(x - 1, y);

    g->SetColor(cIx, cIx, cIx, 255);
    g->DrawPixel(x, y);
}

constexpr float epsilon = 0.0001f;
constexpr float invEpsilon = 1.0f / epsilon;
#define EPSILIZE(v) (floor((v)*invEpsilon)*epsilon)

struct f_roots {
    float r0 = 0.0f, r1 = 0.0f;
    i32 nRoots = 0;
};

/**
 *
 * getRoots
 *
 * returns the number of roots a
 * quadratic function has and their
 * values
 *
 */
f_roots getRoots(float a, float b, float c) {
    if (EPSILIZE(a) == 0.0f)
        return {
            .nRoots = 0
    };
    float root = EPSILIZE(
        (b * b) - (4.0f * a * c)
    ), ida = 1.0f / (2.0f * a);
    if (root < 0.0f)
        return {
            .nRoots = 0
    };
    root = EPSILIZE(sqrtf(root));
    if (root != 0.0f)
        return {
            .r0 = (-b + root) * ida,
            .r1 = (-b - root) * ida,
            .nRoots = 2
    };
    else
        return {
            .r0 = (-b + root) * ida,
            .nRoots = 1
    };
}

//verifys a given root is a valid intersection
bool _vRoot(float r, Point p0, Point p1, Point p2, Point e) {
    return r >= 0.0f && r <= 1.0f && bezier3(p0, p1, p2, r).x > e.x;
};

/**
 *
 * intersectsCurve
 *
 * function to determine if a given point, e
 * intersects a 3-points bezier curve denoted
 * by the points p0, p1, and p2
 *
 * Returns the # of interesctions that were made
 * 0, 1, or 2
 *
 */
i32 intersectsCurve(Point p0, Point p1, Point p2, Point e) {

    //offset points
    p0.y -= e.y;
    p1.y -= e.y;
    p2.y -= e.y;

    const float a = (p0.y - 2 * p1.y) + p2.y, b = 2 * p1.y - 2 * p0.y, c = p0.y;

    f_roots _roots = getRoots(a, b, c);

    if (_roots.nRoots <= 0) return 0; //no roots so no intersection

    //check le roots
    i32 nRoots = 0;

    if (_vRoot(_roots.r0, p0, p1, p2, e)) nRoots++;
    if (_roots.nRoots > 1 && _vRoot(_roots.r1, p0, p1, p2, e)) nRoots++;

    return nRoots;
}

struct gPData {
    std::vector<Point> p;
    std::vector<i32> f;
};

/**
 *
 * cleanGlyphPoints
 *
 * takes the raw points from a Glyph and
 * adds the implied points and contour ends
 * cleaning up the glyph making it much easier
 * to do the rendering.
 *
 */
gPData cleanGlyphPoints(Glyph& tGlyph) {
    gPData res;

    size_t currentContour = 0;

    if (!tGlyph.modifiedContourEnds) {
        //std::cout << "generating contour ends!" << std::endl;
        tGlyph.modifiedContourEnds = new i32[tGlyph.nContours];
        in_memcpy(tGlyph.modifiedContourEnds, tGlyph.contourEnds, sizeof(i32) * (tGlyph.nContours));
    }

    size_t i,c;

    //first add implied points
    for (i = 0; i < tGlyph.nPoints; i++) {
        res.p.push_back(tGlyph.points[i]);
        res.f.push_back(tGlyph.flags[i]);

        i32 pFlag = tGlyph.flags[i];
        Point p = tGlyph.points[i];

        if (i == tGlyph.contourEnds[currentContour] || i >= tGlyph.nPoints - 1) {
            size_t cPos = (currentContour > 0) ? tGlyph.contourEnds[currentContour - 1] + 1 : 0;
            assert(cPos < tGlyph.nPoints);

            //check for an implied point
            i32 flg = tGlyph.flags[cPos];
            bool oc = GetFlagValue(flg, PointFlag_onCurve);

            //add implied point in-between if needed
            if (oc == GetFlagValue(pFlag, PointFlag_onCurve)) {
                res.p.push_back(pLerp(p, tGlyph.points[cPos], 0.5f));
                res.f.push_back(ModifyFlagValue(pFlag, PointFlag_onCurve, !oc));
                for (c = currentContour; c < tGlyph.nContours; c++)
                    tGlyph.modifiedContourEnds[c]++; //adjust the conture end since an implied point is being added
            }

            //add contour end point
            res.p.push_back(tGlyph.points[cPos]);
            res.f.push_back(flg);

            for (c = currentContour; c < tGlyph.nContours; c++)
                tGlyph.modifiedContourEnds[c]++; //adjust the conture end since a contour end point is being added

            currentContour++;
#ifdef MSFL_TTFRENDER_DEBUG
            //std::cout << "Finished Contour: " << currentContour << " / " << tGlyph.nContours << std::endl;
#endif
            continue;
        }

        u32 oCurve;

        if (
            i < tGlyph.nPoints - 1 &&
            (oCurve = GetFlagValue(pFlag, PointFlag_onCurve)) == GetFlagValue(tGlyph.flags[i + 1], PointFlag_onCurve)
        ) {
            //add implied point
            res.p.push_back(pLerp(p, tGlyph.points[i + 1], 0.5f));
            res.f.push_back(ModifyFlagValue(pFlag, PointFlag_onCurve, !oCurve));

            for (c = currentContour; c < tGlyph.nContours; c++)
                tGlyph.modifiedContourEnds[c]++;
        }
    }

    return res;
}

/**
 *
 * ttfRender::RenderGlyphToBitmap
 *
 * renders a given ttfGlyph to a bitmap with
 * the r, g, b, a values being multipliers to
 * whatever given color you want to render the
 * text as
 *
 */
i32 ttfRender::RenderGlyphToBitmap(Glyph tGlyph, Bitmap* bmp, float scale) {
    return 0;
}

f32 pointCross(Point a, Point b) {
    return a.x * b.y - a.y * b.x; 
}

f32 pointCrossNormal(Point a, Point b) {
    return (a.x * b.y - a.y * b.x) / (sqrtf(a.x * a.x + a.y * a.y) * sqrtf(b.x * b.x + b.y * b.y));
}

f32 pointVecBetweenTheta(Point a, Point b) {
    return (a.x * b.x + a.y * b.y) / (sqrtf(a.x * a.x + a.y * a.y) * sqrtf(b.x * b.x + b.y * b.y));
}

struct CurveBounds {
    Point center;
    f32 r = -1;
};

struct BCurve {
    Point p[3];
    struct {
        f32 a_base = 0.0f, b_base = 0.0f, c_base = 0.0f, d_base = 0.0f;
        bool good = false;
    } solve_inf;
    struct {
        size_t ttf_relative_point_index[3]; //index of a given point stored in a ttf file
    } src_inf;
    CurveBounds bounds;
};

struct Edge {
    BCurve *curves = nullptr;
    size_t nCurves = 0;
    size_t final_point_index;
    size_t inital_point_index;
    uvec3 color = uvec3(0,0,0);
    CurveBounds bounds;
};

struct PDistInfo {
    f32 dx = INFINITY,dy = INFINITY,d = INFINITY,t = 0.0f;
    BCurve curve;
    Point p;
    struct {
        f32 roots[3];
        size_t nRoots;
    } root_inf;
    struct {
        f32 a,b,c,d;
    } cubic_dbg;
};

const PDistInfo pSquareDist(Point p0, Point p1) {
    PDistInfo inf;
    inf.dx = p1.x - p0.x;
    inf.dy = p1.y - p0.y;
    inf.d = inf.dx*inf.dx + inf.dy*inf.dy;
    return inf;
}

//derivative of a bezier 3
const Point dBdt3(Point p0, Point p1, Point p2, f32 t) {
    return {
        .x = 2.0f * ((1.0f - t) * (p1.x - p0.x) + t * (p2.x - p1.x)),
        .y = 2.0f * ((1.0f - t) * (p1.y - p0.y) + t * (p2.y - p1.y))
    };
}

const f32 compute_a_base_coord(f32 v0, f32 v1, f32 v2) {
    return 4.0f*v0*v0-16.0f*v0*v1+8.0f*v0*v2+16.0f*v1*v1-16.0f*v1*v2+4.0f*v2*v2;
}

const f32 compute_b_base_coord(f32 v0, f32 v1, f32 v2) {
    return -12.0f*v0*v0+24.0f*v0*v1-12.0f*v0*v2+12.0f*v0*v1-24.0f*v1*v1+12.0f*v1*v2;
}

const f32 compute_c_base_coord(f32 v0, f32 v1, f32 v2) {
    return 12.0f*v0*v0-24.0f*v0*v1+4.0f*v0*v2+8.0f*v1*v1;
}

const f32 compute_d_base_coord(f32 v0, f32 v1, f32 v2) {
    return -4.0f*v0*v0 + 4.0f*v0*v1;
}

const inline PDistInfo bezier3_point_dist(BCurve b, Point p, f32 t) {
    PDistInfo inf = {
        .dx = ((1.0f - t) * (1.0f - t) * b.p[0].x + 2.0f * (1.0f - t) * t * b.p[1].x + t * t * b.p[2].x) - p.x,
        .dy = ((1.0f - t) * (1.0f - t) * b.p[0].y + 2.0f * (1.0f - t) * t * b.p[1].y + t * t * b.p[2].y) - p.y,
        .t = t,
        .curve = b,
        .p = p
    };
    
    inf.d = inf.dx*inf.dx + inf.dy*inf.dy;

    return inf;
}

//TODO: optimize this function for straight edges (see master thesis)
PDistInfo EdgePointSignedDist(Point p, Edge& e) {
    if (!e.curves || e.nCurves == 0) return {.d=INFINITY};

    //constexpr size_t nCheckSteps = 256;
    //constexpr f64 dt = 1.0f / (f32) nCheckSteps;

    PDistInfo d = {
        .d = INFINITY
    };

    Point refPoint;

    i32 c,i;

    f32 roots[5] = {0,1,0,0,0};
    f32 *root_pass = roots + 2;

    Point p0,p1,p2;
    f32 a = -1.0f,b = -1.0f,f = -1.0f,g = -1.0f;
    f32 alpha,beta,gamma,t2,t2_i;
    f64 dx,dy,_D;

    f64 dx_best,dy_best,t_best,d_best = INFINITY;
    i32 best_c = -1;

    const size_t _NC = e.nCurves;

    for (c = 0; c < _NC; c++) {
        BCurve* tCurve = e.curves + c;

        p0 = tCurve->p[0]; p1 = tCurve->p[1]; p2 = tCurve->p[2];

        if (!tCurve->solve_inf.good) {
            //TODO: compute the a_base, b_base, and c_base
            // (bases are the terms computed on desmos that dont include the ref points)
            // these terms are grabbed from function I
            tCurve->solve_inf.a_base = compute_a_base_coord(p0.x, p1.x, p2.x) + compute_a_base_coord(p0.y, p1.y, p2.y);
            tCurve->solve_inf.b_base = compute_b_base_coord(p0.x, p1.x, p2.x) + compute_b_base_coord(p0.y, p1.y, p2.y);
            tCurve->solve_inf.c_base = compute_c_base_coord(p0.x, p1.x, p2.x) + compute_c_base_coord(p0.y, p1.y, p2.y);
            tCurve->solve_inf.d_base = compute_d_base_coord(p0.x, p1.x, p2.x) + compute_d_base_coord(p0.y, p1.y, p2.y);
            tCurve->solve_inf.good = true;
        }

        //when solving the min dist / roots --> optimize to use solve_re_cubic_32_b or solve_re_cubic_64_b

        const i32 nRoots = solve_re_cubic_32_a(
            a = (tCurve->solve_inf.a_base), 
            b = (tCurve->solve_inf.b_base),
            f = (tCurve->solve_inf.c_base 
                - 4.0f * (p0.y*p.y + p0.x*p.x)
                + 8.0f * (p1.y*p.y + p1.x*p.x)
                - 4.0f * (p2.y*p.y + p2.x*p.x)),
            g = (tCurve->solve_inf.d_base - 4.0f * (p1.y*p.y + p1.x*p.x) + 4.0f * (p0.y*p.y + p0.x*p.x)),
            root_pass
        );

        if (a != a || b != b || f != f || g != g) { 
            std::cout << "---------------------\nnan dbg: " << "\n";
            std::cout << a << " " << b << " " << f << " " << g << "\n";
            std::cout << p0.x << " " << p0.y << " | " << p1.x << " " << p1.y << " | " << p2.x << " " << p2.y << "\n";
            std::cout << tCurve->solve_inf.a_base << " " << tCurve->solve_inf.b_base << " " << tCurve->solve_inf.c_base << " " << tCurve->solve_inf.d_base << "|" << p.x << " " << p.y << "\n--------------\n";
        }

        //std::cout << nRoots << std::endl;

        for (i = 2; i < nRoots+2; i++) {
            t2 = (roots[i] > 1.0f) + (roots[i] >= 0.0f && roots[i] <= 1.0f) * roots[i];

            //pd = bezier3_point_dist(tCurve, p, t2);

            t2_i = 1.0f - t2;
            alpha = t2_i * t2_i;
            beta = 2.0f * t2_i * t2;
            gamma = t2 * t2;

            dx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p.x;
            dy = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p.y;
            _D = dx*dx + dy*dy;

            //std::cout << _D << "\n";
            
            if (_D < d_best) {
                dx_best = dx;
                dy_best = dy;
                best_c = c;
                t_best = t2;
                d_best = _D;

                in_memcpy(d.root_inf.roots, root_pass, sizeof(f32)*3);
                d.root_inf.nRoots = nRoots;
            }
        }
    }

    if (_NC == 0 || best_c == -1) {
        std::cout << _D << " " << d_best << " | " << a << " " << b << " " << f << " " << g << std::endl;

        return {
            .d = INFINITY
        };
    }

    const BCurve bestCurve = e.curves[best_c];

    d.dx = dx_best;
    d.dy = dy_best;
    d.t = t_best;
    d.p = p;
    d.curve = bestCurve;

    //compute the signed distance
    d.d = mu_sign(pointCross(
        dBdt3(bestCurve.p[0],bestCurve.p[1],bestCurve.p[2],t_best),
        {(f32) dx_best, (f32) dy_best}
    )) * sqrtf(d_best);

    return d;
}

CurveBounds computeCurveBoundingRadius(BCurve& curve, bool add_to_curve = true) {
    Point p0 = curve.p[0], p1 = curve.p[1], p2 = curve.p[2];

    const Point centroid = {
        .x = (p0.x + p2.x) * 0.5f,
        .y = (p0.y + p2.y) * 0.5f
    };

    //radius point 1
    const f32 r1 = sqrtf((centroid.x - p0.x) * (centroid.x - p0.x) + (centroid.y - p0.y) * (centroid.y - p0.y));

    //radius point 2
    f32 roots[5] = {0,1,0,0,0};
    f32 *root_pass = roots+2;

    if (!curve.solve_inf.good) {
        curve.solve_inf.a_base = compute_a_base_coord(p0.x, p1.x, p2.x) + compute_a_base_coord(p0.y, p1.y, p2.y);
        curve.solve_inf.b_base = compute_b_base_coord(p0.x, p1.x, p2.x) + compute_b_base_coord(p0.y, p1.y, p2.y);
        curve.solve_inf.c_base = compute_c_base_coord(p0.x, p1.x, p2.x) + compute_c_base_coord(p0.y, p1.y, p2.y);
        curve.solve_inf.d_base = compute_d_base_coord(p0.x, p1.x, p2.x) + compute_d_base_coord(p0.y, p1.y, p2.y);
        curve.solve_inf.good = true;
    }
    
    const i32 nRoots = solve_re_cubic_32_a(
        curve.solve_inf.a_base, 
        curve.solve_inf.b_base,
        curve.solve_inf.c_base 
                - 4.0f * (p0.y*p1.y + p0.x*p1.x)
                + 8.0f * (p1.y*p1.y + p1.x*p1.x)
                - 4.0f * (p2.y*p1.y + p2.x*p1.x),
        curve.solve_inf.d_base - 4.0f * (p1.y*p1.y + p1.x*p1.x) + 4.0f * (p0.y*p1.y + p0.x*p1.x),
        root_pass
    );

    f32 min_dist = INFINITY, bx, by, alpha, beta, gamma, t, t_i, _D;

    Point p3 = {
        .x = (centroid.x + p1.x) * 0.5f,
        .y = (centroid.y + p1.y) * 0.5f
    };

    for (i32 n = 0; n < nRoots + 2; n++) {
        t = roots[n];
        if (t < 0.0f || t > 1.0f) continue;

        t_i = 1.0f - t;
        alpha = t_i * t_i;
        beta = 2.0f * t_i * t;
        gamma = t * t;

        bx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p1.x;
        by = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p1.y;
        _D = bx*bx + by*by;

        if (_D < min_dist) {
            min_dist = _D;
            p3.x = bx;
            p3.y = by;
        }
    }

    const f32 r2 = sqrtf((centroid.x - p3.x) * (centroid.x - p3.x) + (centroid.y - p3.y) * (centroid.y - p3.y));

    //return greatest radius
    CurveBounds b = {
        .center = centroid,
        .r = mu_max(r1, r2)
    };

    if (add_to_curve)
        curve.bounds = b;
    
    return b;
}

CurveBounds computeEdgeBounds(Edge& e, bool add_to_edge = true) {
    f32 csumX = 0.0f, csumY = 0.0f;

    const size_t nc = e.nCurves;
    BCurve *cu;

    if (!e.curves || nc == 0)
        return {};\

    i32 c;

    for (c = 0; c < nc; c++) {
        cu = e.curves + c;

        if (cu->bounds.r < 0)
            computeCurveBoundingRadius(*cu);

        csumX += cu->bounds.center.x;
        csumY += cu->bounds.center.y;
    }

    const f32 is = 1.0f / (f32) nc;

    Point center = {.x = csumX * is, .y = csumY * is};

    CurveBounds b = {
        .center = center
    };

    //compute le radius
    f32 r = 0.0f;

    for (c = 0; c < nc; c++) {
        cu = e.curves + c;

        r = mu_max(
            cu->bounds.r + sqrtf(
                (cu->bounds.center.x - center.x)*(cu->bounds.center.x - center.x)+
                (cu->bounds.center.y - center.y)*(cu->bounds.center.y - center.y)
            )
        , r);
    }

    b.r = r;

    if (add_to_edge)
        e.bounds = b;

    return b;
}

struct EdgeDistInfo {
    Edge tEdge;
    size_t edgeIdx;
    PDistInfo signedDist;
    u32 dbgVal = 0;
};

Point pointNormalize(Point p) {
    const f32 f = 1.0f / sqrtf(p.x*p.x + p.y*p.y);

    return {
        .x = p.x * f,
        .y = p.y * f
    };
}

//compute orthoganality of a curve at a given point
f32 curveOrtho(BCurve c, Point p, f32 t) {
    const Point b = bezier3(c.p[0], c.p[1], c.p[2], t);
    return abs(pointCross(pointNormalize(
        dBdt3(c.p[0], c.p[1], c.p[2], t)
    ), pointNormalize(
        {p.x-b.x,p.y-b.y}
    )));
}

EdgeDistInfo MinEdgeDist(Point p, std::vector<Edge>& edges) {
    EdgeDistInfo inf = {
        .signedDist = INFINITY
    };

    f32 minAbsDist = INFINITY;
    f32 sd, ad;
    PDistInfo sdInf;
    i32 eIdx = 0;

    //TODO: turn this into just like a static buffer or something that is shared between all calls to MinEdgedist
    //this takes up 11% of the execution time!!!
    std::vector<EdgeDistInfo> duplicateEdges;

    for (Edge& e : edges) {
        sdInf = EdgePointSignedDist(p, e);
        sd = sdInf.d;
        ad = abs(sd);

        if (abs(ad - minAbsDist) <= 0.01f) {
            EdgeDistInfo dInf = {
                .tEdge = e,
                .edgeIdx = (size_t) eIdx,
                .signedDist = sdInf
            };

            duplicateEdges.push_back(dInf);
        } else if (ad < minAbsDist) {
            inf.tEdge = e;
            inf.edgeIdx = eIdx;
            inf.signedDist = sdInf;
            minAbsDist = ad;
            duplicateEdges.clear();
        }

        eIdx++;
    }

    const size_t nDuplicates = duplicateEdges.size();

    //if there are duplicate distances then we need to maximize orthogonality between edges
    if (nDuplicates > 0) {
        f32 maxOrtho = curveOrtho(inf.signedDist.curve, p, inf.signedDist.t), eOrtho;

        for (EdgeDistInfo e : duplicateEdges) {
            eOrtho = curveOrtho(e.signedDist.curve, p, e.signedDist.t);

            if (eOrtho > maxOrtho){
                inf = e;
                maxOrtho = eOrtho;
            }
        }
    }

    if (nDuplicates > 0) inf.dbgVal = 1;

    return inf;
}

EdgeDistInfo MinEdgeChromaDist(Point p, std::vector<Edge> edges, uvec3 chroma_select) {
    EdgeDistInfo inf = {
        .signedDist = INFINITY
    };

    f32 minAbsDist = INFINITY;
    f32 sd, ad;
    PDistInfo sdInf;
    i32 eIdx = 0;

    //TODO: turn this into just like a static buffer or something that is shared between all calls to MinEdgedist
    //this takes up 11% of the execution time!!!
    std::vector<EdgeDistInfo> duplicateEdges;

    for (Edge e : edges) {
        if (e.color.x * chroma_select.x + e.color.y * chroma_select.y + e.color.z * chroma_select.z == 0)
            continue;

        sdInf = EdgePointSignedDist(p, e);
        sd = sdInf.d;
        ad = abs(sd);

        if (abs(ad - minAbsDist) <= 0.01f) {
            EdgeDistInfo dInf = {
                .tEdge = e,
                .edgeIdx = (size_t) eIdx,
                .signedDist = sdInf
            };

            duplicateEdges.push_back(dInf);
        } else if (ad < minAbsDist) {
            inf.tEdge = e;
            inf.edgeIdx = eIdx;
            inf.signedDist = sdInf;
            minAbsDist = ad;
            duplicateEdges.clear();
        }

        eIdx++;
    }

    const size_t nDuplicates = duplicateEdges.size();

    //if there are duplicate distances then we need to maximize orthogonality between edges
    if (nDuplicates > 0) {
        f32 maxOrtho = curveOrtho(inf.signedDist.curve, p, inf.signedDist.t), eOrtho;

        for (EdgeDistInfo e : duplicateEdges) {
            eOrtho = curveOrtho(e.signedDist.curve, p, e.signedDist.t);

            if (eOrtho > maxOrtho){
                inf = e;
                maxOrtho = eOrtho;
            }
        }
    }

    if (nDuplicates > 0) inf.dbgVal = 1;

    return inf;
}

EdgeDistInfo MinEdgePseudoDist(Point p, std::vector<Edge> edges) {
    EdgeDistInfo e = MinEdgeDist(p, edges);

    //compute the pseudo distance
    PDistInfo dist = e.signedDist;

    i32 i;

    //recompute with high derivative
    if (dist.t >= 1.0f) {
        PDistInfo pd, gDist = dist;

        for (i = 0; i < dist.root_inf.nRoots; ++i) {
            if (dist.root_inf.roots[i] > 1.0f) {
                pd = bezier3_point_dist(dist.curve, p, dist.root_inf.roots[i]);

                if (pd.d < gDist.d)
                    gDist = pd;
            }
        }

        dist = gDist;
    } 
    //recompute with lower derivative
    else if (dist.t <= 0.0f) {
        f32 gRoot = dist.t;

        PDistInfo pd, gDist = dist;

        for (i = 0; i < dist.root_inf.nRoots; ++i) {
            if (dist.root_inf.roots[i] < 0.0f) {
                pd = bezier3_point_dist(dist.curve, p, dist.root_inf.roots[i]);

                if (pd.d < gDist.d)
                    gDist = pd;
            }
        }

        dist = gDist;
    }

    e.signedDist = dist;

    return e;
}


PDistInfo EdgePseudoDist(Point p, Edge edge) {
    PDistInfo dist = EdgePointSignedDist(p, edge);

    i32 i;

    //recompute with high derivative
    if (dist.t >= 1.0f) {
        PDistInfo pd, gDist = dist;

        for (i = 0; i < dist.root_inf.nRoots; ++i) {
            if (dist.root_inf.roots[i] > 1.0f) {
                pd = bezier3_point_dist(dist.curve, p, dist.root_inf.roots[i]);

                if (pd.d < gDist.d)
                    gDist = pd;
            }
        }

        dist = gDist;
    } 
    //recompute with lower derivative
    else if (dist.t <= 0.0f) {
        f32 gRoot = dist.t;

        PDistInfo pd, gDist = dist;

        for (i = 0; i < dist.root_inf.nRoots; ++i) {
            if (dist.root_inf.roots[i] < 0.0f) {
                pd = bezier3_point_dist(dist.curve, p, dist.root_inf.roots[i]);

                if (pd.d < gDist.d)
                    gDist = pd;
            }
        }

        dist = gDist;
    }

    return dist;
}

std::vector<Edge> generateGlyphEdges(Glyph glyph_data, gPData& points, size_t nPoints) {
    std::vector<Edge> glyphEdges;
    
    const size_t nCurves = nPoints >> 1;
    BCurve *curveBuffer = new BCurve[nCurves]; //stores curves of current edge
    ZeroMem(curveBuffer, nCurves);

    BCurve *workingCurve = curveBuffer;

    i32 i;
    i32 pSelect = 0, nEdgeCurves = 0;
    Point nextPoint, prevPoint, p;
    f32 cross;
    bool workingOnACurve = false, pOnCurve;
    Edge newEdge;

    size_t cur_contour = 0;

    size_t beg_p_idx;

    //clean check
    bool onExpect, oc;

    size_t cleanSteps = 1, cContCleanCheck = 0;

    _clean_glyph_data:

    onExpect = true;
    for (i = 0; i < nPoints; i++) {
        oc = GetFlagValue(points.f[i], PointFlag_onCurve);

        //contour stuff
        if (cContCleanCheck < glyph_data.nContours && i == glyph_data.modifiedContourEnds[cContCleanCheck]) {
            onExpect = true;
            cContCleanCheck++;
            continue;
        }

        if ((onExpect && oc) || (!onExpect && !oc)) 
            onExpect = !onExpect;
        else {
            //clean check failed so clean the points
            if (cleanSteps > 0) {
                std::cout << "Initial check failed!" << std::endl;
                points = cleanGlyphPoints(glyph_data);
                cleanSteps--;
                goto _clean_glyph_data;
            } else {
                std::cout << "Clean check failed!" << std::endl;
                return std::vector<Edge>();
            }
        }
    }

    beg_p_idx = 0;

    for (i = 0; i < nPoints; i++) {
        p = points.p[i];
        workingCurve->p[pSelect] = p;

        if (pSelect++ == 2) {
            //add curve to curve buffer / edge curves
            nEdgeCurves++;
            workingCurve = curveBuffer + nEdgeCurves; //set the working curve to the next curve
            pSelect = 0;

            //snipping on a curve won't happpen on a control point
            if (!GetFlagValue(points.f[i], PointFlag_onCurve))
                continue;

            //check to see if a new edge is needed
            //snip is a 2 bit boolean type thing
            //bit one is whether or not to snip
            //bit two is whether or not the end of a contour was reached
            byte snip = 0;

            if (i > 1 && i < nPoints - 1) {
                Point controlPoint1 = points.p[i-1],
                      controlPoint2 = points.p[i+1];

                //cross product between the vectors formed by the control points and the shared point between curves
                //the control points will be in line if it is a spline, else there will be somewhat of a corner
                //due to this property there is no need to compute the curves' derivatives and cross product between those vectors
                //cross product is also normalized here cause it's easier to include very close splines that mineaswell be splines
                cross = pointCrossNormal({
                    p.x - controlPoint1.x,
                    p.y - controlPoint1.y
                }, {
                    controlPoint2.x - p.x,
                    controlPoint2.y - p.y
                });

                //snip if the cross product is not close to zero (which means it's a sharp corner)
                snip = snip || (abs(cross) > 0.1f);
            }

            if (cur_contour < glyph_data.nContours && i == glyph_data.modifiedContourEnds[cur_contour]) {
                snip = 0b11;
                cur_contour++;
            }

            snip = snip | ((i == nPoints - 1) & 1); //snip the last point :3

            //snip the curve and construct a new edge
            if (snip & 1) {
                Edge e;

                e.curves = curveBuffer;
                e.nCurves = nEdgeCurves;
                e.inital_point_index = beg_p_idx;
                e.final_point_index = i;

                glyphEdges.push_back(e);

                //reset for next edge
                curveBuffer = new BCurve[nCurves];
                ZeroMem(curveBuffer, nCurves);
                workingCurve = curveBuffer;
                nEdgeCurves = 0;
                beg_p_idx = i+1;
            }

            if (!(snip & 0b10)) {
                i--; //go back since curves will end up sharing points
                beg_p_idx--;
            }
        }
    }

    //delete whatever is left in le curve buffer
    _safe_free_a(curveBuffer);

    //return :3
    return glyphEdges;
}

i32 ttfRender::RenderGlyphSDFToBitMap(Glyph tGlyph, Bitmap* map, sdf_dim size) {
    if (!map)
        return 1;

    //do some dimension calculations
    u32 sdfW = 0, sdfH = 0, sdfTrueW = 0, sdfTrueH = 0;

    const u32 padding = 1;

    const f32 glyphW = tGlyph.xMax - tGlyph.xMin,
              glyphH = tGlyph.yMax - tGlyph.yMin,
              glyphYxRatio = glyphH / glyphW;

    switch (size.slc) {
    case sdf_dim_ty::Width:
        map->header.w = (sdfW = size.m.w);
        map->header.h = (sdfH = (size_t) ceil(sdfW * glyphYxRatio));
        break;
    case sdf_dim_ty::Height:
        map->header.h = (sdfH = size.m.h);
        map->header.w = (sdfW = (size_t) ceil(sdfH / glyphYxRatio));
        break;
    case sdf_dim_ty::Scale:
        map->header.h = (sdfH = size.m.scale * glyphH);
        map->header.w = (sdfW = size.m.scale * glyphW);
        break;
    }

    sdfTrueW = sdfW + (padding << 1);
    sdfTrueH = sdfH + (padding << 1);

    map->header.h = sdfTrueH;

    //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    map->header.bitsPerPixel = 32;
    map->header.fSz = (map->header.h * map->header.w) * 4;

    map->data = new byte[map->header.fSz];
    ZeroMem(map->data, map->header.fSz);

    //blank glyph so blank sdf
    if (nPoints == 0)
        return 0;

    //curve and edge generation, glyph clean up, and more

    //generate glyph edges
    std::vector<Edge> glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

    //generate single channel sdf
    i32 x,y;

    const f32
        wc = glyphW / (f32) sdfTrueW,
        hc = glyphH / (f32) sdfTrueH;
        //maxPossibleDist = sqrtf(glyphW*glyphW + glyphH*glyphH);

    byte color;

    f32 *distBuffer = new f32[sdfW * sdfH]; //i hate this so much

    f32 maxDist = smol_number; //smol number
    f32 d;

    Point p;

    for (y = 0; y < sdfH; ++y) {
        for (x = 0; x < sdfW; ++x) {
            p.x = (((f32)x) + 0.5f) * wc + tGlyph.xMin;
            p.y = (((f32)y) + 0.5f) * hc + tGlyph.yMin;

            EdgeDistInfo fieldDist = MinEdgeDist(p, glyphEdges);

            distBuffer[x + y * sdfW] = (d = fieldDist.signedDist.d);

            //this is why i need that damn buffer
            d = abs(d);
            maxDist = mu_max(maxDist, d);
        }
    }

    size_t distIdx;

    //i dont want to do it again but maxPossibleDist is being a bitch
    for (y = 0; y < sdfH; ++y) {
        for (x = 0; x < sdfW; ++x) {
            distIdx = x+y*sdfW;
            d = distBuffer[distIdx];

            color = mu_min(mu_max(
                    ((d / maxDist) + 0.5f) * 255.0f
            ,0),255);

            const size_t mp = distIdx << 2;

            if (mp + 3 >= map->header.fSz)
                continue;

            map->data[mp+0] = color;
            map->data[mp+1] = color;
            map->data[mp+2] = color;
            map->data[mp+3] = 255;
        }
    }

    delete[] distBuffer; //wow!

    //oh look were managing memory :O
    for (Edge e : glyphEdges) {
        _safe_free_a(e.curves);
        e.nCurves = 0;
    }

    return 0;
}

f32 smoothstep(f32 t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t*t*(3.0f - 2.0f*t);
}

//simple bilinear sample function or whatever i have i my head
/*

How to use this function:

buffer --> buffer normally from the sdf's bitmap of the pixels in the sdf or image
nchannels --> how many channels there are (assumes 1 byte per channel)
bufW --> how wide is the sample image in px
bufH --> how tall is the sample image in px
sampX --> a percent value of where to sample the buffer from (sampX = xpos of outimage / outimage w)
sampY --> a percent value of where to sample the buffer from (sampY = ypos of outimage / outimage h)

*/
#define MAX_BINLINEAR_CHANNELS 4
void sampleBilinear(byte *buffer, byte *out, size_t nchannels, size_t bufW, size_t bufH, f32 sampX, f32 sampY) {
    //                                     OO <-- Guy she told you not to worry about
    if (!buffer                            || 
        !out                               || 
        nchannels == 0                     || 
        nchannels > MAX_BINLINEAR_CHANNELS || 
        bufW == 0                          || 
        bufH == 0                          || 
        sampX < 0                          || 
        sampY < 0                          || 
        sampX > 1.0f                       || 
        sampY > 1.0f                     //V
    )
        return;

    const f32 sx = sampX * (f32) bufW,
              sy = sampY * (f32) bufH;

    i32 lx = floor(sx), rx = lx + 1,
        ly = floor(sy), ry = ly + 1;

    f32 subX = rx - sx,
        subY = ry - sy;

    if (rx >= bufW) {
        rx = bufW - 1;
        subX = 0;
    }

    if (ry >= bufH) {
        ry = bufH - 1;
        subY = 0;
    }

    if (lx < 0) {
        rx = 0;
        subX = 0;
    }

    if (ly < 0)  {
        ly = 0;
        subY = 0;
    }

    const size_t tlp = (lx + ly * bufW) * nchannels,
                 trp = (rx + ly * bufW) * nchannels,
                 blp = (lx + ry * bufW) * nchannels,
                 brp = (rx + ry * bufW) * nchannels;

    f32 ht, hb;

    i32 i = 0;

    byte tlv,trv,blv,brv;

    //horrizontal blur
    for (i = 0; i < nchannels; i++) {
        if (lx == 0 && ly != 0) tlv = buffer[trp];
        else if (ly == 0 && lx != 0) tlv = buffer[blp];
        else if (lx == 0 && ly == 0) tlv = buffer[brp];
        else tlv = buffer[tlp+i]; 

        if (rx < bufW && ly == 0) trv = buffer[brp];
        else if (rx >= bufW && ly != 0) trv = buffer[tlp];
        else if (rx >= bufW && ly == 0) trv = buffer[blp];
        else trv = buffer[trp+i];

        if (ry < bufH && lx == 0) blv = buffer[brp];
        else if (ry >= bufH && lx != 0) blv = buffer[tlp];
        else if (ry >= bufH && lx == 0) blv = buffer[trp];
        else blv = buffer[blp+i]; 

        if (ry < bufH && rx >= bufW) brv = buffer[blp];
        else if (ry >= bufH && rx < bufW) brv = buffer[trp];
        else if (ry >= bufH && rx >= bufW) brv = buffer[blp];
        else brv = buffer[brp+i];

        hb = lerp(trv, tlv, subX);
        ht = lerp(brv, blv, subX);
        out[i] = lerp(ht, hb, subY);
    }
}

i32 ttfRender::RenderSDFToBitmap(Bitmap* sdf, Bitmap* bmp, sdf_dim res_size) {
    if (!sdf || !bmp) return 1;

    Bitmap::Free(bmp);

    if (!sdf->data) return 2;
    if (sdf->header.bitsPerPixel < 24) return 3;

    bmp->header = sdf->header;

    u32 outW = 0, outH = 0;

    const f32 yxr = (f32) sdf->header.h / (f32) sdf->header.w;

    switch (res_size.slc) {
    case sdf_dim_ty::Width:
        bmp->header.w = (outW = res_size.m.w);
        bmp->header.h = (outH = (size_t) ceil(outW * yxr));
        break;
    case sdf_dim_ty::Height:
        bmp->header.h = (outH = res_size.m.h);
        bmp->header.w = (outW = (size_t) ceil(outH / yxr));
        break;
    case sdf_dim_ty::Scale:
        bmp->header.h = (outH = res_size.m.scale * sdf->header.h);
        bmp->header.w = (outW = res_size.m.scale * sdf->header.w);
        break;
    }


    const size_t by_pp = sdf->header.bitsPerPixel >> 3;

    bmp->header.fSz = bmp->header.w * bmp->header.h * by_pp;
    bmp->data = new byte[bmp->header.fSz];
    ZeroMem(bmp->data, bmp->header.fSz);

    i32 x, y;
    size_t p;

    byte samp[4] = {0,0,0,0};

    for (y = 0; y < outH; ++y) {
        for (x = 0; x < outW; ++x) {
            p = (x + y * outW) * by_pp;
                
            //do the bilinear sampling here
            sampleBilinear(sdf->data, samp, sdf->header.bitsPerPixel >> 3, sdf->header.w, sdf->header.h, 
                ((f32) x + 0.5f) / (f32) outW,
                ((f32) y + 0.5f) / (f32) outH
            );

            if ((i32) samp[0] - 127 > 0)
                forrange(by_pp-1)
                    bmp->data[p+i] = 255;
            //else
                //bmp->data[p+1] = samp[0];

            bmp->data[p+by_pp-1] = 0xff;        
        }
    }

    return 0;
}

i32 ttfRender::RenderGlyphPseudoSDFToBitMap(Glyph tGlyph, Bitmap* map, sdf_dim size) {
    if (!map)
        return 1;

    //do some dimension calculations
    u32 sdfW = 0, sdfH = 0;

    const f32 glyphW = tGlyph.xMax - tGlyph.xMin,
              glyphH = tGlyph.yMax - tGlyph.yMin,
              glyphYxRatio = glyphH / glyphW;

    switch (size.slc) {
    case sdf_dim_ty::Width:
        map->header.w = (sdfW = size.m.w);
        map->header.h = (sdfH = (size_t) ceil(sdfW * glyphYxRatio));
        break;
    case sdf_dim_ty::Height:
        map->header.h = (sdfH = size.m.h);
        map->header.w = (sdfW = (size_t) ceil(sdfH / glyphYxRatio));
        break;
    case sdf_dim_ty::Scale:
        map->header.h = (sdfH = size.m.scale * glyphH);
        map->header.w = (sdfW = size.m.scale * glyphW);
        break;
    }

    //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    map->header.bitsPerPixel = 32;
    map->header.fSz = map->header.h * map->header.w * 4;

    byte *bmpData = new byte[map->header.fSz];
    ZeroMem(bmpData, map->header.fSz);

    //blank glyph so blank sdf
    if (nPoints == 0)
        return 0;

    map->data = bmpData;

    //curve and edge generation, glyph clean up, and more

    std::vector<Edge> glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

    //generate single channel sdf
    i32 x,y;

    const f32
        wc = glyphW / (f32) sdfW,
        hc = glyphH / (f32) sdfH;
        //maxPossibleDist = sqrtf(glyphW*glyphW + glyphH*glyphH);

    byte color;

    f32 *distBuffer = new f32[sdfW * sdfH]; //i hate this so much

    f32 maxDist = smol_number; //smol number
    f32 d;

    Point p;

    for (y = 0; y < sdfH; ++y) {
        for (x = 0; x < sdfW; ++x) {
            p.x = (((f32)x) + 0.5f) * wc + tGlyph.xMin;
            p.y = (((f32)y) + 0.5f) * hc + tGlyph.yMin;

            EdgeDistInfo fieldDist = MinEdgePseudoDist(p, glyphEdges);

            distBuffer[x + y * sdfW] = (d = fieldDist.signedDist.d);

            //this is why i need that damn buffer
            d = abs(d);
            maxDist = mu_max(maxDist, d);
        }
    }

    size_t distIdx;

    //i dont want to do it again but maxPossibleDist is being a bitch
    for (y = 0; y < sdfH; ++y) {
        for (x = 0; x < sdfW; ++x) {
            distIdx = x+y*sdfW;
            d = distBuffer[distIdx];

            color = mu_min(
                mu_max(
                    (d / /*maxPossibleDist*/ (maxDist*2)) * 128.0f + 127, 0),
            255);

            const size_t mp = distIdx << 2;

            if (mp + 3 >= map->header.fSz)
                continue;

            map->data[mp+0] = color;
            map->data[mp+1] = color;
            map->data[mp+2] = color;
            map->data[mp+3] = 255;
        }
    }

    delete[] distBuffer; //wow!

    //oh look were managing memory :O
    for (Edge e : glyphEdges) {
        _safe_free_a(e.curves);
        e.nCurves = 0;
    }

    return 0;
}

//snake optimized msdf min signed dist
/*

The algorithm is goofy as fuck

Just kinda conjured it from the depths of my mind

*/
i32 msdf_msd_snake_opt() {
    return 0;
}

struct Contour {
    std::vector<size_t> edge_idxs;
    size_t minPoint, maxPoint;
};

i32 render_positioned_msdf(Glyph& tGlyph, Bitmap* map, const i32 regionX, const i32 regionY, const u32 regionW, const u32 regionH, const u32 paddingLeft, const u32 paddingTop, const u32 paddingRight, const u32 paddingBottom) {
    //simple error / render ability checks
    if (regionW == 0 || regionH == 0) 
        return 0;

    if (map->header.w == 0 || map->header.h == 0)
        return 0;

    if (!map->data) {
        return 1;
    }

    //
    const size_t nChannels = map->header.bitsPerPixel >> 3;

     //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    //blank glyph so blank sdf
    if (nPoints == 0)
        return 0;

    //curve and edge generation, glyph clean up, and more

    std::vector<Edge> glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

    //compute conture colors
    i32 c;

    u32 edge_i = 0;
    Edge cur_edge, next_edge;
    bool final_edge = false;
    size_t ncontour_edges = 0;

    const size_t nGlyphEdges = glyphEdges.size();

    Contour *glyph_contours = new Contour[tGlyph.nContours];
    u32 cur_min_idx = 0, e_idx = 0;

    for (c = 0; c < tGlyph.nContours; c++) { //oh my fucking god C++???!?!?!?! No way!!! :O
        Contour *cur_c = glyph_contours + c;

        //compute min and max points
        cur_c->minPoint = cur_min_idx;
        cur_c->maxPoint = tGlyph.modifiedContourEnds[c] + cur_c->minPoint;

        cur_min_idx += tGlyph.modifiedContourEnds[c];

        //assign edges
        e_idx = 0;
        for (Edge e : glyphEdges) {
            if (
                (e.inital_point_index >= cur_c->minPoint && e.inital_point_index < cur_c->maxPoint) ||
                (e.final_point_index > cur_c->minPoint && e.final_point_index <= cur_c->maxPoint)
            ) {
                cur_c->edge_idxs.push_back(e_idx);
            }

            e_idx++;
        }
    }

    //now color le edges
    i32 i;

    uvec3 cur_color;

    for (c = 0; c < tGlyph.nContours; c++) {
        Contour ct = glyph_contours[c];

        const size_t ncEdges = ct.edge_idxs.size();
        u32 t_edge;

        if (ncEdges == 0)
            continue;
        else if (ncEdges == 1) {
            t_edge = ct.edge_idxs[0];
            glyphEdges[t_edge].color = uvec3(1,1,1);
            continue;
        }

        cur_color = uvec3(1,0,1);

        for (i = 0; i < ncEdges; i++) {
            t_edge = ct.edge_idxs[i];

            glyphEdges[t_edge].color = cur_color;

            if (cur_color.y == 0) {
                cur_color.y = 1;
                cur_color.z = 0;
            } else if (cur_color.x == 1 && cur_color.y == 1) {
                cur_color.x = 0;
                cur_color.z = 1;
            } else {
                cur_color.x = 1;
                cur_color.z = 0;
            }

            //compute edge bounds too
            computeEdgeBounds(glyphEdges[t_edge]);

            edge_i++;
        }
    }

    //generate multi channel sdf
    i32 x,y;

    const f32 glyphW = tGlyph.xMax - tGlyph.xMin, glyphH = tGlyph.yMax - tGlyph.yMin;

    u32 paddingX = paddingLeft + paddingRight,
        paddingY = paddingTop + paddingBottom;

    while (regionW <= paddingX && paddingX >= 2)
        paddingX -= 2;

    while (regionH <= paddingY && paddingY >= 2)
        paddingY -= 2;

    if (regionW <= paddingX || regionH <= paddingY || paddingX < 0 || paddingY < 0)
        return 1; //no room ;-;

    const f32
        wc = glyphW / (f32) (regionW - paddingX),
        hc = glyphH / (f32) (regionH - paddingY);

    byte color;

    vec3 *distBuffer = new vec3[regionW * regionH]; //i hate this so much

    f32 maxDist = smol_number; //smol number
    PDistInfo dr, dg, db;
    f32 d_cmp;
    vec3 maxDists;

    PDistInfo d;

    Edge er, eg, eb;

    Point p;

    size_t distIdx;

    //curve check index buffer
    BCurve *ccib = nullptr;

    i32 testRadius = -1;

    //generate the msdf
    for (y = 0; y < regionH; ++y) {
        for (x = 0; x < regionW; ++x) {
            p.x = (((f32)x - paddingLeft) + 0.5f) * wc + (tGlyph.xMin);
            p.y = (((f32)y - paddingTop) + 0.5f) * hc + (tGlyph.yMin);
            
            dr.d = dg.d = db.d = INFINITY;

            for (Edge e : glyphEdges) {
                //big optimizing :3
                if (testRadius > -1 &&
                    (p.x - e.bounds.center.x) * (p.x - e.bounds.center.x) +
                    (p.y - e.bounds.center.y) * (p.y - e.bounds.center.y)
                > (testRadius + e.bounds.r) * (testRadius + e.bounds.r))
                    continue;

                d = EdgePointSignedDist(p, e);

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

            testRadius = mu_max(mu_max(dr.d, dg.d), db.d) + wc;

            //auto cmp_dist = MinEdgeDist(p, glyphEdges);

            distIdx = x+y*regionW;
            const size_t mp = distIdx << 2;

            dr.d = EdgePseudoDist(p, er).d;
            dg.d = EdgePseudoDist(p, eg).d;
            db.d = EdgePseudoDist(p, eb).d;

            d_cmp = mu_max(mu_max(abs(dr.d), abs(dg.d)), abs(db.d));            

            distBuffer[x + y * regionW] = vec3(dr.d,dg.d,db.d);

            //this is why i need that damn buffer
            maxDists.x = mu_max(maxDists.x, abs(dr.d));
            maxDists.y = mu_max(maxDists.y, abs(dg.d));
            maxDists.z = mu_max(maxDists.z, abs(db.d));
        }
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
    _safe_free_a(glyph_contours); //more memory management

    //oh look were managing memory :O
    for (Edge e : glyphEdges) {
        _safe_free_a(e.curves);
        e.nCurves = 0;
    }

    return 0;
}

i32 ttfRender::RenderGlyphMSDFToBitMap(Glyph tGlyph, Bitmap* map, sdf_dim size) {
    if (!map)
        return 1;

    //do some dimension calculations
    u32 sdfW = 0, sdfH = 0, sdfTrueW = 0, sdfTrueH = 0;

    const u32 padding = 4;

    const f32 glyphW = tGlyph.xMax - tGlyph.xMin,
              glyphH = tGlyph.yMax - tGlyph.yMin,
              glyphYxRatio = glyphH / glyphW;

    switch (size.slc) {
    case sdf_dim_ty::Width:
        sdfW = size.m.w;
        sdfH = (size_t) ceil(sdfW * glyphYxRatio);
        break;
    case sdf_dim_ty::Height:
        sdfH = size.m.h;
        sdfW = (size_t) ceil(sdfH / glyphYxRatio);
        break;
    case sdf_dim_ty::Scale:
        sdfH = size.m.scale * glyphH;
        sdfW = size.m.scale * glyphW;
        break;
    }

    sdfTrueW = sdfW + (padding << 1);
    sdfTrueH = sdfH + (padding << 1);

    map->header.h = sdfTrueH;
    map->header.w = sdfTrueW;

    const size_t nChannels = 3;

    map->header.bitsPerPixel = (nChannels << 3);
    map->header.fSz = (map->header.h * map->header.w) * nChannels;

    map->data = new byte[map->header.fSz];
    ZeroMem(map->data, map->header.fSz);

    //render the msdf to the canvas
    return render_positioned_msdf(tGlyph, map, 0, 0, sdfTrueW, sdfTrueH, 10, 10, 10, 10);
}

f32 median(f32 a, f32 b, f32 c) {
    return mu_max(mu_min(a,b), mu_min(mu_max(a,b),c));
}

i32 ttfRender::RenderMSDFToBitmap(Bitmap* sdf, Bitmap* bmp, sdf_dim res_size) {
    if (!sdf || !bmp) return 1;

    Bitmap::Free(bmp);

    if (!sdf->data) return 2;
    if (sdf->header.bitsPerPixel < 24) return 3;

    bmp->header = sdf->header;

    u32 outW = 0, outH = 0;

    const f32 yxr = (f32) sdf->header.h / (f32) sdf->header.w;

    switch (res_size.slc) {
    case sdf_dim_ty::Width:
        bmp->header.w = (outW = res_size.m.w);
        bmp->header.h = (outH = (size_t) ceil(outW * yxr));
        break;
    case sdf_dim_ty::Height:
        bmp->header.h = (outH = res_size.m.h);
        bmp->header.w = (outW = (size_t) ceil(outH / yxr));
        break;
    case sdf_dim_ty::Scale:
        bmp->header.h = (outH = res_size.m.scale * sdf->header.h);
        bmp->header.w = (outW = res_size.m.scale * sdf->header.w);
        break;
    }


    const size_t by_pp = sdf->header.bitsPerPixel >> 3;

    bmp->header.fSz = bmp->header.w * bmp->header.h * by_pp;
    bmp->data = new byte[bmp->header.fSz];
    ZeroMem(bmp->data, bmp->header.fSz);

    i32 x, y;
    size_t p;

    byte samp[4] = {0,0,0,0};

    for (y = 0; y < outH; ++y) {
        for (x = 0; x < outW; ++x) {
            p = (x + y * outW) * by_pp;

                
            //do the bilinear sampling here
            sampleBilinear(sdf->data, samp, sdf->header.bitsPerPixel >> 3, sdf->header.w, sdf->header.h, 
                ((f32) x + 0.5f) / (f32) outW,
                ((f32) y + 0.5f) / (f32) outH
            );

            f32 g = median(samp[0], samp[1], samp[2]) / 255.0f - 0.5f;

            if (g > 0.0f)
                bmp->data[p] = 0xff;
            //else
                //bmp->data[p+1] = samp[0];

            bmp->data[p+by_pp-1] = 0xff;        
        }
    }

    return 0;
}

i32 ttfRender::RenderGlyphOutlineToBitmap(Glyph tGlyph, Bitmap* map, sdf_dim size) {
    if (!map)
        return 1;

    //do some dimension calculations
    u32 sdfW = 0, sdfH = 0, sdfTrueW = 0, sdfTrueH = 0;

    const u32 padding = 0;

    const f32 glyphW = tGlyph.xMax - tGlyph.xMin,
              glyphH = tGlyph.yMax - tGlyph.yMin,
              glyphYxRatio = glyphH / glyphW;

    switch (size.slc) {
    case sdf_dim_ty::Width:
        map->header.w = (sdfW = size.m.w);
        map->header.h = (sdfH = (size_t) ceil(sdfW * glyphYxRatio));
        break;
    case sdf_dim_ty::Height:
        map->header.h = (sdfH = size.m.h);
        map->header.w = (sdfW = (size_t) ceil(sdfH / glyphYxRatio));
        break;
    case sdf_dim_ty::Scale:
        map->header.h = (sdfH = size.m.scale * glyphH);
        map->header.w = (sdfW = size.m.scale * glyphW);
        break;
    }

    sdfTrueW = sdfW + (padding << 1);
    sdfTrueH = sdfH + (padding << 1);

    std::cout << "padding extra: " << (sdfTrueW - sdfW) << " | " << (padding << 1) << " | " << sdfTrueW << " " << sdfW << std::endl;

    map->header.h = sdfTrueH;

    //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    map->header.bitsPerPixel = 32;
    map->header.fSz = (map->header.h * map->header.w) * 4;

    map->data = new byte[map->header.fSz];
    ZeroMem(map->data, map->header.fSz);

    //blank glyph so blank sdf
    if (nPoints == 0)
        return 0;

    //curve and edge generation, glyph clean up, and more

    std::vector<Edge> glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

    f32 t;

    const f32
        wc = glyphW / (f32) sdfTrueW,
        hc = glyphH / (f32) sdfTrueH;

    u32 pos;

    for (Edge e : glyphEdges) {
        for (i32 c = 0; c < e.nCurves; c++) {
            for (t = 0.0f; t < 1.0f; t += 0.005f) {
                Point p = bezier3(e.curves[c].p[0],e.curves[c].p[1],e.curves[c].p[2],t);
                p.x = (p.x - tGlyph.xMin) * (1.0f/wc);
                p.y = (p.y - tGlyph.yMin) * (1.0f/hc);

                if (p.x < 0 || p.y < 0 || p.x >= map->header.w || p.y >= map->header.h) continue;

                pos = ((u32)p.x + (u32)p.y * map->header.w) * 4;

                map->data[pos] = 255;
                map->data[pos+1] = 255;
                map->data[pos+2] = 255;
                map->data[pos+3] = 255;
            }
        }
    }

    return 0;
}

template<class _Ty> void mu_swap(_Ty *a, _Ty *b) {
    _Ty temp = *a;
    *a = *b;
    *b = temp;
}

template<class _Ty> i32 _partition(_Ty *arr, i32 (*cmp)(_Ty, _Ty), i32 low, i32 high) {
    _Ty p = arr[low];

    i32 i = low, j = high;

    while (i < j) {
        while (cmp(arr[i], p) <= 0 && i <= high - 1) i++;
        while (cmp(arr[j], p) > 0 && j >= low + 1) j--;
        if (i < j)
            mu_swap(arr + i, arr + j);
    }

    mu_swap(arr + low, arr + high);

    return j;
}

//quick sort function
//cmp is a lambda that compares the two values given to it
// ie [](Obj a, Obj b) return i32;
//returning a 0 is equivalent to the values being equal
//returning < 0 is equivalent to value a being less then value b
//returning > 0 is equivalent to value a being greater then value b
//Ie: sort from least to greatest --> return a-b
//Ie: sort from greatest to least --> return b-a
template<class _Ty> void mu_qsort(_Ty *arr, i32 (*cmp)(_Ty, _Ty), size_t len, i32 _low = 0, i32 _high = -1) {
    if (!arr || _low > _high)
        return;

    if (_high < 0 || _high >= len) _high = len - 1;
    if (_low < 0) _low = 0;
    
    i32 p = _partition(arr, cmp, _low, _high);

    mu_qsort(arr, cmp, len, _low, p - 1);
    mu_qsort(arr, cmp, len, p + 1, _high);
}

i32 _glyphCmp(Glyph a, Glyph b) {
    const i32 Aa = (a.xMax - a.xMin) * (a.yMax - a.yMin),
              Ab = (b.xMax - b.xMin) * (b.yMax - b.yMin);

    return Aa - Ab;
};

//
FontInst ttfRender::GenerateUnicodeMSDFSubset(std::string src, UnicodeRange range, sdf_dim first_char_size) {
    //param checks and shit
    FontInst font;

    const f32 padding_per_32 = 4.0f;

    // get all the glyphs
    GlyphSet glyphs = ttfParse::GenerateGlyphSet(src, range);

    std::cout << "Generating " << glyphs.nGlyphs << " glyphs..." << std::endl;

    if (glyphs.nGlyphs == 0)
        return font;

    font.range = range;

    //compute scale
    f32 scale = 1.0f, p_const = 32.0f;

    switch (first_char_size.slc) {
    case sdf_dim_ty::Width:
        scale = (f32) first_char_size.m.w / (f32) (glyphs.glyphs[0].xMax - glyphs.glyphs[0].xMin);
        p_const = first_char_size.m.w;
        break;
    case sdf_dim_ty::Height:
        scale = (f32) first_char_size.m.h / (f32) (glyphs.glyphs[0].yMax - glyphs.glyphs[0].yMin);
        p_const = first_char_size.m.h;
        break;
    case sdf_dim_ty::Scale:
        scale = first_char_size.m.scale;

        //take avg between char w and char h
        p_const = 
            ((glyphs.glyphs[0].xMax - glyphs.glyphs[0].xMin) + 
            (glyphs.glyphs[0].yMax - glyphs.glyphs[0].yMin)) * 0.5f * scale;
        break;
    }

    sdf_dim c_dim = sdf_scale_dim(scale);

    const u32 padding = ceil((p_const / 32.0f) * padding_per_32);

    //sort by size (ascending)
    Glyph *gly = new Glyph[glyphs.nGlyphs];
    in_memcpy(gly, glyphs.glyphs, sizeof(Glyph) * glyphs.nGlyphs);

    mu_qsort<Glyph>(gly, &_glyphCmp, glyphs.nGlyphs);

    //compute the positions of all the characters
    struct SpriteRegion {
        i32 x = 0, y = 0, w = -1, h = -1, age = 0; //-1 for w and height is treated as infinity
    };

    u32 sheet_w = 1, sheet_h = 1;

    std::vector<SpriteRegion> rgn_stack = {{}}; //add the first "infinite" region

    i32 i = 0, j = -1;
    SpriteRegion Rn;

    font.c_pos = new CharSpritePos[glyphs.nGlyphs];

    while (i < glyphs.nGlyphs) {
        Glyph g = gly[i]; //get the current glyph

        //detect if it is a missing character
        if (g.char_id < 0) {
            font.c_pos[i] = {
                .x = 0,
                .y = 0,
                .w = 0,
                .h = 0
            };
            i++;
            continue;
        }

        i32 best_rgn = 0;
        u32 smallest_fit = (unsigned) (-1), fit, low_age = (unsigned) (-1);

        const size_t nRegions = rgn_stack.size();

        i32 gw = (g.xMax - g.xMin) * scale + padding * 2, gh = (g.yMax - g.yMin) * scale + padding * 2;

        //std::cout << "testing " << nRegions << " regions" << std::endl;

        for (j = 0; j < nRegions; j++) {
            Rn = rgn_stack[j];

            //make sure it fits
            if ((Rn.w < gw && Rn.w > 0) || (Rn.h < gh && Rn.h > 0)) continue;

            //branchless :D
            //still works with -1 denoting infinite region
            //pick region based on least total area increase
            //const u32 fit = (Rn.w >= Rn.h) * (((unsigned) Rn.w) - gw) + (Rn.w < Rn.h) * (((unsigned)Rn.h) - gh);
            const u32 fit = mu_max(Rn.x + gw, sheet_w) * mu_max(Rn.y + gh, sheet_h);

            /*if (Rn.age < low_age) {
                best_rgn = j;
                smallest_fit = fit;
                low_age = Rn.age;
            } */
            if (fit < smallest_fit) {
                best_rgn = j;
                smallest_fit = fit;
            } else if (fit == smallest_fit && Rn.age < low_age) {
                low_age = Rn.age;
                best_rgn = j;
                smallest_fit = fit;
            }
        }

        //add thing to sheet position or something
        SpriteRegion target_rgn = rgn_stack[best_rgn];

        if (target_rgn.x < 0 || target_rgn.y < 0 || gw < 0 || gh < 0) {
            std::cout << "error invalid region!" << std::endl;
            i++;
            continue;
        }

        //std::cout << "T Region: " << target_rgn.x << " " << target_rgn.y << std::endl;

        font.c_pos[i] = {
            .x = (u32) target_rgn.x,
            .y = (u32) target_rgn.y,
            .w = (u32) gw - padding * 2,
            .h = (u32) gh - padding * 2
        };

        //sizing adjust
        //TODO: ADJUST FOR PADDING
        sheet_w = mu_max(sheet_w, target_rgn.x + gw);
        sheet_h = mu_max(sheet_h, target_rgn.y + gh);

        //Split the target region up accordingly and then continue yk
        //just gonna split on the vertical cause why not

        rgn_stack.erase(rgn_stack.begin() + best_rgn); //remove old region

        //top rgn
        SpriteRegion r1 = {
            .x = target_rgn.x + gw,
            .y = target_rgn.y,
            .w = -1,
            .h = gh,
            .age = target_rgn.age + 1
        };

        rgn_stack.push_back(r1);

        //bottom rgn
        const i32 h2 = (target_rgn.h >= 0) * (target_rgn.h - gh) + (target_rgn.h < 0) * -1;

        if (abs(h2) >= 1) {
            SpriteRegion r2 = {
                .x = target_rgn.x,
                .y = target_rgn.y + gh,
                .w = -1,
                .h = h2,
                .age = target_rgn.age + 1
            };
            rgn_stack.push_back(r2); //add 2 new regions
        }
        //std::cout << "Computed Glyph: " << i << " | " << gw << "x" << gh << std::endl;

        i++;
    }

    //generate the spritesheet
    font.sheet.data = new byte[(sheet_w * sheet_h) * 3];
    ZeroMem(font.sheet.data, (sheet_w * sheet_h) * 3);
    font.sheet.header.w = sheet_w;
    font.sheet.header.h = sheet_h;
    font.sheet.header.bitsPerPixel = 24;
    font.sheet.header.fSz = (sheet_w * sheet_h) * 3;

    CharSpritePos r_pos;

    for (i = 0; i < glyphs.nGlyphs; i++) {
        r_pos = font.c_pos[i];

        if (r_pos.w <= 0 || r_pos.h <= 0)
            continue;

        render_positioned_msdf(
            gly[i], 
            &font.sheet, 
            r_pos.x, r_pos.y, r_pos.w, r_pos.h, 
            padding, padding, padding, padding
        );
        //std::cout << "Generated Glyph: " << i << std::endl;
    }

    //memory management
    _safe_free_a(gly);

    return font;
}

/*

It's weird how the faster my code gets the more lines it takes

*/