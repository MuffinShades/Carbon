#include "ttf_render.hpp"
#include "msutil.hpp"
#include "bitmap_render.hpp"
#include "logger.hpp"
#include "polynom.hpp"
#include "vec.hpp"
#include "gl/graphics.hpp"
#include "gl/geometry/rect.hpp"
#include "gl/Shader.hpp"
#include <vector>

#define MSFL_TTFRENDER_DEBUG
#define COMMA ,

#define MSDF_ACCEL_SHADER_PATH_VERT "../../src/msdf_gl_accel_vert.glsl"
#define MSDF_ACCEL_SHADER_PATH_FRAG "../../src/msdf_gl_accel.glsl"

constexpr f32 smol_number = 1.175e-38f; //number that is smol

/**
 *
 * All le code for rendering dem glyphs
 *
 * Written by muffinshades 2024-2026
 *
 */

static Shader msdf_gen_shader;

struct msdf_vert {
    f32 pos[3];
    f32 glyph_rel_region[2];
    i32 curve_range[2];
}; 

MsdfGpuContext *CreateMsdfGPUAccelerationContext(u32 w, u32 h) {
    MsdfGpuContext *ctx = new MsdfGpuContext;

    //load graphics
    ctx->g.Load();

    ctx->g.vertexStructureDefineBegin(sizeof(msdf_vert));
    ctx->g.defineVertexPart(0, vertexClassPart(msdf_vert, pos));
    ctx->g.defineVertexPart(1, vertexClassPart(msdf_vert, glyph_rel_region));
    ctx->g.defineIntegerVertexPart(2, vertexClassPart(msdf_vert, curve_range));
    ctx->g.vertexStructureDefineEnd();

    //set output device
    ctx->fb = FrameBuffer(FrameBuffer::Texture, w, h);

    std::cout << "Creating context: " << w << " " << h << std::endl;

    ctx->g.setOutputDevice(ctx->fb.device());

    ctx->good = true;

    return ctx;
}

MsdfGpuContext *CreateMsdfGPUAccelerationContext_Dynamic(u32 w, u32 h) {
    MsdfGpuContext *ctx = new MsdfGpuContext;

    //load graphics
    ctx->g.Load();

    ctx->g.vertexStructureDefineBegin(sizeof(msdf_vert));
    ctx->g.defineVertexPart(0, vertexClassPart(msdf_vert, pos));
    ctx->g.vertexStructureDefineEnd();

    ctx->fb.w = w;
    ctx->fb.h = h;

    ctx->good = true;

    return ctx;
}

void DeleteMsdfGPUContext(MsdfGpuContext * ctx) {
    if (!ctx)
        return;

    delete[] ctx;
    ctx = nullptr;
}


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

f32 pointDot(Point a, Point b) {
    return a.x * b.x + a.y * b.y;
}

Point pointAdd(Point a, Point b) {
    return Point(a.x + b.x, a.y + b.y);
}

Point pointSub(Point a, Point b) {
    return Point(a.x - b.x, a.y - b.y);
}

Point pointScale(Point a, f32 s) {
    return Point(a.x * s, a.y * s);
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
    CurveBounds bounds = {
        .r = -1
    };
    uvec3 color;
};

struct MsdfCurve {
    Point p[3];
    struct {
        f32 a_base = 0.0f, b_base = 0.0f, c_base = 0.0f, d_base = 0.0f;
        bool good = false;
    } solve_inf;
    CurveBounds bounds = {
        .r = -1
    };
    uvec3 color = uvec3(0,0,0);
};

struct gpu_light_curve {
    float p0[2];
    volatile float bro_why_the_hell_do_i_need_this_stupid_padding_thing[2];
    float p1[2];
    volatile float oh_look_another_waste_of_not_so_precious_memory[2];
    float p2[2];
    volatile float i_mine_aswell_be_a_call_of_duty_dev_atp_with_the_amount_of_bytes_im_wasting[2];
    float chroma[3];
    volatile float small_ass_padding_float_that_only_exists_to_make_my_code_even_worse;
    float compute_base[4]; 
};

struct Edge {
    BCurve *curves = nullptr;
    size_t nCurves = 0;
    size_t final_point_index;
    size_t inital_point_index;
    uvec3 color = uvec3(0,0,0);
    CurveBounds bounds;
};

struct MsdfGenContext {
    void *curves;
    size_t nCurves;
    size_t curveSize;
    enum {
        NormalCurve,
        LightCurve
    } curveType = MsdfGenContext::NormalCurve;
};

void DeleteMsdfGenContext(MsdfGenContext *ctx) {
    _safe_free_a(ctx->curves);
    ctx->nCurves = 0;
    ctx->curveSize = 0;
}

struct PDistInfo {
    f32 dx = INFINITY,dy = INFINITY,d = INFINITY,t = 0.0f, true_t = 0.0f;
    BCurve curve;
    Point p;
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
PDistInfo EdgePointSignedDist(Point p, Edge& e, i32 maxCurvesTest = -1) {
    if (!e.curves || e.nCurves == 0) return {.d=INFINITY};

    PDistInfo d = {
        .d = INFINITY
    };

    Point refPoint;

    i32 c,i;

    f32 roots[3] = {0,0,0};
    f32 *root_pass = roots + 2;

    Point p0,p1,p2;
    f32 a = -1.0f,b = -1.0f,f = -1.0f,g = -1.0f;
    f32 alpha,beta,gamma,t2,t2_i;
    f64 dx,dy,_D;

    f64 dx_best,dy_best,t_best,t_out,d_best = INFINITY;
    i32 best_c = -1;

    size_t _NC = e.nCurves;

    if (maxCurvesTest > 0) _NC = mu_min(_NC, maxCurvesTest);

    for (c = 0; c < _NC; c++) {
        BCurve* tCurve = e.curves + c;

        p0 = tCurve->p[0]; p1 = tCurve->p[1]; p2 = tCurve->p[2];

        if (!tCurve->solve_inf.good) {
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

        const Point ip = pointSub(p0, p), fp = pointSub(p2, p), d0 = dBdt3(p0, p1, p2, 0.0f), d1 = dBdt3(p0, p1, p2, 1.0f), 
              ese0 = pointSub(d0, p0), ese1 = pointSub(p2, d1);

        //t = 0
        d_best = pointDot(ip, ip);
        t_best = 0.0f;
        t_out = -pointDot(ip, ese0) / pointDot(ese0, ese0);
        dx_best = ip.x; dy_best = ip.y;

        //t = 1
        f32 eDist = pointDot(fp, fp);

        if (eDist < d_best) {
            d_best = eDist;
            t_best = 1.0;
            t_out = pointDot(fp, ese1) / pointDot(ese1, ese1);
            dx_best = fp.x;
            dy_best = fp.y;
        }

        for (i = 0; i < nRoots; i++) {
            t2 = roots[i];

            if (t2 < 0.0f || t2 > 1.0f) continue;

            t2_i = 1.0f - t2;
            alpha = t2_i * t2_i;
            beta = 2.0f * t2_i * t2;
            gamma = t2 * t2;

            dx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p.x;
            dy = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p.y;
            _D = dx*dx + dy*dy;
            
            if (_D < d_best) {
                dx_best = dx;
                dy_best = dy;
                best_c = c;
                t_best = (t_out = t2);
                d_best = _D;
            }
        }
    }

    if (_NC == 0 || best_c == -1) {
        return {
            .d = INFINITY
        };
    }

    const BCurve bestCurve = e.curves[best_c];

    d.dx = dx_best;
    d.dy = dy_best;
    d.t = t_out;
    d.p = p;
    d.curve = bestCurve;

    //compute the signed distance
    d.d = mu_sign(pointCross(
        dBdt3(bestCurve.p[0],bestCurve.p[1],bestCurve.p[2], t_best),
        {(f32) dx_best, (f32) dy_best}
    )) * sqrtf(d_best);

    return d;
}

PDistInfo FancyEdgePointSignedDist(Point p, Edge& e, f32 r) {
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

    f32 test_r1, test_r2, test_diff = INFINITY;

    const size_t _NC = e.nCurves;

    f32 dbx,dby;

    for (c = 0; c < _NC; c++) {
        BCurve* tCurve = e.curves + c;

        dbx = p.x - tCurve->bounds.center.x;
        dby = p.y - tCurve->bounds.center.y;

        if (r > 0 &&
            (test_r1 = ((dbx * dbx + dby * dby))) 
            > (test_r2=((r + tCurve->bounds.r) * (r + tCurve->bounds.r)))
        ) {
            if (test_r1 - test_r2 < test_diff)
                test_diff = test_r1 - test_r2;

            continue;
        }

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

        for (i = 2; i < nRoots+2; i++) {
            t2 = (roots[i] > 1.0f) + (roots[i] >= 0.0f && roots[i] <= 1.0f) * roots[i];

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
            }
        }
    }

    if (_NC == 0 || best_c == -1) {
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

struct PDistInf_Lite {
    f32 d = -INFINITY;
    f32 t = 0.0f;
};

PDistInf_Lite CurvePointSignedDist(Point p, MsdfCurve* tCurve) {
    if (!tCurve) return {};

    PDistInf_Lite d;

    Point refPoint;

    i32 c,i;

    f32 roots[3] = {0,0,0};
    f32 *root_pass = roots + 2;

    Point p0,p1,p2;
    f32 a = -1.0f,b = -1.0f,f = -1.0f,g = -1.0f;
    f32 alpha,beta,gamma,t2,t2_i;
    f64 dx,dy,_D;

    f64 dx_best,dy_best,t_best,t_out,d_best = INFINITY;
    i32 best_c = -1;

    p0 = tCurve->p[0]; p1 = tCurve->p[1]; p2 = tCurve->p[2];

    if (!tCurve->solve_inf.good) {
        tCurve->solve_inf.a_base = compute_a_base_coord(p0.x, p1.x, p2.x) + compute_a_base_coord(p0.y, p1.y, p2.y);
        tCurve->solve_inf.b_base = compute_b_base_coord(p0.x, p1.x, p2.x) + compute_b_base_coord(p0.y, p1.y, p2.y);
        tCurve->solve_inf.c_base = compute_c_base_coord(p0.x, p1.x, p2.x) + compute_c_base_coord(p0.y, p1.y, p2.y);
        tCurve->solve_inf.d_base = compute_d_base_coord(p0.x, p1.x, p2.x) + compute_d_base_coord(p0.y, p1.y, p2.y);
        tCurve->solve_inf.good = true;
    }

     //when solving the min dist / roots --> optimize to use solve_re_cubic_32_b or solve_re_cubic_64_b

    const i32 nRoots = solve_re_cubic_32_a(
        (tCurve->solve_inf.a_base), 
        (tCurve->solve_inf.b_base),
        (tCurve->solve_inf.c_base 
            - 4.0f * (p0.y*p.y + p0.x*p.x)
            + 8.0f * (p1.y*p.y + p1.x*p.x)
            - 4.0f * (p2.y*p.y + p2.x*p.x)),
        (tCurve->solve_inf.d_base - 4.0f * (p1.y*p.y + p1.x*p.x) + 4.0f * (p0.y*p.y + p0.x*p.x)),
        root_pass
    );
    
    const Point ip = pointSub(p0, p), fp = pointSub(p2, p), d0 = dBdt3(p0, p1, p2, 0.0f), d1 = dBdt3(p0, p1, p2, 1.0f), 
        ese0 = pointSub(d0, p0), ese1 = pointSub(p2, d1);

    //t = 0
    d_best = pointDot(ip, ip);
    t_best = 0.0f;
    t_out = -pointDot(ip, ese0) / pointDot(ese0, ese0);
    dx_best = ip.x; dy_best = ip.y;

    //t = 1
    f32 eDist = pointDot(fp, fp);

    if (eDist < d_best) {
        d_best = eDist;
        t_best = 1.0;
        t_out = pointDot(fp, ese1) / pointDot(ese1, ese1);
        dx_best = fp.x;
        dy_best = fp.y;
    }

    for (i = 0; i < nRoots; i++) {
        t2 = roots[i];

        if (t2 < 0.0f || t2 > 1.0f) continue;

        t2_i = 1.0f - t2;
        alpha = t2_i * t2_i;
        beta = 2.0f * t2_i * t2;
        gamma = t2 * t2;

        dx = (alpha * p0.x + beta * p1.x + gamma * p2.x) - p.x;
        dy = (alpha * p0.y + beta * p1.y + gamma * p2.y) - p.y;
        _D = dx*dx + dy*dy;
            
        if (_D < d_best) {
            dx_best = dx;
            dy_best = dy;
            best_c = c;
            t_best = (t_out = t2);
            d_best = _D;
        }
    }

    d.t = t_out;

    //compute the signed distance
    d.d = mu_sign(pointCross(
        dBdt3(p0,p1,p2, t_best),
        {(f32) dx_best, (f32) dy_best}
    )) * sqrtf(d_best);

    return d;
}

CurveBounds computeCurveBoundingRadius(BCurve* curve, bool add_to_curve = true) {
    Point p0 = curve->p[0], p1 = curve->p[1], p2 = curve->p[2];

    const Point centroid = {
        .x = (p0.x + p2.x) * 0.5f,
        .y = (p0.y + p2.y) * 0.5f
    };

    //radius point 1
    const f32 r1 = sqrtf((centroid.x - p0.x) * (centroid.x - p0.x) + (centroid.y - p0.y) * (centroid.y - p0.y));

    //triangle point 3
    f32 roots[5] = {0,1,0,0,0};
    f32 *root_pass = roots+2;

    if (!curve->solve_inf.good) {
        curve->solve_inf.a_base = compute_a_base_coord(p0.x, p1.x, p2.x) + compute_a_base_coord(p0.y, p1.y, p2.y);
        curve->solve_inf.b_base = compute_b_base_coord(p0.x, p1.x, p2.x) + compute_b_base_coord(p0.y, p1.y, p2.y);
        curve->solve_inf.c_base = compute_c_base_coord(p0.x, p1.x, p2.x) + compute_c_base_coord(p0.y, p1.y, p2.y);
        curve->solve_inf.d_base = compute_d_base_coord(p0.x, p1.x, p2.x) + compute_d_base_coord(p0.y, p1.y, p2.y);
        curve->solve_inf.good = true;
    }
    
    const i32 nRoots = solve_re_cubic_32_a(
        curve->solve_inf.a_base, 
        curve->solve_inf.b_base,
        curve->solve_inf.c_base 
                - 4.0f * (p0.y*p1.y + p0.x*p1.x)
                + 8.0f * (p1.y*p1.y + p1.x*p1.x)
                - 4.0f * (p2.y*p1.y + p2.x*p1.x),
        curve->solve_inf.d_base - 4.0f * (p1.y*p1.y + p1.x*p1.x) + 4.0f * (p0.y*p1.y + p0.x*p1.x),
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

    //std::cout << "Radius: " << r2 << " " << r1 << std::endl;
    vec2 va = vec2(p3.x - p0.x, p3.y - p0.y),
         vb = vec2(p2.x - p0.x, p2.y - p0.y),
         vc = vec2(p2.x - p3.x, p2.y - p3.y);


    const f32 a = sqrtf(va.x * va.x + va.y * va.y),
              b = sqrtf(vb.x * vb.x + vb.y * vb.y),
              c = sqrtf(vc.x * vc.x + vc.y * vc.y),
              ia = 1.0f / a,
              ib = 1.0f / b,
              ic = 1.0f / c;

    //TODO: optimize using cross product instead of dot product --> cos(theta) --> cosf(cos(theta)) --> sin(2cosf(cos(theta)))
    //instead: sin(2a) = 2sin(a)cos(a) --> find sin and cos from cross and dot product
    const f32 theta = acosf(vec2::DotProd(va, vb) * ia * ib),
              iota = acosf(vec2::DotProd(va * -1.0f, vc * -1.0f) * ia * ic),
              kappa = acosf(vec2::DotProd(vb * -1.0f, vc) * ib * ic);

    const f32 sin_2a = sinf(2.0f * theta),
              sin_2b = sinf(2.0f * iota),
              sin_2c = sinf(2.0f * kappa),
              I = 1.0f / (sin_2a + sin_2b + sin_2c);

    //std::cout << "SOM: " << theta + iota + kappa << std::endl;

    Point ccenter = {
        .x = (p0.x * sin_2a + p3.x * sin_2b + p2.x * sin_2c) * I,
        .y = (p0.y * sin_2a + p3.y * sin_2b + p2.y * sin_2c) * I
    };

    const f32 s = 0.5f * (a + b + c), A = sqrtf(s * (s - a) * (s - b) * (s - c));
    const f32 r3 = (a * b * c) / (4.0f * A);

    const f32 ra = mu_min(r1, r2);

    CurveBounds cb;

    //use the smallest radius bounding circumference
    if (r3 < ra) {
        cb.center = ccenter;
        cb.r = r3;
    } else {
        cb.center = centroid;
        cb.r = ra;
    }

    if (add_to_curve)
        curve->bounds = cb;
    
    return cb;
}

CurveBounds computeEdgeBounds(Edge& e, bool add_to_edge = true) {
    f32 csumX = 0.0f, csumY = 0.0f;

    const size_t nc = e.nCurves;
    BCurve *cu;

    if (!e.curves || nc == 0)
        return {};

    i32 c;

    for (c = 0; c < nc; c++) {
        cu = e.curves + c;

        //std::cout << "idek " << cu->bounds.r << std::endl;

        if (cu->bounds.r <= 0)
            computeCurveBoundingRadius(cu);

        csumX += cu->bounds.center.x;
        csumY += cu->bounds.center.y;
    }

    const f32 is = 1.0f / (f32) nc;

    Point center = {.x = csumX * is, .y = csumY * is};

    //std::cout << csumX << " " << csumY << std::endl;

    CurveBounds b = {
        .center = center
    };

    //compute le radius
    f32 r = -1.0f;

    for (c = 0; c < nc; c++) {
        cu = e.curves + c;

        r = mu_max(
            cu->bounds.r + sqrtf(
                (cu->bounds.center.x - center.x)*(cu->bounds.center.x - center.x) +
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
f32 curveOrtho(MsdfCurve c, Point p, f32 t) {
    const Point b = bezier3(c.p[0], c.p[1], c.p[2], t);
    return abs(pointCross(pointNormalize(
        dBdt3(c.p[0], c.p[1], c.p[2], t)
    ), pointNormalize(
        {p.x-b.x,p.y-b.y}
    )));
}

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

void ConvertToPseudoDist(PDistInf_Lite& d, Point p, MsdfCurve c) {
    if (d.t < 0.0f) {
        Point dr = dBdt3(c.p[0], c.p[1], c.p[2], 0.0f);
        Point p1 = pointAdd(c.p[0], dr);
        Point p0 = c.p[0];
        Point e = pointSub(p1, p0);
        f32 em = pointDot(e,e), t = -pointDot(pointSub(p, p0), e) / em;
        
        if (t < 0.0f) {
            f32 pd = fabs(pointCross(p, e) - pointCross(p0, p1)) / sqrtf(em);

            if (pd < fabs(d.d)) {
                d.d = mu_sign(pointCross(e, pointSub((pointAdd(p0, pointScale(dr, t))),p))) * pd;
                d.t = t;
            }
        }
    } else if (d.t > 1.0f) {
        Point dr = dBdt3(c.p[0], c.p[1], c.p[2], 0.0f);
        Point p1 = pointAdd(c.p[2], dr);
        Point p0 = c.p[2];
        Point e = pointSub(p1, p0);
        f32 em = pointDot(e,e), t = -pointDot(pointSub(p, p0), e) / em;
        
        if (t < 0.0f) {
            f32 pd = fabs(pointCross(p, e) - pointCross(p0, p1)) / sqrtf(em);

            if (pd < fabs(d.d)) {
                d.d = mu_sign(pointCross(e, pointSub((pointAdd(p0, pointScale(dr, t))),p))) * pd;
                d.t = t;
            }
        }
    }
}

struct glfEdgeObject {
    BCurve* curveBuff = nullptr;
    std::vector<Edge> edges;
};

glfEdgeObject generateGlyphEdges(Glyph glyph_data, gPData& points, size_t nPoints) {
    glfEdgeObject eObj;
    
    const size_t nCurves = nPoints >> 1;
    eObj.curveBuff = new BCurve[nCurves];
    ZeroMem(eObj.curveBuff, nCurves);
    BCurve* curveBuffer = eObj.curveBuff; //stores curves of current edge

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
                return eObj;
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

                eObj.edges.push_back(e);

                //reset for next edge
                curveBuffer += nEdgeCurves; //go to next segment in the curve buffer
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

    //return :3
    return eObj;
}

i32 ttfRender::RenderGlyphSDFToBitMap(Glyph tGlyph, Bitmap* map, sdf_dim size) {
    if (!map)
        return 1;

    //do some dimension calculations
    u32 sdfW = 0, sdfH = 0, sdfTrueW = 0, sdfTrueH = 0;

    const u32 padding = 0.0f;

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
    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

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

            EdgeDistInfo fieldDist = MinEdgeDist(p, glyphEdges.edges);

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
    for (Edge e : glyphEdges.edges) {
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

MsdfCurve BtoMsdfCurve(BCurve c, uvec3 color) {
    MsdfCurve mc;

    mc.color.x = color.x;
    mc.color.y = color.y;
    mc.color.z = color.z;

    mc.p[0] = c.p[0];
    mc.p[1] = c.p[1];
    mc.p[2] = c.p[2];

    mc.bounds = c.bounds;
    mc.solve_inf.a_base = c.solve_inf.a_base;
    mc.solve_inf.b_base = c.solve_inf.b_base;
    mc.solve_inf.c_base = c.solve_inf.c_base;
    mc.solve_inf.d_base = c.solve_inf.d_base;
    mc.solve_inf.good = c.solve_inf.good;

    return mc;
}

gpu_light_curve BtoLightCurve(BCurve c, uvec3 color) {
    gpu_light_curve gc;

    gc.chroma[0] = color.x;
    gc.chroma[1] = color.y;
    gc.chroma[2] = color.z;

    gc.p0[0] = c.p[0].x;
    gc.p0[1] = c.p[0].y;
    gc.p1[0] = c.p[1].x;
    gc.p1[1] = c.p[1].y;
    gc.p2[0] = c.p[2].x;
    gc.p2[1] = c.p[2].y;

    gc.compute_base[0] = c.solve_inf.a_base;
    gc.compute_base[1] = c.solve_inf.b_base;
    gc.compute_base[2] = c.solve_inf.c_base;
    gc.compute_base[3] = c.solve_inf.d_base;

    return gc;
}

MsdfGenContext CreateMsdfGenContext(Glyph tGlyph, bool accel = false) {
    MsdfGenContext ctx = {
        .curves = nullptr,
        .nCurves = 0
    };

    //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    //blank glyph so blank sdf
    if (nPoints < 3)
        return ctx;

    //curve and edge generation, glyph clean up, and more
    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

    i32 c;

    const size_t nGlyphEdges = glyphEdges.edges.size();

    Contour *glyph_contours = new Contour[tGlyph.nContours];
    ZeroMem(glyph_contours, tGlyph.nContours);
    u32 cur_min_idx = 0, e_idx = 0;

    size_t nCurves = 0;

    for (c = 0; c < tGlyph.nContours; c++) { //oh my fucking god C++???!?!?!?! No way!!! :O
        Contour *cur_c = glyph_contours + c;
        *cur_c = Contour();

        //compute min and max points
        cur_c->minPoint = cur_min_idx;
        cur_c->maxPoint = tGlyph.modifiedContourEnds[c] + cur_c->minPoint;

        cur_min_idx += tGlyph.modifiedContourEnds[c];

        //assign edges
        e_idx = 0;
        for (Edge e : glyphEdges.edges) {
            if (
                (e.inital_point_index >= cur_c->minPoint && e.inital_point_index < cur_c->maxPoint) ||
                (e.final_point_index > cur_c->minPoint && e.final_point_index <= cur_c->maxPoint)
            ) {
                cur_c->edge_idxs.push_back(e_idx);
            }

            nCurves += e.nCurves;
            e_idx++;
        }
    }

    //now color le edges
    i32 i, j;

    uvec3 cur_color;

    if (accel) {
        ctx.curves = new gpu_light_curve[nCurves];
        ZeroMem((gpu_light_curve*) ctx.curves, nCurves);
        ctx.curveSize = sizeof(gpu_light_curve);
        ctx.curveType = MsdfGenContext::LightCurve;
    } else {
        ctx.curves = new MsdfCurve[nCurves];
        ZeroMem((MsdfCurve*) ctx.curves, nCurves);
        ctx.curveSize = sizeof(MsdfCurve);
        ctx.curveType = MsdfGenContext::NormalCurve;
    }

    gpu_light_curve *lc_buff = (gpu_light_curve*) ctx.curves;
    MsdfCurve *nc_buff = (MsdfCurve*) ctx.curves;
    
    ctx.nCurves = nCurves;
    
    Edge E;

    size_t cpi = 0;

    BCurve qu;

    for (c = 0; c < tGlyph.nContours; c++) {
        Contour ct = glyph_contours[c];

        const size_t ncEdges = ct.edge_idxs.size();
        u32 t_edge;

        if (ncEdges == 0)
            continue;
        else if (ncEdges == 1) {
            t_edge = ct.edge_idxs[0];
            E = glyphEdges.edges[t_edge];
            E.color = uvec3(1,1,1);

            //add new curves
            for (j = 0; j < E.nCurves; j++) {
                qu = E.curves[j];

                if (!qu.solve_inf.good) {
                    qu.solve_inf.a_base = compute_a_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_a_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                    qu.solve_inf.b_base = compute_b_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_b_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                    qu.solve_inf.c_base = compute_c_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_c_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                    qu.solve_inf.d_base = compute_d_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_d_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                }

                if (accel)
                    lc_buff[cpi++] = BtoLightCurve(qu, cur_color);
                else
                    nc_buff[cpi++] = BtoMsdfCurve(qu, cur_color);
            }

            continue;
        }

        cur_color = uvec3(1,0,1);

        for (i = 0; i < ncEdges; i++) {
            t_edge = ct.edge_idxs[i];
            E = glyphEdges.edges[t_edge];

            if (!E.curves)
                continue;

            //add new curves
            for (j = 0; j < E.nCurves; j++) {
                qu = E.curves[j];

                if (!qu.solve_inf.good) {
                    qu.solve_inf.a_base = compute_a_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_a_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                    qu.solve_inf.b_base = compute_b_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_b_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                    qu.solve_inf.c_base = compute_c_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_c_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                    qu.solve_inf.d_base = compute_d_base_coord(qu.p[0].x,qu.p[1].x,qu.p[2].x) + compute_d_base_coord(qu.p[0].y,qu.p[1].y,qu.p[2].y);
                }

                if (accel)
                    lc_buff[cpi++] = BtoLightCurve(qu, cur_color);
                else
                    nc_buff[cpi++] = BtoMsdfCurve(qu, cur_color);
            }

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
        }
    }

    _safe_free_a(glyph_contours);
    _safe_free_a(glyphEdges.curveBuff);

    return ctx;
}

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
    MsdfGenContext g_ctx = CreateMsdfGenContext(tGlyph);

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

    PDistInf_Lite d, dr, dg, db;

    MsdfCurve *cr, *cg, *cb;

    Point p;

    //curve check index buffer
    MsdfCurve *ccurve = nullptr;

    f32 testRadius = -1;

    vec3 dv;

    f32 d_cmp;

    i32 i;

    MsdfCurve *cu_buff = (MsdfCurve*)g_ctx.curves;

    //generate the msdf
    //must snake scan (reason it's called the snake algorithm) since when y ++ things go south
    i32 xScanMin = 0, xScanMax = regionW, scanDx = 1;
    for (y = 0; y < regionH; ++y) {
        for (x = xScanMin; abs(xScanMax - x) > 0; x += scanDx) {
            p.x = floor(((f32)x - paddingLeft) + 0.5f) * wc + (tGlyph.xMin);
            p.y = floor(((f32)y - paddingTop) + 0.5f) * hc + (tGlyph.yMin);
            
            dr.d = dg.d = db.d = INFINITY;

            //Edge e = glyphEdges[0];

            for (i = 0; i < g_ctx.nCurves; i++) {
                ccurve = cu_buff + i;

                d = CurvePointSignedDist(p, ccurve);

                //d = FancyCurvePointSignedDist(p, g_ctx.curves[i], testRadius);
                //d = EdgePointSignedDist(p,e);

                if (d.d == INFINITY)
                    continue;

                if (ccurve->color.x) {
                    if (abs(abs(d.d) - abs(dr.d)) <= 0.01f) {
                        //check orthoginality
                        f32 o1 = curveOrtho(*ccurve, p, d.t),
                            o2 = curveOrtho(*cr, p, dr.t);

                        if (o2 < o1) goto set_r;
                    } else if (abs(d.d) < abs(dr.d)) {
                     set_r:  
                        dr = d;
                        cr = ccurve;
                    }
                }

                if (ccurve->color.y) {
                    if (abs(abs(d.d) - abs(dg.d)) <= 0.01f) {
                        //check orthoginality
                        f32 o1 = curveOrtho(*ccurve, p, d.t),
                            o2 = curveOrtho(*cg, p, dg.t);

                        if (o2 < o1) goto set_g;
                    } else if (abs(d.d) < abs(dg.d)) {
                    set_g:  
                        dg = d;
                        cg = ccurve;
                    }
                }

                if (ccurve->color.z) {
                    if (abs(abs(d.d) - abs(db.d)) <= 0.01f) {
                        //check orthoginality
                        f32 o1 = curveOrtho(*ccurve, p, d.t),
                            o2 = curveOrtho(*cb, p, db.t);

                        if (o2 < o1) goto set_b;
                    } else if (abs(d.d) < abs(db.d)) {
                    set_b:  
                        db = d;
                        cb = ccurve;
                    }
                }
            }
            
            d_cmp = mu_max(mu_max(abs(dr.d), abs(dg.d)), abs(db.d)); 
            testRadius = d_cmp + wc * 1.5f;

            ConvertToPseudoDist(dr, p, *cr);
            ConvertToPseudoDist(dg, p, *cg);
            ConvertToPseudoDist(db, p, *cb);           

            dv = vec3(dr.d,dg.d,db.d);

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

        //DO NOT FLIP THE ORDERS OF THESE OR SHIT WILL BREAK
        scanDx = (2 * !!xScanMin) - 1;
        xScanMax = regionW * !!xScanMin - 1 * !!xScanMax;
        xScanMin = regionW * !xScanMin - 1 * !xScanMin;
    }

    return 0;
}

/*

Accelerated version of msdf gen


*/
i32 render_positioned_msdf_gpu_accel(Glyph& tGlyph, MsdfGpuContext *ctx, const i32 regionX, const i32 regionY, const u32 regionW, const u32 regionH, const u32 paddingLeft, const u32 paddingTop, const u32 paddingRight, const u32 paddingBottom) {
    
    //simple error / render ability checks
    if (regionW == 0 || regionH == 0) 
        return 0;

    if (!ctx) {
        std::cout << "error invalid context" << std::endl;
        return 1;
    }

    //
    const size_t nChannels = 4;

     //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    //blank glyph so blank sdf
    if (nPoints == 0)
        return 0;

    //curve and edge generation, glyph clean up, and more

    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

    //compute conture colors
    i32 c;

    u32 edge_i = 0;
    Edge cur_edge, next_edge;
    bool final_edge = false;
    size_t ncontour_edges = 0;

    const size_t nGlyphEdges = glyphEdges.edges.size();

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
        for (Edge e : glyphEdges.edges) {
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
    i32 i, j;

    uvec3 cur_color;

    std::vector<gpu_light_curve> gpu_curves;

    uvec3 col_temp;

    Point cp0,cp1,cp2;

    for (c = 0; c < tGlyph.nContours; c++) {
        Contour ct = glyph_contours[c];

        const size_t ncEdges = ct.edge_idxs.size();
        u32 t_edge;

        if (ncEdges == 0)
            continue;
        else if (ncEdges == 1) {
            t_edge = ct.edge_idxs[0];
            glyphEdges.edges[t_edge].color = uvec3(1,1,1);
            continue;
        }

        cur_color = uvec3(1,0,1);

        for (i = 0; i < ncEdges; i++) {
            t_edge = ct.edge_idxs[i];

            Edge& e = glyphEdges.edges[t_edge];
            e.color = cur_color;

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

            //create the light curves
            BCurve cu;

            //solve stuff
            for (j = 0; j < glyphEdges.edges[t_edge].nCurves; j++) {
                cu = e.curves[j];

                cp0 = cu.p[0]; cp1 = cu.p[1]; cp2 = cu.p[2];

                //compute the precomputation shit
                cu.solve_inf.a_base = compute_a_base_coord(cp0.x, cp1.x, cp2.x) + compute_a_base_coord(cp0.y, cp1.y, cp2.y);
                cu.solve_inf.b_base = compute_b_base_coord(cp0.x, cp1.x, cp2.x) + compute_b_base_coord(cp0.y, cp1.y, cp2.y);
                cu.solve_inf.c_base = compute_c_base_coord(cp0.x, cp1.x, cp2.x) + compute_c_base_coord(cp0.y, cp1.y, cp2.y);
                cu.solve_inf.d_base = compute_d_base_coord(cp0.x, cp1.x, cp2.x) + compute_d_base_coord(cp0.y, cp1.y, cp2.y);

                //construct the curve
                gpu_light_curve lc = {
                    .p0 = {cu.p[0].x, cu.p[0].y},
                    .p1 = {cu.p[1].x, cu.p[1].y},
                    .p2 = {cu.p[2].x, cu.p[2].y},
                    .chroma = {(f32) e.color.x, (f32) e.color.y, (f32) e.color.z},
                    .compute_base = {cu.solve_inf.a_base,cu.solve_inf.b_base,cu.solve_inf.c_base,cu.solve_inf.d_base}
                };

                gpu_curves.push_back(lc); //add the curve
            }

            edge_i++;
        }
    }

    //compute some dimensions
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

    //graphics setup

    if (!msdf_gen_shader.good())
        msdf_gen_shader = Shader::LoadShaderFromFile(MSDF_ACCEL_SHADER_PATH_VERT, MSDF_ACCEL_SHADER_PATH_FRAG);

    if (!ctx->good)
        std::cout << "warning bad context!" << std::endl;

    ctx->g.setCurrentShader(&msdf_gen_shader);
    
    //params of the curves being sent to the gpu
    if (!ctx->curveBuffer)
        glGenBuffers(1, &ctx->curveBuffer);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ctx->curveBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(gpu_light_curve) * gpu_curves.size(), gpu_curves.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ctx->curveBuffer);


    const f32 space_normal_x = 1.0f / ctx->fb.w,
              space_normal_y = 1.0f / ctx->fb.h;

    //set le uniforms
    vec4 padding_vec = vec4(paddingLeft / regionW, paddingTop / regionH, paddingRight / regionW, paddingBottom / regionH);
    vec4 region_vec = vec4(regionX * space_normal_x, regionY * space_normal_y, regionW * space_normal_x, regionH * space_normal_y);
    vec4 glyph_dim_vec = vec4(tGlyph.xMin, tGlyph.yMin, tGlyph.xMax, tGlyph.yMax);

    msdf_gen_shader.SetInt("nCurves", gpu_curves.size());
    msdf_gen_shader.SetVec4("padding", &padding_vec);
    msdf_gen_shader.SetVec4("region", &region_vec);
    msdf_gen_shader.SetVec4("glyphDim", &glyph_dim_vec);

    //render
    msdf_vert out_rect[] = RECT_VERTS(region_vec.x, region_vec.y, region_vec.z, region_vec.w, 0.0 COMMA glyph_dim_vec.x COMMA glyph_dim_vec.y COMMA glyph_dim_vec.x COMMA glyph_dim_vec.w);

    ctx->g.render_begin();
    ctx->g.push_verts(out_rect, sizeof(out_rect) / sizeof(msdf_vert));
    ctx->g.render_flush();

    _safe_free_a(glyph_contours); //more memory management

    //oh look were managing memory :O
    _safe_free_a(glyphEdges.curveBuff);

    return 0;
}

//////////////////////////////////////////////
// version of gpu accel for multiple glyphs //
//////////////////////////////////////////////

const size_t MAX_BUFFER_CURVES = 1024;

i32 render_multi_positioned_msdf_gpu_accel(Glyph* tGlyphs, CharSpritePos* pos, size_t nGlyphs, MsdfGpuContext *ctx, f32 padding_scl) {
    
    //simple error / render ability checks
    if (ctx->fb.w == 0 || ctx->fb.h == 0) 
        return 0;

    if (!ctx) {
        std::cout << "error invalid context" << std::endl;
        return 1;
    }

    //
    const size_t nChannels = 4;

    Glyph tg;
    CharSpritePos g_pos;
    f32 gw,gh;

    i32 i;

    size_t dat_collect;

    if (!msdf_gen_shader.good())
        msdf_gen_shader = Shader::LoadShaderFromFile("../../src/msdf_gl_accel_vert.glsl", "../../src/msdf_gl_accel.glsl");

    ctx->g.setCurrentShader(&msdf_gen_shader);
    ctx->g.render_begin();

    constexpr size_t N_RECT_VERTS = 6;
    size_t nextCurveInsert = 0;

    MsdfGenContext g_ctx;

    for (i = 0; i < nGlyphs; i++) {
        tg = tGlyphs[i];
        g_pos = pos[i];

        if (g_pos.w == 0 || g_pos.h == 0) continue; //dimension check (dont add null glyphs)

        gw = tg.xMax - tg.xMin;
        gh = tg.yMax - tg.yMin;

        //generate the glpyh context
        g_ctx = CreateMsdfGenContext(tg, true);

        if (ctx->g.vert_space() < N_RECT_VERTS || nextCurveInsert + g_ctx.nCurves >= MAX_BUFFER_CURVES) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ctx->curveBuffer);

            ctx->g.render_flush();
            ctx->g.render_begin(false);

            nextCurveInsert = 0;
        }

        //compute some dimensions
        i32 x,y;

        //TODO: reimplement this check with the scaling instead of value
        /*u32 paddingX = padding * 2,
            paddingY = paddingX;

        while (g_pos.w <= paddingX && paddingX >= 2)
            paddingX -= 2;

        while (g_pos.h <= paddingY && paddingY >= 2)
            paddingY -= 2;

        if (g_pos.w <= paddingX || g_pos.h <= paddingY || paddingX < 0 || paddingY < 0)
            continue; //no room ;-;*/

        //graphics setup
        if (!ctx->good)
            std::cout << "warning bad context!" << std::endl;
    
        //curve buffer check
        if (!ctx->curveBuffer) {
            glGenBuffers(1, &ctx->curveBuffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ctx->curveBuffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(gpu_light_curve) * MAX_BUFFER_CURVES, 0, GL_DYNAMIC_DRAW);
        }

        //copy over the curves
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ctx->curveBuffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, nextCurveInsert * sizeof(gpu_light_curve), sizeof(gpu_light_curve) * g_ctx.nCurves, g_ctx.curves);
        
        //advance curve insert pointer
        i32 curveMin = nextCurveInsert, curveMax = nextCurveInsert + g_ctx.nCurves;

        nextCurveInsert += g_ctx.nCurves;


        const f32 space_normal_x = 1.0f / (ctx->fb.w),
              space_normal_y = 1.0f / (ctx->fb.h);

        //set le uniforms
        vec4 region_vec, scan_rgn;

        region_vec = vec4((g_pos.x * space_normal_x) * 2.0f - 1.0f, (g_pos.y * space_normal_y) * 2.0f - 1.0f, (g_pos.w * space_normal_x) * 2.0f, (g_pos.h * space_normal_y) * 2.0f);
        scan_rgn = vec4(tg.xMin - padding_scl * gw, tg.yMin - padding_scl * gh, gw + padding_scl * gw * 2.0f, gh + padding_scl * gh * 2.0f);

        //render
        if (g_pos.rotate) {
            f32 tmp = scan_rgn.w;
            scan_rgn.w = scan_rgn.z;
            scan_rgn.z = tmp;

            msdf_vert out_rect[] = { 
                (region_vec.x), (region_vec.y), 0.0 , 
                scan_rgn.x+scan_rgn.w, scan_rgn.y, curveMin, curveMax, //tl --> tr

                (region_vec.x), (region_vec.y+region_vec.z), 0.0 , 
                scan_rgn.x , (scan_rgn.y), curveMin, curveMax, //bl --> tl

                (region_vec.x+region_vec.w), (region_vec.y), 0.0 , 
                (scan_rgn.x+scan_rgn.z), (scan_rgn.y+scan_rgn.w),  curveMin, curveMax, //tr --> br
            
                (region_vec.x+region_vec.w), (region_vec.y+region_vec.z), 0.0 , 
                scan_rgn.x, scan_rgn.y+scan_rgn.w, curveMin, curveMax,  //br --> bl
            
                (region_vec.x+region_vec.w), (region_vec.y), 0.0 , 
                scan_rgn.x+scan_rgn.z, scan_rgn.y+scan_rgn.w, curveMin, curveMax, //tr --> br
            
                (region_vec.x), (region_vec.y+region_vec.z), 0.0 , 
                scan_rgn.x , scan_rgn.y, curveMin, curveMax //bl --> tl
            };

            const size_t NV = sizeof(out_rect) / sizeof(msdf_vert);

            ctx->g.push_verts(out_rect, NV);
        } else {
            msdf_vert out_rect[] = { 
                (region_vec.x), (region_vec.y), 0.0 , 
                scan_rgn.x, scan_rgn.y, curveMin, curveMax, 

                (region_vec.x), (region_vec.y+region_vec.w), 0.0 , 
                scan_rgn.x , (scan_rgn.y + scan_rgn.w), curveMin, curveMax, 

                (region_vec.x+region_vec.z), (region_vec.y), 0.0 , 
                (scan_rgn.x+scan_rgn.z), (scan_rgn.y),  curveMin, curveMax, 
            
                (region_vec.x+region_vec.z), (region_vec.y+region_vec.w), 0.0 , 
                scan_rgn.x+scan_rgn.z , scan_rgn.y+scan_rgn.w, curveMin, curveMax, 
            
                (region_vec.x+region_vec.z), (region_vec.y), 0.0 , 
                scan_rgn.x+scan_rgn.z, scan_rgn.y, curveMin, curveMax, 
            
                (region_vec.x), (region_vec.y+region_vec.w), 0.0 , 
                scan_rgn.x , scan_rgn.y+scan_rgn.w, curveMin, curveMax
            };

            const size_t NV = sizeof(out_rect) / sizeof(msdf_vert);

            ctx->g.push_verts(out_rect, NV);
        }

        DeleteMsdfGenContext(&g_ctx);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ctx->curveBuffer);
    ctx->g.render_flush();

    return 0;
}

///////////////////////
///////////////////////
///////////////////////

void WriteGPUContextToBitmap(MsdfGpuContext *ctx, Bitmap *map) {
    if (!ctx || !map) return;

    FrameBuffer fb = ctx->fb;
    fb.extractToBitmap(map);
}

i32 ttfRender::RenderGlyphMSDFToBitMap(Glyph tGlyph, Bitmap* map, sdf_dim size, bool accel) {
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
    if (accel) {
        MsdfGpuContext* ctx = CreateMsdfGPUAccelerationContext_Dynamic(sdfTrueW, sdfTrueH);

        if (!ctx) {
            return 1;
        }

        i32 stat = render_positioned_msdf_gpu_accel(tGlyph, ctx, 0, 0, sdfTrueW, sdfTrueH, 0, 0, 0, 0);

        WriteGPUContextToBitmap(ctx, map);

        _safe_free_a(ctx);
        return stat;
    } else
        return render_positioned_msdf(tGlyph, map, 0, 0, sdfTrueW, sdfTrueH, 0, 0, 0, 0);
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

    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints);

    f32 t;

    const f32
        wc = glyphW / (f32) sdfTrueW,
        hc = glyphH / (f32) sdfTrueH;

    u32 pos;

    for (Edge e : glyphEdges.edges) {
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
    const _Ty temp = *a;
    *a = *b;
    *b = temp;
}

template<class _Ty> i32 _partition(_Ty *arr, i32 (*cmp)(_Ty, _Ty), i32 low, i32 high) {
    _Ty p = arr[high];

    i32 i = low - 1;
    
    for (i32 j = low; j < high; j++) {
        if (cmp(arr[j], p)) {
            i++;
            mu_swap(arr + i, arr + j);
        }
    }

    mu_swap(arr + i + 1, arr + high);

    return i + 1;
}

//quick sort function
//cmp is a lambda that compares the two values given to it
// ie [](Obj a, Obj b) return i32;
//returning a 0 is equivalent to the values being equal
//returning < 0 is equivalent to value a being less then value b
//returning > 0 is equivalent to value a being greater then value b
//Ie: sort from least to greatest --> return a-b
//Ie: sort from greatest to least --> return b-a
template<class _Ty> void mu_qsort(_Ty *arr, i32 (*cmp)(_Ty, _Ty), size_t len, i32 _low = 0, i32 _high = 0x7fffffff) {
    if (!arr || _low >= _high)
        return;

    if (_high == 0x7fffffff) _high = len - 1;
    
    i32 p = _partition(arr, cmp, _low, _high);

    mu_qsort(arr, cmp, len, _low, p - 1);
    mu_qsort(arr, cmp, len, p + 1, _high);
}

i32 _glyphCmp(Glyph a, Glyph b) {
    const i32 Aa = /*(a.xMax - a.xMin) **/ (a.yMax - a.yMin),
              Ab = /*(b.xMax - b.xMin) **/ (b.yMax - b.yMin);

    return Ab < Aa;
};

//
FontInst ttfRender::GenerateUnicodeMSDFSubset(std::string src, UnicodeRange range, sdf_dim first_char_size, bool accel) {
    //param checks and shit
    FontInst font;

    const f32 padding_per_32 = 1.0f;

    // get all the glyphs
    GlyphSet glyphs = ttfParse::GenerateGlyphSet(src, range);
    ttfFile f = glyphs.file;

    std::cout << "Generating " << glyphs.nGlyphs << " glyphs..." << std::endl;

    if (glyphs.nGlyphs == 0)
        return font;

    font.range = range;

    //set some info needed later for spacing calculations
    font.ad_inf.unitsPerEm = f.header.unitsPerEm;
    font.ad_inf.ascent = f.h_inf.ascent;
    font.ad_inf.descent = f.h_inf.descent;

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

    //const u32 padding = ceil((p_const / 32.0f) * padding_per_32);
    const f32 padding = 0.1f;

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

    CharSpritePos *c_pos = new CharSpritePos[glyphs.nGlyphs];

    constexpr size_t nFontMapHashBits = 15;
    
    if (glyphs.nRanges == 1) {
        font.map.ty = CharMapType::Direct;
        font.map.hash_inf.sz = glyphs.nGlyphs;
    } else {
        font.map.hash_inf.nBits = nFontMapHashBits;
        font.map.hash_inf.sz = 1 << font.map.hash_inf.nBits;
        font.map.ty = CharMapType::Hash;
        font.map.hash_map = new CharLink[nFontMapHashBits];
        ZeroMem(font.map.hash_map, nFontMapHashBits);
    }

    //TODO: populate the font map!!!

    while (i < glyphs.nGlyphs) {
        Glyph g = gly[i]; //get the current glyph

        //detect if it is a missing character
        if (g.char_id < 0) {
            c_pos[i] = {
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
        bool fit_rotated = false;

        const size_t nRegions = rgn_stack.size();

        i32 gw = ((g.xMax - g.xMin) * (1.0f + padding * 2.0f)) * scale, gh = ((g.yMax - g.yMin ) * (1.0f + padding * 2.0f)) * scale;

        for (j = 0; j < nRegions; j++) {
            Rn = rgn_stack[j];

            //make sure it fits
            if ((Rn.w < gw && Rn.w > 0) || (Rn.h < gh && Rn.h > 0)) continue;

            const u32 fit = mu_max(Rn.x + gw, sheet_w) * mu_max(Rn.y + gh, sheet_h);

            if (fit < smallest_fit) {
                low_age = Rn.age;
                best_rgn = j;
                smallest_fit = fit;
                fit_rotated = false;
            } else if (fit == smallest_fit && Rn.age < low_age) {
                low_age = Rn.age;
                best_rgn = j;
                smallest_fit = fit;
                fit_rotated = false;
            }

            //check potential 90 deg rotating benefits
            if ((Rn.w < gh && Rn.w > 0) || (Rn.h < gw && Rn.h > 0)) continue;
            const u32 fit90 = mu_max(Rn.x + gh, sheet_w) * mu_max(Rn.y + gw, sheet_h);

            /*if (fit90 < smallest_fit) {
                low_age = Rn.age;
                best_rgn = j;
                smallest_fit = fit90;
                fit_rotated = true;
            }*/
        }

        //add thing to sheet position or something
        SpriteRegion target_rgn = rgn_stack[best_rgn];

        if (target_rgn.x < 0 || target_rgn.y < 0 || gw < 0 || gh < 0) {
            std::cout << "error invalid region!" << std::endl;
            i++;
            continue;
        }

        //std::cout << "T Region: " << target_rgn.x << " " << target_rgn.y << std::endl;

        c_pos[i] = {
            .x = (u32) target_rgn.x,
            .y = (u32) target_rgn.y,
            .w = (u32) (fit_rotated ? gh : gw),
            .h = (u32) (fit_rotated ? gw : gh),
            .rotate = fit_rotated
        };

        //std::cout << "POS: Draw(" << target_rgn.x << "," << target_rgn.y << "," << gw << "," << gh << ") " << (gw*gh) << std::endl;

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

        //add character info
        Character ochar;

        ochar.dim.w = g.xMax - g.xMin;
        ochar.dim.h = g.yMax - g.yMin;

        //TODO:
        switch (font.map.ty) {
        case CharMapType::Direct: {
            font.map.hash_map[g.char_id]->ochar = ochar;
            break;
        }
        case CharMapType::Hash: {

            break;
        }
        default:
            std::cout << "ttf_render error: invalid char_map_type when generating msdfs" << std::endl;
            break;
        }

        //
        i++;
    }

    //generate the spritesheet

    CharSpritePos r_pos;

    MsdfGpuContext *a_ctx = nullptr;

    if (accel) {
        a_ctx = CreateMsdfGPUAccelerationContext(sheet_w, sheet_h);
        font.msdf_dat.dim.w = a_ctx->fb.w;
        font.msdf_dat.dim.h = a_ctx->fb.h;
        render_multi_positioned_msdf_gpu_accel(gly, c_pos, glyphs.nGlyphs, a_ctx, padding);
    } else {
        font.msdf_dat.mode = MsdfMode::Bitmap;
        Bitmap *sheet = new Bitmap;
        sheet->data = new byte[(sheet_w * sheet_h) * 3];
        ZeroMem(sheet->data, (sheet_w * sheet_h) * 3);
        sheet->header.w = sheet_w;
        sheet->header.h = sheet_h;
        sheet->header.bitsPerPixel = 24;
        sheet->header.fSz = (sheet_w * sheet_h) * 3;

        font.msdf_dat.MSDF.bitmap = sheet;

        for (i = 0; i < glyphs.nGlyphs; i++) {
            r_pos = c_pos[i];

            if (r_pos.w <= 0 || r_pos.h <= 0)
                continue;

            //use gpu acceleration if needed
            //TODO: add memory management for the frame buffer and delete the buffer
            render_positioned_msdf(
                gly[i], 
                sheet, 
                r_pos.x, r_pos.y, r_pos.w, r_pos.h, 
                padding, padding, padding, padding
            );
            //std::cout << "Generated Glyph: " << i << std::endl;
        }
    }

    if (accel) {
        font.msdf_dat.mode = MsdfMode::GL_Texture;
        font.msdf_dat.MSDF.gl_texture = BindableTexture(a_ctx->fb.getTextureHandle(), a_ctx->fb.w, a_ctx->fb.h);
    }

    //font._dbg_ctx = a_ctx;

    //memory management
    _safe_free_a(gly);

    //add font info stuff
    font.inf = f.h_inf;
    font.ad_inf.monospace = (font.inf.nLongHorMetrics == 1);

    return font;
}

void ttfRender::_msdfRenderDebug(Glyph g, MsdfGpuContext** ctx) {
    CharSpritePos r_pos;

    if (!*ctx) {
        std::cout << "Creating context!" << std::endl;
        *ctx = CreateMsdfGPUAccelerationContext_Dynamic(256, 256);
    }

    render_positioned_msdf_gpu_accel(
        g, 
        *ctx, 
        0, 0, 256, 256, 
        0, 0, 0, 0
    );
}

void DeleteFontInst(FontInst *font) {
    if (!font) return;

    if (font->msdf_dat.MSDF.bitmap) {
        Bitmap::Free(font->msdf_dat.MSDF.bitmap);
        _safe_free_b(font->msdf_dat.MSDF.bitmap);
        font->msdf_dat.MSDF.bitmap = nullptr;
    }

    font->msdf_dat.MSDF.gl_texture.free();

    if (font->map.hash_map) {
        _safe_free_a(font->map.hash_map);
        font->map.hash_map = nullptr;
        font->map.hash_inf.sz = 0;
        font->map.ty = (CharMapType) 0;
    }

    font->good = false;
}

/*

It's weird how the faster my code gets the more lines it takes

*/

void GenerateFontInstanceTexture(FontInst *font, bool keep_redudant_data = false) {
    if (!font) return;

    if (!font->good) {
        std::cout << "ttf_render error | failed to generate font instance: BAD INSTANCE" << std::endl;
        return;
    }

    switch (font->mode) {
    case FontMode::MSDF: {
        switch (font->msdf_dat.mode) {
        case MsdfMode::GL_Texture: {
            BindableTexture tex = font->msdf_dat.MSDF.gl_texture;
            if (!tex.getHandle()) {
                goto _bmp_cvrt;
            }
            break;
        }
        case MsdfMode::Bitmap: {
        _bmp_cvrt:
            Bitmap *bmp = font->msdf_dat.MSDF.bitmap;
            const BitmapStatus bmp_stat = Bitmap::BitmapCheck(bmp);

            if (!bmp || bmp_stat != BitmapStatus::Good) {
                std::cout << "ttf_render error | invalid font bitmap" << std::endl;
                font->good = false;
                return;
            }

            font->msdf_dat.MSDF.gl_texture = BindableTexture(bmp);

            if (!keep_redudant_data)
                Bitmap::Free(bmp);

            _safe_free_b(bmp);
            font->msdf_dat.MSDF.bitmap = nullptr;

            break;
        }
        default:
            std::cout << "ttf_render error | invalid msdf format" << std::endl;
            font->good = false;
            return;
        }
        break;
    }
    default:
        //nothing to convert
        break;
    }
}

/*

Actual font rendering code :OOO

Copyright muffinshades & Lambdana software 2026-present

*/

///textrendering through graphics related code=
constexpr size_t n_gf_buf_verts = 0xffff;

void graphics::ini_generic_font_state() {
    this->useGraphicsState(&generic_font_state);
    this->iniDynamicGraphicsState(n_gf_buf_verts);

    gfont_s_ready = true;
}

struct str_pre_metrics {
    i32 maxW, maxH;
    size_t str_len;
    i32 line_h;
    f32 stride_factor; //amount to multiply each stride by in order to get proper pt scale

    //basically just multiply a glyph's width (xMax - xMin) by this value to get its output screen pixel width
    //then compute the output height of the glyph using the ratio of the glyphs height to width and the compute pixel width above
    f32 WemRatio;

    i32 tab_distance = 0;
};

//precomputations needed for text rendering
/*

Flag Format

bit 1 (bool): whether or not every characters' position and dimensions should be calculated and stored in the metrics object
    --> exists because there is a function to compute things like the bounding box of the strings and the actual positions are not needed
    --> also this whole computation might be done whilsts redering each glyph so like yeah this functionality might be scraped but idgaf

bits 2-8: reserved

*/
#ifdef WIN32
    constexpr u32 standardDPI = 96;
#else
    constexpr u32 standardDPI = 72;
#endif

constexpr f32 ptInch = 0.01389f;
constexpr f32 ptem = 0.08364583416f;
constexpr f32 empt = 11.955168f;

str_pre_metrics computePreStringMetrics(FontInst *font, f32 x, f32 y, f32 z, const char* str, GenericFontProperties prop, u8 flags) {
    str_pre_metrics metrics;

    //compute string length
    size_t len = 0;
    const char* l_cmp = str;

    while (*l_cmp != 0x00) {
        len++;
        l_cmp++;
    }

    metrics.str_len = len;

    //pt based calculations
    const f32 px_per_pt = ptInch * standardDPI;
    const i32 px_w = prop.scale.pt * px_per_pt;
    metrics.WemRatio = px_w / ((f32) font->ad_inf.unitsPerEm); //width based em ratio

    //compute font line height
    metrics.line_h = abs(font->ad_inf.descent - font->ad_inf.ascent);

    //
    return metrics;
}

struct StrRenderContext {
    char *cur_char = nullptr;
    f32 x = 0, baseline_y = 0;
};

struct genericFontVert {
    f32 pos[3];
    f32 tex[2];
};

//TODO: coordinate this process with graphics states
void graphics::RenderString(FontInst *font, f32 x, f32 y, f32 z, const char* str, GenericFontProperties prop) {
    if (!font || !str)
        return;

    if (!gfont_s_ready)
        ini_generic_font_state();

    //check font instance and hash map stuff

    //compute the stirng metrics first
    constexpr u8 met_flg = 0b10000000;
    str_pre_metrics metrics = computePreStringMetrics(font, x, y, z, str, prop, met_flg);

    //render setup
    StrRenderContext s_ctx = {
        .cur_char = (char*) str,
        .x = x,
        .baseline_y = y + font->ad_inf.ascent //add max ascent to get proper positioning for the text basline
    };

    //begin render
    this->render_begin();

    char cc;
    i32 p;

    while ((cc = *s_ctx.cur_char) != 0x00) {
        switch (cc) {
        //New Line
        case 0x0A:
            s_ctx.baseline_y += metrics.line_h;
            break;
        //Tab
        case 0x09:
            s_ctx.x += metrics.tab_distance;
            break;
        //carriage return (basically home button)
        case 0x0D:

            break;
        default:

        //check for blank characters rq
        if (cc == 0x00) {
            break;
        }
        //

        //find the character info
        Character o_char; //TODO: set this to the missing char for easy escape if given char does not exist

        if (font->map.ty == CharMapType::Direct && cc < font->map.hash_inf.sz)
            o_char = font->map.hash_map[cc].ochar;
        else {
            const u32 hVal = compute_basic_hash_32(font->map.hash_inf.nBits, &cc, 1);
            CharLink *lnk = (font->map.hash_map+hVal);

            //render missing glyph if bad
            while (lnk && lnk->ochar.val != cc) {
                lnk = lnk->next;
            }

            if (lnk) {
                o_char = lnk->ochar;
            }
        }

        f32 w, h;

        //
        for (p = 0; p < o_char.nParts; p++) {
            CharPart cp = o_char.spriteParts[p];

            w = o_char.dim.w;

            // z
            //cp.sheet_loc.x, cp.sheet_loc.y, cp.sheet_loc.w, cp.sheet_loc.h

            genericFontVert glyph_rect_base[] = { 
                (region_vec.x), (region_vec.y), 0.0 , 
                scan_rgn.x, scan_rgn.y, curveMin, curveMax, 

                (region_vec.x), (region_vec.y+region_vec.w), 0.0 , 
                scan_rgn.x , (scan_rgn.y + scan_rgn.w), curveMin, curveMax, 

                (region_vec.x+region_vec.z), (region_vec.y), 0.0 , 
                (scan_rgn.x+scan_rgn.z), (scan_rgn.y),  curveMin, curveMax, 
            
                (region_vec.x+region_vec.z), (region_vec.y+region_vec.w), 0.0 , 
                scan_rgn.x+scan_rgn.z , scan_rgn.y+scan_rgn.w, curveMin, curveMax, 
            
                (region_vec.x+region_vec.z), (region_vec.y), 0.0 , 
                scan_rgn.x+scan_rgn.z, scan_rgn.y, curveMin, curveMax, 
            
                (region_vec.x), (region_vec.y+region_vec.w), 0.0 , 
                scan_rgn.x , scan_rgn.y+scan_rgn.w, curveMin, curveMax
            };
        }
        


        break; //end of switch statement default branch
        }

        //advance to next character
        s_ctx.cur_char++;
    }
}