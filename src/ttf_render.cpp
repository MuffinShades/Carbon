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

#define MSDF_ACCEL_SHADER_PATH_VERT "../../src/msdf_gl_accel_vert.glsl"
#define MSDF_ACCEL_SHADER_PATH_FRAG "../../src/msdf_gl_accel.glsl"
#define MSDF_ACCEL_CC_SHADER_PATH_VERT "../../src/msdf_precision_correction_vert.glsl"
#define MSDF_ACCEL_CC_SHADER_PATH_FRAG "../../src/msdf_precision_correction_frag.glsl"
#define MSDF_ACCEL_CC_SHADER_PATH_TCS "../../src/msdf_precision_correction_tcs.glsl"
#define MSDF_ACCEL_CC_SHADER_PATH_TES "../../src/msdf_precision_correction_tes.glsl"
#define MSDF_ACCEL_CC_COMPOSITE_SHADER_PATH_VERT "../../src/msdf_pc_appl_vert.glsl"
#define MSDF_ACCEL_CC_COMPOSITE_SHADER_PATH_FRAG "../../src/msdf_pc_appl_frag.glsl"

constexpr f32 smol_number = 1.175e-38f; //number that is smol
constexpr f32 chonk_number = 3.402e38f; //number that is chonk

constexpr f64 very_smol_number = DBL_MIN;
constexpr f64 very_chonk_number = DBL_MAX;


//frequencies for the whole heuristics thingy to load common characters into memory better
constexpr f32 freq_scale_base = 1.0f;
constexpr f32 latin_simp_freq[] = {1.174345784f,1.01070806f,1.06843762f,1.113440833f,1.212200993f,1.052606186f,
                          1.040554675f,1.143849268f,1.164203862f,0.75f,0.9364109091f,1.105529572f,
                          1.064809002f,1.159329957f,1.16896916f,1.030015864f,0.7591983745f,1.145465887f,
                          1.149546587f,1.185342532f,1.074309442f,0.9822939953f,1.043365841f,0.8012108714f,
                          1.04428499f,0.7155773422f};

/**
 *
 * All le code for rendering dem glyphs
 *
 * Written by muffinshades 2024-2026
 *
 */

extern void find_font_curve_friends(FontInst *font);

static Shader msdf_gen_shader;
static Shader msdf_gen_cc_shader;
static Shader msdf_gen_cc_composite_shader;

struct msdf_vert {
    f32 pos[3];
    f32 glyph_rel_region[2];
    i32 curve_range[2];
}; 

struct con_correct_vert {
    f32 pos[3];
    i32 curve_idx;
};

struct con_correct_composite_vert {
    f32 pos[3];
    f32 tex[2];
};

MsdfGpuContext *CreateMsdfGPUAccelerationContext(u32 w, u32 h) {
    MsdfGpuContext *ctx = new MsdfGpuContext;

    //normal render descriptor
    ctx->def_desc = {
        .dynamic = true
    };

    //contour correction descriptor
    ctx->cc_desc = {
        .dynamic = true,
        .render_primitive = GL_PATCHES
    };

    ctx->cc_composite_desc = {
        .dynamic = true
    };

    //load graphics
    ctx->g = graphics(ctx->def_desc);

    ctx->g.VertexDefineBegin(sizeof(msdf_vert));
    ctx->g.DefineVertexPart(0, vertexClassPart(msdf_vert, pos));
    ctx->g.DefineVertexPart(1, vertexClassPart(msdf_vert, glyph_rel_region));
    ctx->g.DefineIntegerVertexPart(2, vertexClassPart(msdf_vert, curve_range));
    ctx->g.VertexDefineEnd();

    //load cc graphics
    ctx->cc_rstate = ctx->g.CreateNewRenderState(ctx->cc_desc);
    ctx->g.SetRenderState(ctx->cc_rstate);

    ctx->g.VertexDefineBegin(sizeof(con_correct_vert));
    ctx->g.DefineVertexPart(0, vertexClassPart(con_correct_vert, pos));
    ctx->g.DefineIntegerVertexPart(1, vertexClassPart(con_correct_vert, curve_idx));
    ctx->g.VertexDefineEnd();

    //load cc composite graphics
    ctx->cc_composite_rstate = ctx->g.CreateNewRenderState(ctx->cc_composite_desc);
    ctx->g.SetRenderState(ctx->cc_composite_rstate);

    ctx->g.VertexDefineBegin(sizeof(con_correct_composite_vert));
    ctx->g.DefineVertexPart(0, vertexClassPart(con_correct_composite_vert, pos));
    ctx->g.DefineVertexPart(1, vertexClassPart(con_correct_composite_vert, tex));
    ctx->g.VertexDefineEnd();

    ctx->g.RestoreDefaultRenderState(); //restore the default state

    //set output device
    FrameBufferExtInf inf = {
        .mipmap = false
    };

    FrameBufferExtInf cc_inf = {
        .color_fmt = GL_RGBA32F,
        .data_color_fmt = GL_RGBA
    };

    FrameBufferExtInf cc_composite_inf = {
        .color_fmt = GL_RGBA8,
        .data_color_fmt = GL_RGBA
    };

    ctx->fb = FrameBuffer(FrameBuffer::Texture, w, h, inf);
    ctx->cc_fb = FrameBuffer(FrameBuffer::Texture, w, h, cc_inf); //contour correction buffer
    ctx->cc_composite_fb = FrameBuffer(FrameBuffer::Texture, w, h, cc_composite_inf);

    ctx->good = true;

    return ctx;
}

MsdfGpuContext *CreateMsdfGPUAccelerationContext_Dynamic(u32 w, u32 h) {
    MsdfGpuContext *ctx = new MsdfGpuContext;

    RenderStateDescriptor def_desc = {
        .dynamic = true
    };

    //load graphics
    ctx->g = graphics(def_desc);

    ctx->g.VertexDefineBegin(sizeof(msdf_vert));
    ctx->g.DefineVertexPart(0, vertexClassPart(msdf_vert, pos));
    ctx->g.VertexDefineEnd();

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

struct simple_connection {
    volatile i32 g;
};

struct gPData {
    std::vector<Point> p;
    std::vector<i32> f;
    std::vector<simple_connection> connections;
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

        if (cu->bounds.r <= 0)
            computeCurveBoundingRadius(cu);

        csumX += cu->bounds.center.x;
        csumY += cu->bounds.center.y;
    }

    const f32 is = 1.0f / (f32) nc;

    Point center = {.x = csumX * is, .y = csumY * is};

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

////////////////////////////////////////////////////////////////////
// most of the ray count related functions/functionality ///////////
////////////////////////////////////////////////////////////////////

struct rcGenContext {
    gpu_rc_curve* curveBuf = nullptr;
    size_t nCurves = 0, wOff = 0, c = 0; //number of curves and write offset
};

rcGenContext *create_rc_gen_ctx(size_t nTotalCurves) {
    if (nTotalCurves == 0)
        return nullptr;

    rcGenContext *rctx = new rcGenContext;

    if (!rctx) {
        std::cout << "err failed to generact rcGenContext : bad alloc" << std::endl;
        return nullptr;
    }

    const size_t nnc = nTotalCurves << 1;
    rctx->curveBuf = new gpu_rc_curve[nnc];

    if (!rctx->curveBuf) {
        std::cout << "err failed to generate rcGenContext : bad alloc (on cu-buff)" << std::endl;
        return nullptr;
    }

    ZeroMem(rctx->curveBuf, nnc);

    return rctx;
}

void delete_rc_gen_ctx(rcGenContext *&rctx) {
    if (!rctx) return;
    if (rctx->curveBuf)
        _safe_free_a(rctx->curveBuf);
    _safe_free_b(rctx);
    rctx = nullptr;
}

constexpr size_t rcCtxBufAdd = 0x3ff; //add space for another 1023 curves

//random kinda niche function that should not have to be called if we do stuff right but just incase I don't want the whole thing
//blowing itself up because it doesn't have enough curve space
void rc_gen_realloc_extra(const size_t extra, rcGenContext *ctx) {
    if (!ctx || extra == 0) return;

    ctx->nCurves += extra;
    auto *nBuf = new gpu_rc_curve[ctx->nCurves];

    if (!nBuf) {
        std::cout << "failed to allocate new rc curve buffer :(" << std::endl;
        ctx->nCurves -= extra;
        return;
    }

    ZeroMem(nBuf, ctx->nCurves);

#ifdef CHECK_MEM_TAMPER
    if (ctx->nCurves < extra && ctx->curveBuf) {
        in_memcpy(nBuf, ctx->curveBuf, (ctx->nCurves - extra) * sizeof(gpu_rc_curve)); //copy over le curves
        _safe_free_a(ctx->curveBuf);
    } else {
        std::cout << "someone is tampering with memory... | err failed to copy over new curve buffer" << std::endl;
        return;
    }
#else
    if (ctx->curveBuf) {
        in_memcpy(nBuf, ctx->curveBuf, (ctx->nCurves - extra) * sizeof(gpu_rc_curve)); //copy over le curves
        _safe_free_a(ctx->curveBuf);
    }
#endif

    ctx->curveBuf = nBuf;
}

inline void rc_process_curve(rcGenContext *ctx, BCurve *cu, i32 connect) {
    if (!ctx || !cu) return;

    if (ctx->nCurves - ctx->wOff < 2 || ctx->nCurves <= ctx->wOff) 
        rc_gen_realloc_extra(rcCtxBufAdd, ctx);

    const Point b = pointScale(pointSub(cu->p[1], cu->p[0]), 2.0f);
    const f32 dz = (-0.5f * b.y) / (cu->p[0].y - 2.0f * cu->p[1].y + cu->p[2].y);

    if (dz >= 1 || dz <= 0) {
        ctx->curveBuf[ctx->wOff++] = {
            .p0 = {cu->p[0].x, cu->p[0].y},
            .p1 = {cu->p[1].x, cu->p[1].y},
            .p2 = {cu->p[2].x, cu->p[2].y},
            .cu_connect = connect
        };
    } else { //more than 1 curve
        const f32 dz2 = dz*dz;

        //just stole ts from sebastian lague's video (https://www.youtube.com/watch?v=SO83KQuuZvg&t=3211s) cause im too tired to derive it myself :P
        const Point aa = pointSub(cu->p[0], pointSub(pointScale(cu->p[1], 2.0f), cu->p[2]));
        Point piv = pointAdd(pointScale(aa, dz2), pointAdd(pointScale(b, dz), cu->p[0]));
        const f32 la = (piv.y - cu->p[0].y) / b.y,
                  lb = (piv.y - cu->p[2].y) / (2.0f * aa.y + b.y); //labubu
        Point pa = pointAdd(cu->p[0], pointScale(b, la)),
              pb = pointAdd(cu->p[2], pointScale(pointAdd(pointScale(aa, 2.0f), b), lb));

        //adjust the connections
        //that's gonna be fun
        const u32 c0 = ctx->wOff, c1 = ctx->wOff+1;
        i32 con0 = (connect & 0xFFFF0000)
                 | (c1 & 0x7FFF), // Creates connection where left half (con0) is the original connection and the right half (con1 is the next curve p0)
            con1 = 
                 ((0x8000 + (c0 & 0x7FFF)) << 16) | //p0 connection for curve 2
                 (
                    ((connect + 1) & 0x7FFF) | 
                    (connect & 0x8000)
                );

        ctx->curveBuf[ctx->wOff++] = {
            .p0 = {cu->p[0].x, cu->p[0].y},
            .p1 = {pa.x, pa.y},
            .p2 = {piv.x, piv.y},
            .cu_connect = con0
        };

        ctx->curveBuf[ctx->wOff++] = {
            .p0 = {piv.x, piv.y},
            .p1 = {pb.x, pb.y},
            .p2 = {cu->p[2].x, cu->p[2].y},
            .cu_connect = con1
        };
    }
}

////////////////////////////////////////////////////////////////////

struct glfEdgeObject {
    size_t nCurves;
    BCurve* curveBuff = nullptr;
    std::vector<Edge> edges;
};

glfEdgeObject generateGlyphEdges(Glyph glyph_data, gPData& points, size_t nPoints, rcGenContext*& rcCtx, bool gen_rc_ctx = false) {
    glfEdgeObject eObj;

    if (!glyph_data.modifiedContourEnds) {
        std::cout << "ttf_render error: cannot generate glyph edges --> poor contour data" << std::endl;
        return eObj;
    }

    const size_t nCurves = nPoints >> 1;
    eObj.curveBuff = new BCurve[nCurves];
    ZeroMem(eObj.curveBuff, nCurves);
    BCurve* curveBuffer = eObj.curveBuff; //stores curves of current edge

    eObj.nCurves = 0;

    if (!rcCtx && gen_rc_ctx) {
        rcCtx = create_rc_gen_ctx(nCurves);
    }

    BCurve *workingCurve = curveBuffer;

    i32 i, ci;
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
    i32 contStart = rcCtx->wOff, c = 0;
    i32 cur_connect = 0;

    for (i = 0; i < nPoints; i++) {
        p = points.p[i];
        workingCurve->p[pSelect] = p;

        if (i == glyph_data.modifiedContourEnds[c] && pSelect < 2) {
            std::cout << "ttf warning: funky contour ending!" << std::endl;
        }

        /*
            ================Format of cu_connect================

            1 i32 split into 2 i16s

            0x00000000 --> 0x0000 << 16 + 0x0000

            Formatted big endian such that I0 << 16 + I1

            i16 0: refers to the connection for point 0 on the curve
            i16 1: refers to the connection for point 2 on the curve

            Each i16 follows the following format:
            *note in the following the bits will read left to right (big endian)

            Bit 0 1 2 3 4 5 6 7 ... 15
             0b C I I I I I I I ... I

            C --> 1bit curve selector that determines whether the curve is connected to point 0 or 1
            C=0: curve is attached to p0 of the other curve
            C=1: curve is attached to p2 of the other curve
            I --> 15bit uint that stores the index of the curve that the connection references
        */

        #define _wOff(ctx) ctx->wOff
        #define _wOff_next(ctx) (ctx->wOff+1)
        #define _wOff_prev(ctx) (ctx->wOff-1)
        #define _mask_con0(co) co &= 0xFFFF0000
        #define _mask_con2(co) co &= 0x0000FFFF
        #define _set_con0(co, pSelect, cuIdx) co |= (((((pSelect) & 1) << 15) | ((cuIdx) & 0x7FFF)) << 16)
        #define _set_con2(co, pSelect, cuIdx) co |= ((((pSelect) & 1) << 15) | ((cuIdx) & 0x7FFF))
        #define TTF_CU_CONNECTION_SELECT_P0 0
        #define TTF_CU_CONNECTION_SELECT_P2 1

        if (pSelect++ == 2) {
            //process for ray thingy working curve
            _mask_con0(cur_connect);

            if (rcCtx) { 
                if ((i == glyph_data.modifiedContourEnds[c] || i >= nPoints-3) && rcCtx->nCurves > contStart) {     // -----------------------------------------------------------------
                    _mask_con0(rcCtx->curveBuf[contStart].cu_connect);                                              // Discard any junk in the p0 slot of the contour curve's connection
                    _set_con0(rcCtx->curveBuf[contStart].cu_connect, TTF_CU_CONNECTION_SELECT_P2, _wOff(rcCtx));    // Set the p0 slot of the contour curve's connection to the second point in the current curve (final point in the contour)
                    _set_con2(cur_connect, TTF_CU_CONNECTION_SELECT_P0, contStart);                                 // Set the p2 slot of the current curve's connection to be the p0 (first) point of the first curve in the contour
                    contStart = _wOff_next(rcCtx);                                                                  // Set the new contour start to be the next curve (NEED TO VERIFY THIS IS RIGHT AND NOT _wOff)
                    c++;                                                                                            // Move onto the next contour (increment the current contour counter)
                } else {                                                                                            // -----------------------------------------------------------------
                    _set_con2(cur_connect, TTF_CU_CONNECTION_SELECT_P0, _wOff_next(rcCtx));                         // By default: set the p2 slot of the current connection to be the first point of the next connection
                }                                                                                                   // -----------------------------------------------------------------

                rc_process_curve(rcCtx, workingCurve, cur_connect);

                if (_wOff(rcCtx) == 0) {
                    std::cout << "concerning warning: wOff was not incremented in rc_process_curve" << std::endl;
                    goto _set_cur_con_0_skip;
                }

                /*
                Set the connection for p0 (_set_con0) of the next curve to the last
                point of the current curve | point: p2, index: prev_woff (since after rc_process call)
                */
                _set_con0(cur_connect, TTF_CU_CONNECTION_SELECT_P2, _wOff_prev(rcCtx)); //this is good and correct
            }

            _set_cur_con_0_skip:

            //add curve to curve buffer / edge curves
            nEdgeCurves++;
            eObj.nCurves++;
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
                snip = snip || (abs(cross) > 0.01f);
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
    rcGenContext *_disc_ctx = nullptr;
    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints, _disc_ctx);

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

/*MsdfGenContext CreateMsdfGenContext(Glyph tGlyph, bool accel = false) {
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
}*/

void ConfigureGenContext(MsdfGenContext *ctx, Glyph tGlyph, rcGenContext*& rc_ctx, bool accel = false) {
    //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    //blank glyph so blank sdf
    if (nPoints < 3) {
        ctx->nCurves = 0;
        ctx->curves = nullptr;
        return;
    }

    //curve and edge generation, glyph clean up, and more
    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints, rc_ctx, true);

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
        ctx->curves = new gpu_light_curve[nCurves];
        ZeroMem((gpu_light_curve*) ctx->curves, nCurves);
        ctx->curveSize = sizeof(gpu_light_curve);
        ctx->curveType = MsdfGenContext::LightCurve;
    } else {
        ctx->curves = new MsdfCurve[nCurves];
        ZeroMem((MsdfCurve*) ctx->curves, nCurves);
        ctx->curveSize = sizeof(MsdfCurve);
        ctx->curveType = MsdfGenContext::NormalCurve;
    }

    gpu_light_curve *lc_buff = (gpu_light_curve*) ctx->curves;
    MsdfCurve *nc_buff = (MsdfCurve*) ctx->curves;
    
    ctx->nCurves = nCurves;
    
    Edge E;
    BCurve qu;
    size_t cpi = 0;

    for (c = 0; c < tGlyph.nContours; c++) {
        Contour ct = glyph_contours[c];

        const size_t ncEdges = ct.edge_idxs.size();
        u32 t_edge;

        //case where there are only 0 or 1 edges
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

        //assign a color to each edge
        cur_color = uvec3(1,0,1);

        for (i = 0; i < ncEdges; i++) {
            t_edge = ct.edge_idxs[i];
            E = glyphEdges.edges[t_edge];

            if (!E.curves)
                continue;

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
}

MsdfGenContext CreateMsdfGenContext(Glyph tGlyph, rcGenContext*& rc_ctx, bool accel = false) {
    MsdfGenContext ctx = {
        .curves = nullptr,
        .nCurves = 0
    };

    ConfigureGenContext(&ctx, tGlyph, rc_ctx, accel);

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
    rcGenContext *_disc_ctx = nullptr;
    MsdfGenContext g_ctx = CreateMsdfGenContext(tGlyph, _disc_ctx);

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

    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints, ctx->rc_ctx, true);

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

    ctx->g.SetShader(&msdf_gen_shader);
    
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
    msdf_vert out_rect[] = RECT_VERTS(region_vec.x, region_vec.y, region_vec.z, region_vec.w, 0.0 COMMA (f32) glyph_dim_vec.x COMMA (f32) glyph_dim_vec.y COMMA (i32) glyph_dim_vec.x COMMA (i32) glyph_dim_vec.w);

    ctx->g.RenderBegin();
    ctx->g.PushVerts(out_rect, sizeof(out_rect) / sizeof(msdf_vert), true);
    ctx->g.RenderFlush();

    _safe_free_a(glyph_contours); //more memory management

    //oh look were managing memory :O
    _safe_free_a(glyphEdges.curveBuff);

    return 0;
}

/*

TODO: actually properly implement this function

This will basically render each glyph by just rendering their outlines to a certain scaled bitmap by rounding down each point to the nearest pixel
    * if two lines don't matematically intersect but two curves write to the same point, then there is a feature that is too small
      to represent on the grid
    * For msdf --> add the curves and their color regardless of scale

f32: t_resol --> time resolution for rendering the curves

*/

struct glf_scale_artifact {
    
};

struct glf_scale_correction_info {
    size_t nArtifacts = 0;
    glf_scale_artifact *artifacts = nullptr;
};

glf_scale_correction_info arender_msdf_correction_map(MsdfGpuContext *a_ctx, u32 curve_resolu) {
    glf_scale_correction_info res;

    if (!a_ctx) {
        std::cout << "Correction Map Render error: invalid gpu context!" << std::endl;
        return res;
    }

    

    return res;
}

struct bcurve_dist {
    f32 t = 0.0f, s = 0.0f;
    f32 square_dist = chonk_number;
};

struct cmdl_precompute {
    f32 J,K,L,M,N,O,P,Q,T,U;
};

cmdl_precompute precomp_cure_min_dist_lite(gpu_light_curve c0, gpu_light_curve c1) {
    return {};
}

/*bcurve_dist curve_min_dist_lite(const f32 px, gpu_light_curve c0, gpu_light_curve c1) {

    //TODO: optimize these by precomputing all of the constant variables since many don't involve t
    //;implement and use the cmdl_precompute struct each time the function is called
    auto Fx = [=](f32 t, vec2 A, vec2 B, vec2 C, vec2 D, vec2 E, vec2 F) {
        const f32 t2 = t*t,
                      L = A.y-2.0f*B.y+C.y,  M = A.x-2.0f*B.x+C.x, 
                      N = 2.0f*D.x-2.0f*E.x,    O = 2.0f*D.y-2.0f*E.y,
                      P = -2.0f*A.x+2.0f*B.x,   Q = -2.0f*A.y+2.0f*B.y,
                      J = -D.x+2.0f*E.x-F.x, K = -D.y+2.0f*E.y-F.y,
                      Sn = (2.0f*L*N - 2.0f*M*O)*t + (N*Q - O*P),
                      Sd = (4.0f*K*M - 4.0f*J*L)*t + (2.0f*P*K - 2.0f*J*Q),
                      S = Sn / Sd, S2 = S*S;
        return powf(M*t2+P*t+A.x+J*S2+N*S-D.x, 2.0f) + powf(L*t2+Q*t+A.y+K*S2+O*S-D.y, 2.0f);
    };

    auto dFx = [=](f32 t, vec2 A, vec2 B, vec2 C, vec2 D, vec2 E, vec2 F) {
        const f32 t2 = t*t,
                      L = A.y-2.0f*B.y+C.y,  M = A.x-2.0f*B.x+C.x, 
                      N = 2.0f*D.x-2.0f*E.x,    O = 2.0f*D.y-2.0f*E.y,
                      P = -2.0f*A.x+2.0f*B.x,   Q = -2.0f*A.y+2.0f*B.y,
                      J = -D.x+2.0f*E.x-F.x, K = -D.y+2.0f*E.y-F.y,
                      T = (2.0f*L*N - 2.0f*M*O), U = (4.0f*K*M - 4.0f*J*L),
                      Sn = T*t + (N*Q - O*P),
                      Sd = U*t + (2.0f*P*K - 2.0f*J*Q),
                      S = Sn / Sd, S2 = S*S,
                      ds = (T*Sd - Sn*U) / (Sd * Sd), ds2 = ds * ds;

        return 2.0f*(M*t2+P*t+A.x+J*S2+N*S-D.x)*(2.0f*M*t+P+2.0f*J*S*ds+N*ds)+2.0f*(L*t2+Q*t+A.y+K*S2+O*S-D.y)*(2.0f*L*t+Q+2.0f*K*S*ds+O*ds);
    };

    i32 i;
    f32 tst = 0.5, min = 0.0, max = 1.0;

    //const iel2 = 1.0 / Math.log(2.0);
    const f32 iel2 = (1.0f / logf(2.0f));
    const f32 N = ceilf(logf(px) * iel2);

    f32 sd, D;

    for (i = 0; i < N; i++) {
        sd = mu_sign(Fx(t));
        D = dFx(t);

        if (sd < 0) 
            min = t; 
        else
            max = t;

        tst = (max - min) * 0.5 + min;
    }

    bcurve_dist dist;

    dist.t = tst;
    //TODO: compute dist.s
    dist.square_dist = Fx(tst);
    
    return {};
}*/

//////////////////////////////////////////////
// version of gpu accel for multiple glyphs //
//////////////////////////////////////////////

const size_t MAX_BUFFER_CURVES = 1024;

i32 render_multi_positioned_msdf_gpu_accel(Glyph* tGlyphs, CharSpritePos* pos, FontInst *font, size_t nGlyphs, MsdfGpuContext *ctx, f32 padding_scl) {
    
    //simple error / render ability checks
    if (ctx->fb.w == 0 || ctx->fb.h == 0) {
        std::cout << "msdf gen error: output dim is 0 in one or more dimension/s" << std::endl;
        return 0;
    }

    if (!ctx) {
        std::cout << "error invalid context" << std::endl;
        return 1;
    }

    //
    const size_t nChannels = 4;

    Glyph tg;
    CharSpritePos g_pos;
    f32 gw,gh;

    size_t dat_collect;

    if (!msdf_gen_shader.good()) {
        msdf_gen_shader = Shader::LoadShaderFromFile(MSDF_ACCEL_SHADER_PATH_VERT, MSDF_ACCEL_SHADER_PATH_FRAG);
        //msdf_gen_shader.EnablePersistantUniforms();
    }

    if (!msdf_gen_cc_shader.good()) {
        msdf_gen_cc_shader = Shader::LoadShaderFromFile(MSDF_ACCEL_CC_SHADER_PATH_VERT, MSDF_ACCEL_CC_SHADER_PATH_FRAG, MSDF_ACCEL_CC_SHADER_PATH_TCS, MSDF_ACCEL_CC_SHADER_PATH_TES);
        msdf_gen_cc_shader.EnablePersistantUniforms();
    }

    if (!msdf_gen_cc_composite_shader.good())
        msdf_gen_cc_composite_shader = Shader::LoadShaderFromFile(MSDF_ACCEL_CC_COMPOSITE_SHADER_PATH_VERT, MSDF_ACCEL_CC_COMPOSITE_SHADER_PATH_FRAG);

    ctx->g.RestoreDefaultRenderState();
    ctx->g.SetOutputDevice(ctx->fb.device());

    //set shaders and tesselation params
    ctx->g.SetShader(&msdf_gen_shader);
    ctx->g.RenderBegin();

    constexpr size_t N_RECT_VERTS = 6;
    size_t nextCurveInsert = 0;

    MsdfGenContext *g_ctx_store = new MsdfGenContext[nGlyphs];
    MsdfGenContext g_ctx;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    i32 i, j;

    for (i = 0; i < nGlyphs; i++) {
        tg = tGlyphs[i];

        if (tg.char_id < 0) {
        _char_id_fail:
            if (!tg.compound && !tg.component)
                std::cout << "error invalid char id! @ i = " << i << " / " << nGlyphs << std::endl;
            else {
                #ifdef TTFRENDER_DBG_REPORT_ALL_SKIPPED_CHARS
                    std::cout << "notice skipped compound char @i = " << i << " / " << nGlyphs << std::endl;
                #endif
            }
            continue;
        }

        if (tg.arb_char_idx < 0 || tg.arb_char_idx >= font->ad_inf.ngdata) {
            std::cout << "warning: misaligned chararcter " << i << " | Char Id: " << tg.char_id << std::endl;
            continue;
        }

        g_pos = pos[i];

        //add spritesheet character info for compound glyphs
        Character *ochar = font->gdata+tg.arb_char_idx;

        if (ochar->nParts > 0) {
            std::cout << "ttf_render warning: strange number of character parts for glyph " << i << " / " << nGlyphs << " | Char ID: " << ochar->val << " " << (char)(ochar->val & 0xff) << std::endl;
        }

        //
        if (g_pos.w == 0 || g_pos.h == 0) {
            std::cout << "ttf_render warning: character " << i << " is very small" << std::endl;
            continue; //dimension check (dont add null glyphs)
        }

        gw = tg.xMax - tg.xMin;
        gh = tg.yMax - tg.yMin;

        //generate the glpyh context
        const size_t rc_wposb4 = ((ctx->rc_ctx) ? ctx->rc_ctx->wOff : 0);
        ConfigureGenContext(g_ctx_store+i, tg, ctx->rc_ctx, true);

        if (ochar && ctx->rc_ctx && ctx->rc_ctx->wOff > 0) {
            ochar->rc_Dat.rc_curve_start = rc_wposb4;
            ochar->rc_Dat.rc_curve_end = ctx->rc_ctx->wOff - 1;
        } else {
            std::cout << "ttf_render warning: could not add ray count info for glyph index " << i << " of " << nGlyphs 
                      << "\n\t src: " << ctx->trace.font_src 
                      << "\n\t check: ochar?" << (ochar != nullptr) 
                      << " / rcContext?" << (ctx->rc_ctx != nullptr) 
                      << " / wOff=" << (ctx->rc_ctx->wOff) << " (should be > 0) " << std::endl;

            if (ochar) {
                ochar->rc_Dat.rc_curve_start = 0;
                ochar->rc_Dat.rc_curve_end = 0;
            }
        }

        g_ctx = g_ctx_store[i];

        if (nextCurveInsert + g_ctx.nCurves >= MAX_BUFFER_CURVES) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ctx->curveBuffer);
            ctx->g.RenderFlush(false);
            ctx->g.RenderBegin();

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
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ctx->curveBuffer);
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

        //region_vec = vec4(0.0f, 0.0f, 1.0f, 1.0f);

        //render
        if (g_pos.rotate_90) {
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

            ctx->g.PushVerts(out_rect, NV, true);
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

            ctx->g.PushVerts(out_rect, NV, true);
        }

        //DeleteMsdfGenContext(&g_ctx);

        //swap and render correction stuff
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ctx->curveBuffer);

    ctx->g.RenderFlush();

    const u32 o_fb = ctx->fb.getTextureHandle();

    //bind the default texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, o_fb);

    ///////////
    ctx->g.SetRenderState(ctx->cc_rstate);
    ctx->g.SetTesselationVertNum(3);
    ctx->g.SetShader(&msdf_gen_cc_shader);
    ctx->g.SetOutputDevice(ctx->cc_fb.device());

    constexpr i32 bcc_detail = 32; //num segments for each curve being rendered

    vec2 oDim = vec2(ctx->cc_fb.w, ctx->cc_fb.h);

    msdf_gen_cc_shader.SetInt("curve_detail", bcc_detail);
    msdf_gen_cc_shader.SetVec2("output_dim", &oDim);

    i32 old_depthFn;
    glGetIntegerv(GL_DEPTH_FUNC, &old_depthFn);

    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    ctx->g.RenderBegin(ctx->cc_fb.w, ctx->cc_fb.h);

    for (i = 0; i < nGlyphs; i++) {
        tg = tGlyphs[i];

        if (tg.char_id < 0 || tg.xMax == tg.xMin || tg.yMax == tg.yMin || tg.arb_char_idx < 0) {
            continue;
        }

        g_ctx = g_ctx_store[i];
        g_pos = pos[i];

        if (g_pos.w == 0 || g_pos.h == 0) {
            std::cout << "ttf error: poor glyph sheet construction skipping cur char" << std::endl;
            continue;
        }

        gpu_light_curve *gcuBuf = (gpu_light_curve*)g_ctx.curves;
        gpu_light_curve cgcu;

        const f32 space_cc_normal_x = 1.0f / (ctx->cc_fb.w),
                  space_cc_normal_y = 1.0f / (ctx->cc_fb.h);

        //region_vec = vec4((g_pos.x * space_cc_normal_x) * 2.0f - 1.0f, (g_pos.y * space_cc_normal_y) * 2.0f - 1.0f, (g_pos.w * space_cc_normal_x) * 2.0f, (g_pos.h * space_cc_normal_y) * 2.0f);

        const f32 gfw = tg.xMax - tg.xMin, gfh = tg.yMax - tg.yMin;

        const f32 pxt_A =  (1.0f / (gfw + padding_scl * 2.0f * gfw)) * g_pos.w,
                  pxt_B = space_cc_normal_x * 2.0f,
                  pyt_A = (1.0f / (gfh + padding_scl * 2.0f * gfh)) * g_pos.h,
                  pyt_B = space_cc_normal_y * 2.0f;

        const f32 dpx = padding_scl * gfw, dpy = gfh * padding_scl;

        for (j = 0; j < g_ctx.nCurves; j++) {
            cgcu = gcuBuf[j];

            constexpr f32 CURVE_Z_CORRECT_PREC = 0.0001f;
            constexpr f32 CURVE_Z_CORRECT_BASE = 0.5f;
            const f32 cz = j * CURVE_Z_CORRECT_PREC + CURVE_Z_CORRECT_BASE;

            con_correct_vert curve_verts[] = {
                ((cgcu.p0[0] + dpx) * pxt_A + g_pos.x) , ((cgcu.p0[1] + dpy) * pyt_A + g_pos.y) , cz, j,
                ((cgcu.p1[0] + dpx) * pxt_A + g_pos.x) , ((cgcu.p1[1] + dpy) * pyt_A + g_pos.y) , cz, j,
                ((cgcu.p2[0] + dpx) * pxt_A + g_pos.x) , ((cgcu.p2[1] + dpy) * pyt_A + g_pos.y) , cz, j,
            };

            ctx->g.PushVerts(curve_verts, 3, true);
        }

        DeleteMsdfGenContext(&g_ctx);
    }

    ctx->g.RenderFlush();

    const u32 cc_fb_tHand = ctx->cc_fb.getTextureHandle();

    //set the ref texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cc_fb_tHand);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //restore depth functio and other stuff
    glDepthFunc(old_depthFn);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    //composite render
    ctx->g.SetRenderState(ctx->cc_composite_rstate);
    ctx->g.SetOutputDevice(ctx->cc_composite_fb.device());
    ctx->g.RenderBegin();

    const con_correct_composite_vert cc_composite_verts[] = RECT_VERTS_EX(
        -1.0f, -1.0f, 2.0f, 2.0f, 
        0.0f COMMA 0.0f COMMA 0.0f, 0.0f COMMA 1.0f COMMA 0.0f, 0.0f COMMA 1.0f COMMA 1.0f, 0.0f COMMA 0.0f COMMA 1.0f
    );

    ctx->g.RenderBegin();
    ctx->g.SetShader(&msdf_gen_cc_composite_shader);

    msdf_gen_cc_composite_shader.SetInt("base_msdf", 0);
    msdf_gen_cc_composite_shader.SetInt("correction_map", 1);

    ctx->g.PushVerts((void *) cc_composite_verts, 6, true);
    ctx->g.RenderFlush();


    /*
    TODODODODO

    adjust the correction sheet to account for aditional transformations done on the glyphs such as inter and intra padding

    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    */

    const u32 cc_composite_fb_tHand = ctx->cc_composite_fb.getTextureHandle();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, cc_composite_fb_tHand);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //rc data
    if (ctx && ctx->rc_ctx) {
        font->rc_dat.lcs = ctx->rc_ctx->curveBuf;
        font->rc_dat.nCurves = ctx->rc_ctx->nCurves;
        _safe_free_b(ctx->rc_ctx);
    } else {
        std::cout << "ttf render (severe) warning: could not include rc data!" << std::endl;
        font->rc_dat.lcs = nullptr;
        font->rc_dat.nCurves = 0;
    }

    //for debug stuff
    font->msdf_dat.MSDF.__dbg.cc_tex = BindableTexture(cc_composite_fb_tHand, ctx->cc_composite_fb.w, ctx->cc_composite_fb.h);

    _safe_free_a(g_ctx_store);
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
    rcGenContext *__disc_ctx = nullptr;
    glfEdgeObject glyphEdges = generateGlyphEdges(tGlyph, cleanDat, nPoints, __disc_ctx);

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
    //constants
    constexpr f32 padding_per_32 = 1.0f;
    constexpr f32 inter_glyph_padding = 1.0f;
    constexpr f32 padding = 0.1f;
    
    //param checks and shit
    FontInst font = {
        .range = range
    };

    if (src.length() == 0 || (u32) range >= (u32) UnicodeRange::Unknown)
        return font;

    // get all the glyphs
    GlyphSet glyphs = ttfParse::GenerateGlyphSet(src, range);
    ttfFile f = glyphs.file;

    if (glyphs.nGlyphs == 0) {
        ttfParse::DeleteGlyphSet(glyphs);
        return font;
    }

    if (glyphs.minGlyphId > 0xffff) {
        std::cout << "Cannot generate font instance for: " << src << " | bad min glyph" << std::endl;
        ttfParse::DeleteGlyphSet(glyphs);
        return font;
    }

    //set some info needed later for spacing calculations
    font.ad_inf.unitsPerEm = f.header.unitsPerEm;
    font.ad_inf.ascent = f.header.yMax;
    font.ad_inf.descent = f.header.yMin;

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

    //sort by size (ascending)
    Glyph *gly = new Glyph[glyphs.nGlyphs];
    in_memcpy(gly, glyphs.glyphs, sizeof(Glyph) * glyphs.nGlyphs);
    mu_qsort<Glyph>(gly, &_glyphCmp, glyphs.nGlyphs);

    /////////////////
    font.ad_inf.ngdata = glyphs.nGlyphs;
    font.gdata = new Character[font.ad_inf.ngdata];

    //translation maps
    //mpa8/char
    font.c_translate.mpa8 = new u32[256];
    if (!font.c_translate.mpa8) {
        std::cout << "error could not create proper character mappings (mpa8: bad alloc)" << std::endl;
        font.good = false;
        return font;
    }
    ZeroMem(font.c_translate.mpa8, 256);   

    //mpa16/wchar
    if (glyphs.wchar_supported) {
        font.c_translate.mpa16 = new u32[65536];

        if (!font.c_translate.mpa16) {
            std::cout << "error could not create proper character mappings (mpa16: bad alloc)" << std::endl;
            font.good = false;
            return font;
        }

        ZeroMem(font.c_translate.mpa16, 65536);
    }

    /*
    
    Compute the position in the sprite sheet of all characters
    maybe make this a seperate function that's inline

    */
    struct SpriteRegion {
        i32 x = 0, y = 0, w = -1, h = -1, age = 0; //-1 for w and height is treated as infinity
    };

    SpriteRegion Rn;
    i32 i = 0, j = -1;
    u32 sheet_w = 1, sheet_h = 1;

    const f32 ig_pad_2 = inter_glyph_padding * 2.0f;

    //create the sprite sheet position mapping things
    CharSpritePos *c_pos = new CharSpritePos[glyphs.nGlyphs];
    std::vector<SpriteRegion> rgn_stack = {{
        .x = 0, .y = 0,
        .w = -1, .h = -1
    }}; //add the first "infinite" region

    //genereate the sprite sheet layout
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

        //best things
        i32 best_rgn = 0;
        u32 smallest_fit = (unsigned) (-1), fit, low_age = (unsigned) (-1);
        bool fit_rotated = false;

        const size_t nRegions = rgn_stack.size();

        //compute msdf dim
        i32 gw = ((g.xMax - g.xMin) * (1.0f + padding * 2.0f)) * scale, gh = ((g.yMax - g.yMin ) * (1.0f + padding * 2.0f)) * scale;

        for (j = 0; j < nRegions; j++) {
            Rn = rgn_stack[j];

            //make sure it fits
            if ((Rn.w < gw + ig_pad_2 && Rn.w > 0) || (Rn.h < gh + ig_pad_2 && Rn.h > 0)) continue;

            const u32 fit = mu_max(Rn.x + gw + ig_pad_2, sheet_w) * mu_max(Rn.y + gh + ig_pad_2, sheet_h);

            if (fit < smallest_fit) {
                low_age = Rn.age;
                best_rgn = j;
                smallest_fit = fit;
                fit_rotated = false;
            } else if (fit == smallest_fit && Rn.age < low_age) {
                low_age = Rn.age;
                best_rgn = j;
                fit_rotated = false;
            }

            //check potential 90 deg rotating benefits
            //if ((Rn.w < gh && Rn.w > 0) || (Rn.h < gw && Rn.h > 0)) continue;
            //const u32 fit90 = mu_max(Rn.x + gh, sheet_w) * mu_max(Rn.y + gw, sheet_h);

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

        c_pos[i] = {
            .x = (u32) (target_rgn.x + inter_glyph_padding),
            .y = (u32) (target_rgn.y + inter_glyph_padding),
            .w = (u32) (fit_rotated ? gh : gw),
            .h = (u32) (fit_rotated ? gw : gh),
            .rotate_90 = fit_rotated
        };

        //sizing adjust
        sheet_w = mu_max(sheet_w, target_rgn.x + gw + ig_pad_2);
        sheet_h = mu_max(sheet_h, target_rgn.y + gh + ig_pad_2);

        //Split the target region up accordingly and then continue yk
        //just gonna split on the vertical cause why not

        rgn_stack.erase(rgn_stack.begin() + best_rgn); //remove old region

        //top rgn
        SpriteRegion r1 = {
            .x = target_rgn.x + gw + (i32) ig_pad_2,
            .y = target_rgn.y,
            .w = -1,
            .h = gh,
            .age = target_rgn.age + 1
        };

        rgn_stack.push_back(r1);

        //bottom rgn
        const i32 h2 = (target_rgn.h >= 0) * (target_rgn.h - gh) + (target_rgn.h < 0) * -1;

        if (abs(h2) > 0) {
            SpriteRegion r2 = {
                .x = target_rgn.x,
                .y = target_rgn.y + gh + (i32) ig_pad_2,
                .w = -1,
                .h = h2,
                .age = target_rgn.age + 1
            };
            rgn_stack.push_back(r2); //add 2 new regions
        }
        
        i++;
    }

    //note / OPTIMIZATION: can combine with above function but for cleanliness puposes these are separated
    //the only caviate is that the first if-statement in the while loop will cause issue since the block
    //of code the proceeds this comment but execute for all glyphs and not select glyphs
    //add character info
    for (i = 0; i < font.ad_inf.ngdata; i++) {
        Glyph g = gly[i];

        Character ochar;

        ochar.dim.w = g.xMax - g.xMin;
        ochar.dim.h = g.yMax - g.yMin;
        ochar.dim.ranges.xMin = g.xMin; ochar.dim.ranges.xMax = g.xMax;
        ochar.dim.ranges.yMin = g.yMin; ochar.dim.ranges.yMax = g.yMax;
        ochar.dim.hw_ratio = ((f32) ochar.dim.h) / ((f32) ochar.dim.w);
        ochar.hmetrics = g.h_inf;
        ochar.val = g.char_id;
        
        //ochar.nParts = 
        ochar.nParts = g.compound ? g.compound_inf.nGlyphParts : 0;

        if (ochar.nParts > 0) {
            ochar.spriteParts = new GlyphPart[ochar.nParts];
            ZeroMem(ochar.spriteParts, ochar.nParts);
            in_memcpy(ochar.spriteParts, g.compound_inf.glyph_parts, ochar.nParts * sizeof(GlyphPart));
        } else {
            //nParts = 0 indicates to just read from sprite_dat
            ochar.nParts = 0;
            ochar.sprite_dat.msdf_support = true;
            ochar.sprite_dat.sheet_loc = c_pos[i];
        }

        constexpr size_t gDataReallocXtra = 0XF;
        const u32 gInsert = g.glyph_id /*- glyphs.minGlyphId*/;

        if (gInsert >= font.ad_inf.ngdata) {
            const size_t gCopySz = font.ad_inf.ngdata;
            font.ad_inf.ngdata = gInsert + 1 + gDataReallocXtra;
            Character *gd = new Character[font.ad_inf.ngdata];
            in_memcpy(gd, font.gdata, sizeof(Character) * gCopySz);
            _safe_free_a(font.gdata);
            font.gdata = gd;
        }

        memcpy(font.gdata+gInsert, &ochar, sizeof(Character));
        (*(gly+i)).arb_char_idx = gInsert;

        //populate the translation maps
        if (g.char_id < 256)        font.c_translate.mpa8[g.char_id]  = gInsert + 1;
        if (glyphs.wchar_supported) font.c_translate.mpa16[g.char_id] = gInsert + 1;
    }

    //generate the spritesheet

    CharSpritePos r_pos;

    MsdfGpuContext *a_ctx = nullptr;

    if (accel) {
        a_ctx = CreateMsdfGPUAccelerationContext(sheet_w, sheet_h);
        a_ctx->trace.font_src = src;
        font.msdf_dat.dim.w = a_ctx->fb.w;
        font.msdf_dat.dim.h = a_ctx->fb.h;
        render_multi_positioned_msdf_gpu_accel(gly, c_pos, &font, glyphs.nGlyphs, a_ctx, padding);
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
            Glyph glf = gly[i];

            r_pos = c_pos[i];

            //add spritesheet location for compound glyph parts since they depend on already being computed

            if (r_pos.w <= 0 || r_pos.h <= 0)
                continue;

            //use gpu acceleration if needed
            //TODO: add memory management for the frame buffer and delete the buffer
            render_positioned_msdf(
                glf, 
                sheet, 
                r_pos.x, r_pos.y, r_pos.w, r_pos.h, 
                padding, padding, padding, padding
            );
        }
    }

    if (accel) {
        font.msdf_dat.mode = MsdfMode::GL_Texture;
        font.msdf_dat.MSDF.gl_texture = BindableTexture(a_ctx->cc_composite_fb.getTextureHandle(), a_ctx->cc_composite_fb.w, a_ctx->cc_composite_fb.h);
        font.msdf_dat.MSDF.gl_texture.bind();
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    //font._dbg_ctx = a_ctx;

    //memory management
    _safe_free_a(gly);

    //add font info stuff
    font.ad_inf.mxv = f.header.max_vals;
    font.ad_inf.minChar = glyphs.minChar;
    font.h_inf.inf = f.h_inf;
    font.h_inf.metrics = new h_char_metric[f.n_metrics];
    in_memcpy(font.h_inf.metrics, f.h_metrics, f.n_metrics * sizeof(h_char_metric));
    font.ad_inf.monospace = (f.h_inf.nLongHorMetrics == 1);
    font.ad_inf.wchar_support = glyphs.wchar_supported;
    font.ad_inf.null_char_loc = glyphs.nullCharLoc;

    //find curve buddies real quick
    find_font_curve_friends(&font);

    font.good = true;

    ttfParse::DeleteGlyphSet(glyphs);

    return font;
}

void ttfRender::_msdfRenderDebug(Glyph g, MsdfGpuContext** ctx) {
    CharSpritePos r_pos;

    if (!*ctx) {
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

    /*if (font->map.hash_map) {
        _safe_free_a(font->map.hash_map);
        font->map.hash_map = nullptr;
        font->map.hash_inf.sz = 0;
        font->map.ty = (CharMapType) 0;
    }*/

    if (font->gdata) {
        _safe_free_a(font->gdata);
        font->ad_inf.ngdata = 0;
    }

    if (font->c_translate.mpa8) {
        _safe_free_a(font->c_translate.mpa8);
    }

    if (font->c_translate.mpa16) {
        _safe_free_a(font->c_translate.mpa16);
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

struct str_pre_metrics {
    i32 maxW, maxH;
    size_t str_len;
    i32 line_h;
    f32 stride_factor; //amount to multiply each stride by in order to get proper pt scale

    //basically just multiply a glyph's width (xMax - xMin) by this value to get its output screen pixel width
    //then compute the output height of the glyph using the ratio of the glyphs height to width and the compute pixel width above
    f32 pRatio;

    f32 tab_distance = 0;
    f32 space_distance = 0;
    f32 tab_size = 4;
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
    //metrics.WemRatio = px_w / ((f32) font->ad_inf.unitsPerEm); //width based em ratio

    metrics.pRatio = (standardDPI * prop.scale.pt) / (72.0f * font->ad_inf.unitsPerEm);

    //compute font line height
    metrics.line_h = abs(font->ad_inf.ascent - font->ad_inf.descent);
    metrics.space_distance = font->ad_inf.unitsPerEm * metrics.pRatio * 0.33f;
    metrics.tab_distance = metrics.space_distance * metrics.tab_size;

    //
    return metrics;
}

struct StrRenderContext {
    char *cur_char = nullptr;
    f32 x = 0, left_edge = 0, baseline_y = 0;
};

struct genericFontVert {
    f32 pos[3];
    f32 tex[2];
};

struct rcFontVert {
    f32 pos[3];
    f32 rgn[2];
    i32 curve_range[2];
    f32 delta[2];
};

static RenderState *defFontRenderState = nullptr, *rcFontRenderState = nullptr;
static bool defFontRenderStateCreated = false, defFontVertexDef = false, 
            rcFontRenderStateCreated = false, rcFontVertexDef = false;
static Shader defFontShader, smplRayCountShader;
static mat4 str_proj_mat;
constexpr size_t nFontRenderVerts = 2048;

static RenderStateDescriptor fontRenderDesc = {
    .dynamic = true,
    .use_indicies = false,
    .max_batch_verts = nFontRenderVerts
};

static RenderStateDescriptor rcRenderDesc = {
    .dynamic = true,
    .use_indicies = false,
    .max_batch_verts = nFontRenderVerts
};

void graphics::ini_generic_font_state() {
    defFontRenderState = CreateNewRenderState(fontRenderDesc);

    defFontRenderStateCreated = true;
}

void graphics::ini_rc_font_state() {
    rcFontRenderState = CreateNewRenderState(rcRenderDesc);

    rcFontRenderStateCreated = true;
}

//todo: set these paths
#define DEF_FONT_SHADER_VERT_SRC "../../src/basic_font_vert.glsl"
#define DEF_FONT_SHADER_FRAG_SRC "../../src/basic_font_frag.glsl"

#define SIMPLE_RC_SHADER_VERT_SRC "../../src/font_render_ray_count_vert.glsl"
#define SIMPLE_RC_SHADER_FRAG_SRC "../../src/font_render_ray_count_adv_frag.glsl"

/*
MAJOR TODO:

ok so possible remove the uniforms to the shader and add functionality to be able to render multiple characters in one batch

*/

//TODO: coordinate this process with graphics states
void graphics::RenderString(FontInst *font, f32 x, f32 y, f32 z, const char* str, GenericFontProperties prop, bool yRelTop) {
    if (!font || !str)
        return;

    if (!font->c_translate.mpa8) {
        std::cout << "cannot render string: not mpa8 map!" << std::endl;
        return;
    }

    if (!defFontRenderStateCreated)
        ini_generic_font_state();

    if (!rcFontRenderState)
        ini_rc_font_state();

    //determine rendering method
    const bool use_msdf = !(prop.scale.pt < font->ad_inf.render.maxFontSizeForRayCount && font->ad_inf.render.useRayCountAtSmallScales);

    //check font instance and hash map stuff

    //compute the stirng metrics first
    constexpr u8 met_flg = 0b10000000;
    str_pre_metrics metrics = computePreStringMetrics(font, x, y, z, str, prop, met_flg);

    //render setup
    StrRenderContext s_ctx = {
        .cur_char = (char*) str,
        .x = x,
        .left_edge = x,
        .baseline_y = y /*+ font->ad_inf.ascent*/ //add max ascent to get proper positioning for the text basline
    };

    const u32 screenW = this->getOutputWidth();
    const u32 screenH = this->getOutputHeight();

    if (!defFontVertexDef && !rcFontVertexDef) {
        str_proj_mat = mat4::CreateOrthoProjectionMatrix(screenW, 0.0f, screenH, 0.0f, -1.0f, 1.0f);
    }

    if (!defFontShader.good()) {
        defFontShader = Shader::LoadShaderFromFile(DEF_FONT_SHADER_VERT_SRC, DEF_FONT_SHADER_FRAG_SRC);
    }

    if (!smplRayCountShader.good()) {
        smplRayCountShader = Shader::LoadShaderFromFile(SIMPLE_RC_SHADER_VERT_SRC, SIMPLE_RC_SHADER_FRAG_SRC);
    }

    //enable alpha blending and what not
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glDepthFunc(GL_GEQUAL);
    glDisable(GL_CULL_FACE);

    if (use_msdf) {
        this->SetRenderState(defFontRenderState);

        if (!defFontVertexDef) {
            this->VertexDefineBegin(sizeof(genericFontVert));
            this->DefineVertexPart(0, vertexClassPart(genericFontVert, pos));
            this->DefineVertexPart(1, vertexClassPart(genericFontVert, tex));
            this->VertexDefineEnd();
            defFontVertexDef = true;
        }

        this->SetShader(&defFontShader);
        defFontShader.SetMat4("screen_project", &str_proj_mat);
        font->msdf_dat.MSDF.gl_texture.bind();

        
    } else {
        this->SetRenderState(rcFontRenderState);

        if (!rcFontVertexDef) {
            this->VertexDefineBegin(sizeof(rcFontVert));
            this->DefineVertexPart(        0, vertexClassPart(rcFontVert, pos));
            this->DefineVertexPart(        1, vertexClassPart(rcFontVert, rgn));
            this->DefineIntegerVertexPart( 2, vertexClassPart(rcFontVert, curve_range));
            this->DefineVertexPart(        3, vertexClassPart(rcFontVert, delta));
            this->VertexDefineEnd();
            rcFontVertexDef = true;
        }

        this->SetShader(&smplRayCountShader);
        smplRayCountShader.SetMat4("screen_project", &str_proj_mat);

        //set font color
        constexpr f32 i255 = 1.0f / 255.0f;
        const vec3 v_color = vec3(prop.style.color.red() * i255, prop.style.color.green() * i255, prop.style.color.blue() * i255);
        smplRayCountShader.SetVec3("font_color", const_cast<vec3*>(&v_color));

        //bind font curve buffer
        if (!font->rc_dat.cu_buf_good) {
            glGenBuffers(1, &font->rc_dat.cu_buf);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, font->rc_dat.cu_buf);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(gpu_rc_curve) * font->rc_dat.nCurves, font->rc_dat.lcs, GL_STATIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, font->rc_dat.cu_buf);
            font->rc_dat.cu_buf_good = true;
        }
    }

    char cc;
    i32 p,i;

    const p_mat_2d def_pos_mat = {
        1.0f, 0.0f, /********************   E     F     M     N*/
        0.0f, 1.0f, /********************/ 0.0f, 0.0f, 1.0f, 1.0f
    };

    //begin render
    this->Resize(screenW, screenH);
    this->RenderBegin();

    while ((cc = *s_ctx.cur_char) != 0x00) {
        switch (cc) {
        //New Line
        case 0x0A:
            s_ctx.baseline_y += metrics.line_h * metrics.pRatio;
            s_ctx.x = s_ctx.left_edge;
            break;
        //Tab
        case 0x09:
            s_ctx.x += metrics.tab_distance;
            break;
        //carriage return (basically home button)
        case 0x0D:
            s_ctx.x = s_ctx.left_edge;
            break;
        case 0x20:
            s_ctx.x += metrics.space_distance;
            break;
        default:

        //check for blank characters rq
        if (cc == 0x00) {
            break;
        }
        //

        //find the character info
        Character o_char; //TODO: set this to the missing char for easy escape if given char does not exist

        //render glyph parts
        f32 part_w, part_h, part_y, part_x, HemRatio, cx_max = s_ctx.x;

        /********************************
            Transforms' Format:
            
            Matrix (woah iso metric looking matrix cool!):

             ⌈ A, C ⌉
            ⌊ B, D ⌋
        
            A = a/m, B = b/n, C = c/m, D = d/n

            Extra:

            .xy --> xy-translation [e,f]
            .zw --> xy-scale [m,n]
        *********************************/
        f32 gp_transform_mat2[] = {1.0f, 0.0f, 0.0f, 1.0f};
        vec4 gp_transform_ext = vec4(0.0f, 0.0f, 1.0f, 1.0f);

        f32 xxtra = 0.0f;

        auto render_part = [&](Character cp, p_mat_2d pos_mat, auto&& render_part) -> void {
            if (cp.nParts > 0) {
                u32 idx;

                for (i = 0; i < cp.nParts; i++) {
                    idx = cp.spriteParts[i].idx;
                    if (idx >= font->ad_inf.ngdata)
                        continue;
                    render_part(font->gdata[idx], cp.spriteParts[i].pos_mat, render_part);
                }

                return;
            }

            //compute needed render constants
            part_w = (cp.dim.ranges.xMax - cp.dim.ranges.xMin) * metrics.pRatio;
            part_h = (cp.dim.ranges.yMax - cp.dim.ranges.yMin) * metrics.pRatio;

            HemRatio = part_h / (cp.dim.ranges.yMax - cp.dim.ranges.yMin); 
            //part_y = s_ctx.baseline_y - (cp.size.yMax) * HemRatio;

            //set the transforms
            const f32 im = 1.0f / pos_mat.m, in = 1.0f / pos_mat.n;

            gp_transform_mat2[0] = pos_mat.a * im; //matrix
            gp_transform_mat2[1] = pos_mat.b * in;
            gp_transform_mat2[2] = pos_mat.c * im;
            gp_transform_mat2[3] = pos_mat.d * in;

            part_y = s_ctx.baseline_y - ((cp.dim.ranges.yMin - ((i32) yRelTop * metrics.line_h)) * metrics.pRatio) - part_h;

            const f32 dbg_iw = 1.0f / screenW,
                      dbg_ih = 1.0f / screenH;
            
            if (font->h_inf.metrics) {
                h_char_metric hm;

                hm = (font->ad_inf.monospace ? 
                    font->h_inf.metrics[0] :
                    font->h_inf.metrics[font->c_translate.mpa8[cc] - 1]
                );

                /*hm = (font->ad_inf.monospace ? 
                    font->h_inf.metrics[0] :
                    cp.hmetrics
                );*/

                part_x = s_ctx.x;
                //cx_max = part_x + hm.advance_w * metrics.pRatio;
                cx_max = part_x + part_w;
            } else {
                cx_max = part_x + part_w;
            }

            if (use_msdf) {
                const f32 iTexX = 1.0f / font->msdf_dat.MSDF.gl_texture.width(),
                        iTexY = 1.0f / font->msdf_dat.MSDF.gl_texture.height();

                //verticies
                genericFontVert glyph_rect_base[6] = { 
                    part_x, part_y, z, 
                    (f32) cp.sprite_dat.sheet_loc.x * iTexX, (f32)(cp.sprite_dat.sheet_loc.y+cp.sprite_dat.sheet_loc.h) * iTexY, 

                    (part_x), (part_y+part_h), z, 
                    (f32) cp.sprite_dat.sheet_loc.x * iTexX, (f32) (cp.sprite_dat.sheet_loc.y)* iTexY, 

                    (part_x+part_w), (part_y), z, 
                    (f32) (cp.sprite_dat.sheet_loc.x+cp.sprite_dat.sheet_loc.w)* iTexX, (f32) (cp.sprite_dat.sheet_loc.y+cp.sprite_dat.sheet_loc.h)* iTexY, 
            
                    (part_x+part_w), (part_y+part_h), z, 
                    (f32) (cp.sprite_dat.sheet_loc.x+cp.sprite_dat.sheet_loc.w)* iTexX, (f32) (cp.sprite_dat.sheet_loc.y)* iTexY, 
            
                    (part_x+part_w), (part_y), z, 
                    (f32) (cp.sprite_dat.sheet_loc.x+cp.sprite_dat.sheet_loc.w)* iTexX, (f32) (cp.sprite_dat.sheet_loc.y+cp.sprite_dat.sheet_loc.h)* iTexY,  
            
                    (part_x), (part_y+part_h), z, 
                    (f32) cp.sprite_dat.sheet_loc.x* iTexX, (f32) (cp.sprite_dat.sheet_loc.y)* iTexY, 
                };

                //TODO: actually render ts
                this->PushVerts(glyph_rect_base, 6, true);
            } else {
                const f32 cw = cp.dim.ranges.xMax - cp.dim.ranges.xMin,
                          ch = cp.dim.ranges.yMax - cp.dim.ranges.yMin;

                const f32 sdx = (cw) / part_w,
                          sdy = (ch) / part_h;

                constexpr f32 rc_padd = 0.2f;

                const f32 px = part_x, py = part_y, pw = part_w + 2.0f, ph = part_h + 2.0f;

                rcFontVert glyph_rc_verts[6] = {
                    px, py, z, 
                    (f32) cp.dim.ranges.xMin - sdx, (f32) cp.dim.ranges.yMax + sdy, 
                    (i32) cp.rc_Dat.rc_curve_start, (i32) cp.rc_Dat.rc_curve_end,
                    sdx, sdy,

                    (px), (py+ph), z, 
                    (f32) cp.dim.ranges.xMin - sdx, (f32) cp.dim.ranges.yMin - sdy, 
                    (i32) cp.rc_Dat.rc_curve_start, (i32) cp.rc_Dat.rc_curve_end,
                    sdx, sdy,

                    (px+pw), (py), z, 
                    (f32) cp.dim.ranges.xMax + sdx, (f32) cp.dim.ranges.yMax + sdy, 
                    (i32) cp.rc_Dat.rc_curve_start, (i32) cp.rc_Dat.rc_curve_end,
                    sdx, sdy,
            
                    (px+pw), (py+ph), z, 
                    (f32) cp.dim.ranges.xMax + sdx, (f32) cp.dim.ranges.yMin - sdy, 
                    (i32) cp.rc_Dat.rc_curve_start, (i32) cp.rc_Dat.rc_curve_end,
                    sdx, sdy,
            
                    (px+pw), (py), z, 
                    (f32) cp.dim.ranges.xMax + sdx, (f32) cp.dim.ranges.yMax + sdy,  
                    (i32) cp.rc_Dat.rc_curve_start, (i32) cp.rc_Dat.rc_curve_end,
                    sdx, sdy,
            
                    (px), (py+ph), z, 
                    (f32) cp.dim.ranges.xMin - sdx, (f32) cp.dim.ranges.yMin - sdy, 
                    (i32) cp.rc_Dat.rc_curve_start, (i32) cp.rc_Dat.rc_curve_end,
                    sdx, sdy
                };

                this->PushVerts(glyph_rc_verts, 6, true);
            }
        };

        //
        u32 cIdx = font->c_translate.mpa8[cc];

        if (cIdx == 0) {
            cIdx = font->ad_inf.null_char_loc;
        } else {
            cIdx--;
        }

        if (cIdx < font->ad_inf.ngdata) render_part(font->gdata[cIdx], def_pos_mat, render_part);

        xxtra = mu_max(1, prop.scale.pt * 0.1);
        s_ctx.x = cx_max + xxtra;

        break; //end of switch statement default branch
        }

        //advance to next character
        s_ctx.cur_char++;
    }

    this->RenderFlush();
}

#include "json.hpp"

FontInst ttfRender::GenerateFontFromForeign(std::string img_src, std::string json_layout_src) {
    FontInst fnt;

    if (img_src.length() == 0 || json_layout_src.length() == 0) {
        std::cout << "Failed to generate font from foreign data: invalid srcs!" << std::endl;
        return fnt;
    }

    png_image msdf_img = PngParse::Decode(ContentSrc::FromFile(img_src));

    if (!msdf_img.data || msdf_img.sz == 0) {
        std::cout << "Failed to generate font from foreign data: invalid, blank, or corrupt image!" << std::endl;
        return fnt;
    }

    //TODO: store the image in the font's msdf

    //parse the locations / json
    text_file jf = FileWrite::readFromText(json_layout_src);

    if (!jf.dat || jf.len == 0) {
        std::cout << "Failed to generate font from foreign data: invalid json map!" << std::endl;
        return fnt;
    }

    JStruct fmap = jparse::parseStr(jf.dat, jf.len);

    //TODO: parse the map

    //memory stuff
    _safe_free_a(msdf_img.data);
    _safe_free_a(jf.dat);
    jf.len = 0;

    return fnt;
}

//


//render a character via the ray count thing
/*void _render_char_rc_256(Character o_char, f32 x, f32 y, f32 z, f32 w, f32 h, FontInst *font) {
    //easy peasy lemon squeezy (not really cause of all the precomputations)
    if (!font || !font->rc_dat.lcs)
        return;

    
}*/

void ttfRender::DeleteFontObject(FontInst *&font) {
    if (!font) return;

    //TODO: delete all the fonts parts
    DeleteFontInst(font);

    _safe_free_b(font);
    font = nullptr;
}

////////////////////////

inline f32 ap_dot(f32 p0[2], f32 p1[2]) {
    return (p0[0]*p1[0]+p0[1]*p1[1]);
}

inline f32 (&ap_sub(f32 p0[2], f32 p1[2]))[2] {
    f32 r[2] = {p0[0]-p1[0],p0[1]-p1[1]};
    return r;
}

inline f32 ap_mag(f32 p[2]) {
    return  sqrtf(p[0]*p[0]+p[1]*p[1]);
}

//the whole fancy float encoding, base=-4
//note: look at the google doc for explanation on works of the system
//range for width is [0.0625, ~5.764e17]
constexpr f32 weba  = 9.403954806578300063749892e-38f;
constexpr f32 iweba = 1.063382396627932698323046e+37f;

inline f32 wEncodeF32(f32 w, f32 xSgn, f32 ySgn) {
    return (w * weba) * (((-ySgn + 1.0f) * 0.5f) * 1.8446744073709551616e19f) * xSgn;
}

inline f32 wDecodeF32(f32 v) {
    v = abs(v);
    return ((v) >= 1.084202172485504434007453e-19f) ? (((v) * 5.421010862427522170037264e-20f) * iweba) : ((v) * iweba);
}

//direction based distance solve
//dist < 0 --> left, dist > 0 --> right, dist == 0 (:Skull:)
f32 simple_curve_point_dir_dist(f32 p[2], gpu_rc_curve cu, f32 *solve_inf) {
    auto p0 = cu.p0, p1 = cu.p1, p2 = cu.p2;

    //when solving the min dist / roots --> optimize to use solve_re_cubic_32_b or solve_re_cubic_64_b

    f32 root_pass[3] = {-1.0f, -1.0f, -1.0f};

    const i32 nRoots = solve_re_cubic_32_a(
        solve_inf[0], 
        solve_inf[1],
        solve_inf[2]
            - 4.0f * (p0[1]*p[1] + p0[0]*p[0])
            + 8.0f * (p1[1]*p[1] + p1[0]*p[0])
            - 4.0f * (p2[1]*p[1] + p2[0]*p[0]),
        solve_inf[3] - 4.0f * (p1[1]*p[1] + p1[0]*p[0]) + 4.0f * (p0[1]*p[1] + p0[0]*p[0]),
        root_pass
    );

    f32 d_best;

    auto ip = ap_sub(p0, p), fp = ap_sub(p2, p);
    //t = 0
    d_best = ap_dot(ip, ip);

    //t = 1
    f32 eDist = ap_dot(fp, fp);

    if (eDist < d_best)
        d_best = eDist;

    i32 i;
    f32 dx, dy, t, t_i, _D, alpha, beta, gamma;
    f32 xSgn = 1.0f, ySgn = 1.0f;

    for (i = 0; i < nRoots; i++) {
        t = root_pass[i];

        if (t < 0.0f || t > 1.0f) continue;

        t_i = 1.0f - t;
        alpha = t_i * t_i;
        beta = 2.0f * t_i * t;
        gamma = t * t;

        dx = (alpha * p0[0] + beta * p1[0] + gamma * p2[0]) - p[0];
        dy = (alpha * p0[1] + beta * p1[1] + gamma * p2[1]) - p[1];
        _D = dx*dx + dy*dy;
            
        if (_D < d_best) {
            xSgn = mu_sign(dx); //- sign will floor, + sign will ceil
            ySgn = mu_sign(dy); //same as above comment
            d_best = _D;
        }
    }

    //f32 vvv = wEncodeF32(sqrtf(d_best), xSgn, ySgn);
    return sqrtf(d_best);
}

#define USE_FAST_RC_CURVE_LEFT

f32 get_rc_cu_left_pos(gpu_rc_curve cu) {
#ifdef USE_FAST_RC_CURVE_LEFT 
    return mu_min(mu_min(cu.p0[0], cu.p1[0]), cu.p2[0]);
#else
    //TODO: change this to solve the quadratic and find the left most part of the curve
    return mu_min(mu_min(cu.p0[0], cu.p1[0]), cu.p2[0]);
#endif
}

/**********************************************************
 * 
 * Auto font tweaking
 * 
 * written by muffinshades 2026
 * 
 * below this comment is all the code for the auto font tweaking which will
 * be responsible for adjusting the position of part of glyphs automatically
 * for them to be clearer. This tech can be paired with clear type tech in
 * order for even more clear font, but I'm not gonna implement clear type 
 * for a little while so it's mainly for an alternate system to clear type
 * that should overall be better.
 * 
 * This is very similar to font focus, but I couldn't find a patent for it
 * so it's just my own implementation and code / design :3
 * 
 * Copyright muffinshades All Rights Reserve 2026-Present
 * 
 */
//ttf glyph tweeking
//friend seeking constants (can adjust these to properly get pairs)
//constexpr f32 pairity_thresh = 0.992546152f; //~cos(7deg)
constexpr f32 pairity_thresh = 0.9848f;

//finding glyph curve friends
//TODO: store solve inf through a shared groupmem between all the glyphs
//TODO: project either stright line for the curve onto the similar plane
            // and reject the curves if they dont collide since they aren't paired

            //then get the dist between the curves and minimize the distance
            //if one curve is smaller than another you might have to add an 
            //an exception system in order to properly pair curves and have
            //all friendships have a parent / core curve that is the one whose
            //position is adjusted. Also add a system to keep track of paired 
            //points since adjusting one curve will result in adjusting another

            /*
            
            New method: compute the estimated thickness for the parent stem then calculate
            the alignment in some sort of shader and then tweak the curves through the shader
            by just adjusting every curve's buddy
            
            */
void find_font_curve_friends(FontInst *font) {
    if (!font ||
        font->rc_dat.nCurves == 0 ||
        !font->rc_dat.lcs
    ) //holy checks bro :sob: 
    {
        std::cout << "could not generate friend data: bad stufff" << std::endl;
        std::cout << (uintptr_t) font << " | " << font->rc_dat.nCurves << " | " << (uintptr_t) font->rc_dat.lcs << std::endl;
        return;
    }
    
    i32 i,j,k;

    const size_t ncu = font->rc_dat.nCurves;

    gpu_rc_curve cc, fc;

    //compute solve info
    f32 *solve_inf = new f32[ncu * 4];

    if (!solve_inf) {
        std::cout << "error could not generate font correction info: failed to allocate solve info" << std::endl;
        return;
    }
    
    for (i = 0; i < ncu; i++) {
        cc = font->rc_dat.lcs[i];
        j = i << 2;
        solve_inf[j+0] = compute_a_base_coord(cc.p0[0], cc.p1[0], cc.p2[0]) + compute_a_base_coord(cc.p0[1], cc.p1[1], cc.p2[1]);
        solve_inf[j+1] = compute_b_base_coord(cc.p0[0], cc.p1[0], cc.p2[0]) + compute_b_base_coord(cc.p0[1], cc.p1[1], cc.p2[1]);
        solve_inf[j+2] = compute_c_base_coord(cc.p0[0], cc.p1[0], cc.p2[0]) + compute_c_base_coord(cc.p0[1], cc.p1[1], cc.p2[1]);
        solve_inf[j+3] = compute_d_base_coord(cc.p0[0], cc.p1[0], cc.p2[0]) + compute_d_base_coord(cc.p0[1], cc.p1[1], cc.p2[1]);
    }

    std::cout << "finding friends for: " << ncu << " curves and " << font->ad_inf.ngdata << " glyphs" << std::endl;

    //copyright James Weigand 2026-Present All Rights Reserved
    //TODO: interate this in a whole different way blud

    f32 avg_w = 0.0f;
    size_t nproc_glf = 0;

    auto process_glf = [&](Character& c, auto&& process_glf) -> void {
        if (c.nParts > 0) {
            u32 idx;

            for (i = 0; i < c.nParts; i++) {
                idx = c.spriteParts[i].idx;
                if (idx >= font->ad_inf.ngdata)
                    continue;
                process_glf(font->gdata[idx], process_glf);
            }

            return;
        }

        //std::cout << "cu range: " << c.rc_Dat.rc_curve_start << " --> " << c.rc_Dat.rc_curve_end << std::endl;

        if (c.rc_Dat.rc_curve_start >= font->rc_dat.nCurves && c.rc_Dat.rc_curve_end >= font->rc_dat.nCurves) {
            std::cout << "cannot font friends for glyph: " << c.val << std::endl;
            std::cout << "Range: " << c.rc_Dat.rc_curve_start << " --> " << c.rc_Dat.rc_curve_end << std::endl;
            return;
        }

        for (i = c.rc_Dat.rc_curve_start; i <= c.rc_Dat.rc_curve_end; i++) {
            cc = font->rc_dat.lcs[i];

            //skip curves that the minW was already computed for
            if (cc.minW > 0.0f)
                continue;

            //f32 minThickness = 2.524354746243960872064730e-29f;
            f32 minThickness = chonk_number;

            i32 lc = i;

            for (j = i+1; j <= c.rc_Dat.rc_curve_end; j++) {
                fc = font->rc_dat.lcs[j];

                auto cv0 = ap_sub(cc.p0, cc.p1), cv1 = ap_sub(cc.p1, cc.p2), cv2 = ap_sub(cc.p0, cc.p2);
                auto fv0 = ap_sub(fc.p0, fc.p1), fv1 = ap_sub(fc.p1, fc.p2), fv2 = ap_sub(fc.p0, fc.p2);

                const f32 mc0 = ap_mag(cv0), fc0 = ap_mag(fv0),
                          mc1 = ap_mag(cv1), fc1 = ap_mag(fv1),
                          mc2 = ap_mag(cv2), fc2 = ap_mag(fv2);

                /*std::cout << "aaa: " << ap_dot(cv0, fv0) << " " << pairity_thresh * mc0 * fc0 << "\n"
                                     << ap_dot(cv1, fv1) << " " << pairity_thresh * mc1 * fc1 << "\n"
                                    << ap_dot(cv2, fv2) << " " << pairity_thresh * mc2 * fc2;*/

                if (ap_dot(cv0, fv0) < pairity_thresh * mc0 * fc0 || 
                    ap_dot(cv1, fv1) < pairity_thresh * mc1 * fc1 ||
                    ap_dot(cv2, fv2) < pairity_thresh * mc2 * fc2
                ) //the 2 curves are not similar enough
                    continue;

                //check collisions
                f32 imc0 = 1.0f / mc0,
                    pNorm[2] = {cv0[0] * imc0, cv0[0] * imc0};

                const f32 pc0 = ap_dot(cc.p0, pNorm), pc1 = ap_dot(cc.p2, pNorm),
                        pf0 = ap_dot(fc.p0, pNorm), pf1 = ap_dot(fc.p2, pNorm);
            
                if (mu_max(pc0, pc1) < mu_min(pf0, pf1))
                    continue;

                //compute distances
                const f32 d0 = simple_curve_point_dir_dist(fc.p0, cc, solve_inf + (i << 2)),
                        d1 = simple_curve_point_dir_dist(fc.p2, cc, solve_inf + (i << 2));
                f32 T = d0;
                //if (abs(wDecodeF32(d1)) < abs(wDecodeF32(d0))) T = d1;
                if (abs(d1) < abs(d0)) T = d1;
                
                //min distance and ensure that the left most curve is the one being noted to store the minthickness
                //also make sure that the curve doesn't already have a computed width
                //if (abs(wDecodeF32(T)) < abs(wDecodeF32(minThickness))) {
                if (abs(T) < abs(minThickness)) {
                    minThickness = T;
                    if (
                        get_rc_cu_left_pos(font->rc_dat.lcs[j]) < get_rc_cu_left_pos(font->rc_dat.lcs[i]) && 
                        font->rc_dat.lcs[j].minW <= 0.0f
                    ) {
                        lc = j;
                    } else {
                        lc = i;
                    }
                }
            }

            //TODO: ensure that the curve that the minW is assigned to is the left most curve of the min pair (doesn't matter anymore with enw developments)
            //--> you can sort the curve left to right first if needed
            //--> the above solution could fuck the whole connection mapping
            //TODO: the actual connections mapping
            //if (wDecodeF32(minThickness) < 99.9e7f) {
            if (minThickness < 99.9e7f) {
                font->rc_dat.lcs[lc].minW = minThickness;
                avg_w += minThickness;
                nproc_glf++;
            }
        }
    };

    for (k = 0; k < font->ad_inf.ngdata; k++) {
        Character& c = font->gdata[k];

        process_glf(c, process_glf);
    }

    //check width orientation
    avg_w /= (f32) nproc_glf;

    _safe_free_a(solve_inf);
}

/*

#include <iostream>
#include <iomanip>
#include <bitset>

int main()
{
    float v = 0.5f;
    
    int iv = *((int*)(&v));
    
    std::cout << std::bitset<32>(iv) << std::endl;

    return 0;
}

*/