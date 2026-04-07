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
 * 3,000+ lines of code of adhd-induced speed. Can render ttf glyphs in numerous ways
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

struct MsdfGpuContext {
    FrameBuffer fb;
    FrameBuffer cc_fb; //curve correction frame buffer
    FrameBuffer cc_composite_fb;
    graphics g;
    u32 curveBuffer = 0;
    RenderStateDescriptor def_desc, cc_desc, cc_composite_desc;
    RenderState *cc_rstate = nullptr, *cc_composite_rstate = nullptr;
    bool good = false;
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

struct CharPart {
    CharSpritePos sheet_loc;
    p_mat_2d offset;
    struct {
        i32 xMax, xMin, yMax, yMin;
    } size;
};

struct Character {
    CharPart *spriteParts;
    size_t nParts = 0;
    u32 val;
    struct {
        u32 w, h;
        i32 rise; //how many pixels relative to the glyphs w / h the glyph is above the baseline
        f32 hw_ratio = 1.0f;
        h_char_metric metr;
        bool use_metr = false;
    } dim;
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
};

struct FontInst {
    UnicodeRange range;
    CharMap map;
    __msdf_data msdf_dat;
    __bmp_data bitmap_dat;
    FontMode mode = FontMode::Unknown;
    h_char_inf inf;
    struct {
        bool monospace = false, efficient_compound_glyphs = true;
        i32 unitsPerEm = 0;
        i16 ascent;
        i16 descent;
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

    MSFL_EXP static void _msdfRenderDebug(Glyph g, MsdfGpuContext** ctx);
    MSFL_EXP static void _msdfRenderDebug2(Glyph g, MsdfGpuContext** ctx);

    MSFL_EXP static i32 RenderGlyphMSDFToBitMap(Glyph tGlyph, Bitmap* bmp, sdf_dim size, bool accel = false);

    MSFL_EXP static void DeleteFontObject(FontInst *font);

    MSFL_EXP static void DeleteFontInst(FontInst *font);
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif