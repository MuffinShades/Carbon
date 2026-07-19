#pragma once
#include "ttf.hpp"
#include "bitmap.hpp"
#include "gl/graphics.hpp" 
#include "gl/Texture.hpp"  

/**
 * 
 * ttf_render.hpp
 * 
 * For rendering ttf glyphs
 * 
 * written by muffinshades 2024-2025
 * 
 * 5,000+ lines of code of adhd-induced speed. Can render ttf glyphs in numerous ways
 * 
 * ------------------------------------
 * 
 * 1. MSDF Rendering
 * 
 * Can generate msdf for many glyphs in a small-compact sprite sheet.
 * 4 Generation Modes
 * - No-Acceleration (cpu) (slowest)
 * - Multi-Threaded (cpu)
 * - GPU-Accel (cpu+gpu)
 * - GPU-Thread-Accel (cpu+gpu) (fastest)
 * 
 * ------------------------------------
 * 
 * Regardless of method program will return a Font Instance object which can
 * then be passed to render strings will various effects depeding on method 
 * chosen and whatever the hell I feel like actually adding cause holy shit
 * this is taking way longer than I thought and has become one of the most
 * complicated pieces of software I have ever written.
 * 
 */

enum class sdf_dim_ty {
    Width,
    Height,
    Scale
};

struct sdf_dim {
    union {
        u32 w;
        u32 h;
        f32 scale;
    } m;
    sdf_dim_ty slc;
};

static sdf_dim sdf_width_dim(u32 w) {
    sdf_dim d;

    d.m.w = w;
    d.slc = sdf_dim_ty::Width;

    return d;
}

static sdf_dim sdf_height_dim(u32 h) {
    sdf_dim d;

    d.m.h = h;
    d.slc = sdf_dim_ty::Height;

    return d;
}

static sdf_dim sdf_scale_dim(f32 s) {
    sdf_dim d;

    d.m.scale = s;
    d.slc = sdf_dim_ty::Scale;

    return d;
}

#ifdef MSFL_DLL
#ifdef MSFL_EXPORTS
#define MSFL_EXP __declspec(dllexport)
#else
#define MSFL_EXP __declspec(dllimport)
#endif
#else
#define MSFL_EXP
#endif

#ifdef MSFL_DLL
#ifdef __cplusplus
extern "C" {
#endif
#endif

struct _trace_inf {
    std::string font_src = "";
};

struct MsdfGpuContext {
    struct rcGenContext *rc_ctx = nullptr;
    FrameBuffer fb;
    FrameBuffer cc_fb; //curve correction frame buffer
    FrameBuffer cc_composite_fb;
    graphics g;
    u32 curveBuffer = 0;
    RenderStateDescriptor def_desc, cc_desc, cc_composite_desc;
    RenderState *cc_rstate = nullptr, *cc_composite_rstate = nullptr;
    bool good = false;
    _trace_inf trace;
};


struct CharSpritePos {
    u32 x, y, w, h;
    bool rotate_90 = false;
};

enum class MsdfMode {
    Bitmap,
    GL_Texture
};

enum class FontMode {
    Unknown,
    Bitmap,
    MSDF,
    SDF
};

/*

NOTE IF A CHARACTER HAS ONLY 1 CHAR PART --> ignore offset

*/

enum class FontLang {
    en
};

class CharPredModel {
private:
    FontLang lang;
public:

};

struct gpu_rc_curve {
    f32 p0[2];
    //volatile f32 zzzzzbob_aka_padding[2];
    f32 p1[2];
    //volatile f32 zzzzzbilly_aka_more_padding[2];
    f32 p2[2];

    //specifies the min width of the stem formed by the curve
    f32 minW = -100.0f;

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
    i32 cu_connect = 0; 
};

struct CharPart {
    i32 id = -1;
    CharSpritePos sheet_loc;
    p_mat_2d offset;
    struct {
        u32 rc_curve_start = 0, rc_curve_end = 0; //end and beginning indexes of the curves and where they're stored
    } rc_Dat;
    struct {
        i32 xMax, xMin, yMax, yMin;
    } size;
};

struct Character {
    GlyphPart *spriteParts = nullptr;
    size_t nParts = 0;
    u32 val;
    struct {
        u32 w, h;
        i32 rise; //how many pixels relative to the glyphs w / h the glyph is above the baseline
        f32 hw_ratio = 1.0f;
        h_char_metric metr;
        bool use_metr = false;
        struct {
            f32 xMin, yMin, xMax, yMax;
        } ranges;
    } dim;
    struct {
        u32 rc_curve_start = 0, rc_curve_end = 0; //end and beginning indexes of the curves and where they're stored
    } rc_Dat;
    struct {
        bool msdf_support = false;
        CharSpritePos sheet_loc;
    } sprite_dat;
    h_char_metric hmetrics;
};

struct __msdf_dim {
    size_t w = 0, h = 0;
};

struct __msdf_tex {
    Bitmap *bitmap = nullptr;
    BindableTexture gl_texture;
    struct {
        BindableTexture cc_tex;
    } __dbg;
};

struct __msdf_data {
    __msdf_tex MSDF;
    __msdf_dim dim;
    MsdfGpuContext *_dbg_ctx = nullptr;
    MsdfMode mode = MsdfMode::GL_Texture;
};

struct __bmp_data {
    Bitmap bmp;
};

struct __ray_data {
    gpu_rc_curve* lcs = nullptr;
    size_t nCurves = 0;
    u32 cu_buf = 0;
    bool cu_buf_good = false;
};

struct CharLink {
    Character ochar;
    CharLink *next = nullptr, *prev = nullptr;
};

enum class CharMapType {
    Hash, //char id is linked to a hash map
    Direct //char id is index
};

struct CharMap {
    struct {
        size_t nBits = 0;
        size_t sz = 0;
    } hash_inf;
    CharMapType ty = CharMapType::Direct;
    CharLink *hash_map = nullptr;
    i32 firstId = 0;
};

struct lite_glyf_dat {
    u16 n_parts;
    u32 *part_ids = nullptr;
};

struct FontInst {
    UnicodeRange range;
    CharMap classic_map;

    Character *gdata = nullptr;

    /*
    
    Format: each u32 is an index into the map / storage of the characters + 1

    0 represents -1 which indicates the glyph is not present in the ttf
    
    */
    struct {
        u32 *mpa8 = nullptr, //256*4 bytes 
            *mpa16 = nullptr; //65535*4 bytes
    } c_translate;
    __msdf_data msdf_dat;
    __bmp_data bitmap_dat;
    __ray_data rc_dat;
    struct {
        CharPredModel model;
    } predict;
    FontLang target_lang = FontLang::en;
    FontMode mode = FontMode::Unknown;
    struct {
        h_char_inf inf;
        h_char_metric *metrics = nullptr;
    } h_inf;
    struct {
        bool monospace = false, efficient_compound_glyphs = true;
        bool use_prediction = false, wchar_support = false;
        i32 unitsPerEm = 0;
        i16 ascent;
        i16 descent;
        struct {
            bool useRayCountAtSmallScales = true;
            i32 maxFontSizeForRayCount = 50;
            size_t nCurvesInRcBuffer = 2048;
        } render;
        maxVals mxv;
        u32 minChar = 0;
        u16 ngdata = 0;
        u32 null_char_loc = 0;
    } ad_inf;
    bool good = false;
};

struct MsdfSettings {
    bool efficient_compound_glyphs = false;
    bool accel = true;
};

class ttfRender {
public:
    enum {
        RenderAccel_
    };

    MSFL_EXP static i32 RenderGlyphToBitmap(Glyph tGlyph, Bitmap* bmp, float scale = 1.0f);
    MSFL_EXP static i32 RenderGlyphOutlineToBitmap(Glyph tGlyph, Bitmap* bmp, sdf_dim size);
    MSFL_EXP static i32 RenderGlyphSDFToBitMap(Glyph tGlyph, Bitmap* bmp, sdf_dim size);

    MSFL_EXP static i32 RenderSDFToBitmap(Bitmap* sdf, Bitmap* bmp, sdf_dim res_size);
    MSFL_EXP static i32 RenderMSDFToBitmap(Bitmap* sdf, Bitmap* bmp, sdf_dim res_size);
    MSFL_EXP static FontInst GenerateUnicodeMSDFSubset(std::string src, UnicodeRange range, sdf_dim first_char_size, bool accel = false);
    MSFL_EXP static FontInst GenerateFontFromForeign(std::string img_src, std::string json_layout_src);

    MSFL_EXP static void _msdfRenderDebug(Glyph g, MsdfGpuContext** ctx);
    //MSFL_EXP static void _msdfRenderDebug2(Glyph g, MsdfGpuContext** ctx);

    MSFL_EXP static i32 RenderGlyphMSDFToBitMap(Glyph tGlyph, Bitmap* bmp, sdf_dim size, bool accel = false);

    MSFL_EXP static void DeleteFontObject(FontInst *&font);
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif