#pragma once
#include "ttf.hpp"
#include "bitmap.hpp"
#include "gl/graphics.hpp"   

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
    graphics g;
    u32 curveBuffer = 0;
    bool good = false;
};


struct CharSpritePos {
    u32 x, y, w, h;
    bool rotate = false;
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

struct Character {
    union {
        struct {
            i32 x,y,w,h;
            bool rotate_90 = false; //is the glyph rotated 90deg counter clockwise in the sheet (optinal thing when generating sheet since it can save space)
        } loc;
    } sprite;

    bool compound = false;
    u32 val;
};

struct FontInst {
    UnicodeRange range;
    Character *map = nullptr;
    union {
        struct {
            union {
                Bitmap bitmap;
                Texture gl_texture;
            } MSDF;
            struct {
                size_t w = 0, h = 0;
            } dim;
            MsdfGpuContext *_dbg_ctx;
        } msdf_dat;
        struct {
            Bitmap bmp;
        } bitmap_dat;
    };
    FontMode mode = FontMode::Unknown;
    bool good = false;
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
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif