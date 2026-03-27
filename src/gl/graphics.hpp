#pragma once
#include <iostream>
#include "window.hpp"
#include "Shader.hpp"
#include "mesh.hpp"
#include "vertex.hpp"
#include "../bitmap.hpp"

#define _CARBONGL_SHADOW_SPECIAL_VAL 0xfb01

struct __mu_glVInf {
    size_t off = 0, sz = 0, p_sz = 0;
};

extern inline __mu_glVInf __gen_glvinf(size_t sz, size_t off, size_t p_sz) {
    return {
        .off = off,
        .sz = sz,
        .p_sz = p_sz
    };
}

#define vertexClassPart(v, part) \
    __gen_glvinf(sizeof(v), offsetof(v, part), sizeof(v::part))

#define vertexFloatBufSelect(v_buf, i, part) \
    v_buf[((sizeof(v_buf) / sizeof(f32)) * (i))]

struct OutputDevice {
    void *device = nullptr;
    enum {
        DefaultOutputDevice,
        FrameBuffer,
        Unknown
    } type;
};

struct FrameBufferExtInf {
    u32 color_fmt = GL_RGBA8, data_color_fmt = GL_RGBA;
    bool mipmap = false;
};

class FrameBuffer {
public:
    enum fb_type {
        Texture,
        Depth,
        Render,
        Unknown
    };
protected:
    u32 handle = 0, texHandle = 0, rbo = 0;
    fb_type ty = FrameBuffer::Texture;

    void bind();
    void delete_tex();
    
    void texAttach(u32 w, u32 h, FrameBufferExtInf ex_inf);
    void depthStencilAttach(u32 w, u32 h);
    void depthAttach(u32 w, u32 h);

    OutputDevice od;

    u64 specialVal = 0; //reserved for derived classes
public:
    u32 w = 0, h = 0;

    FrameBuffer() {}
    FrameBuffer(fb_type ty, u32 w, u32 h, FrameBufferExtInf ex_inf);
    u32 getHandle() {
        return this->handle;
    }
    void free() {
        if (this->handle != 0)
            glDeleteFramebuffers(1, &this->handle); 
    }
    Bitmap extractBitmap();
    void extractToBitmap(Bitmap *map);

    friend class graphics;
    friend class graphics2;

    OutputDevice *device() {
        if (!this->od.device) {
            this->od.device = (void*) this;
            this->od.type = OutputDevice::FrameBuffer;
        }

        return &this->od;
    }

    u32 getTextureHandle() {return this->texHandle;}
};

struct GenericFontProperties {
    struct {
        i32 pt;
        i32 px;
        i32 kern;
        i32 tab_size = 4;
        struct {
            f32 h = 1.0f; //horizontal warp
            f32 v = 1.0f; //vertical warp
        } warp;
    } scale;
    struct {
        bool bold = false;
        bool italic = false;
        bool underline = false;
        bool strikethrough = false;
    } style;
};

struct CustomFontProperties {
    struct {
        i32 pt;
        i32 px;
        i32 kern;
        i32 tab_size = 4;
    } scale;
    struct {
        Shader msdf;
        Shader sdf;
        //TODO: add other shaders for other render methods
    } shader;
};

struct BasicVertex {
    f32 x,y,z;
};

struct RenderStateDescriptor {
    bool dynamic = false;
    bool use_indicies = false;
    size_t max_batch_verts = 0xffff;
    size_t max_batch_indicies = 0xffff;
    size_t vertex_size = 0;
    size_t indc_sz = 4;
    size_t vertex_overflow_buffer_cell_verts = 0xfff;
    size_t indicie_overflow_buffer_cell_verts = 0xfff;
    bool auto_store_extra = true;
    u32 render_primitive = GL_TRIANGLES;
};

struct _OverflowBufferCell {
    _OverflowBufferCell* next = nullptr;
    void *dat = nullptr;
    size_t iPos = 0;
};

struct RenderState {
    enum class __gs_fmt {
        _null,
        _static,
        _dynamic
    } g_fmt = __gs_fmt::_null;
    //indicie size is by default an  unsigned int
    size_t vertex_size = 0, indc_sz = 4;
    u32 vao = 0, vbo = 0, ibo = 0;
    OutputDevice* oDevice = nullptr;
    size_t c_vert = 0, c_ind = 0; //current vertex and current indicie
    struct {
        u32 w,h;
    } dim;
    struct {
        bool _ini = false;
        bool _vert_def = false;
        bool _null = false;
        struct {
            i32 nBindings = 0;
        } _protect;
        RenderStateDescriptor _desc;
    } _p_inf;
    enum class Process {
        None,
        VertexDefine,
        Locked,
        Render
    } cur_process = Process::None;
    enum class SupressionLevel {
        None,
        NothingWarnings,
        Warnings,
        Errors
    } suppress = SupressionLevel::None;
    Shader *cur_shader = nullptr;

    //NOTE: each cell's size is store under _p_inf._desc.vertex_overflow_buffer_cell_verts or the other
    _OverflowBufferCell *vtx_overflow = nullptr, *idc_overflow = nullptr, *vtxo_cur = nullptr, *idco_cur = nullptr;
};

/*

TODO:

check set indicies visually
finish the storing functinoality
render flush
functions to render meshes easier
testing
constructors

*/

class graphics {
private:
    RenderState *default_state = nullptr;
    RenderState *state = nullptr, *prev_state = nullptr;

    void _IniCurrentGraphicsState(RenderStateDescriptor desc);
    void _StoreExtraVerts(void *v_buf, size_t sz);
    void _StoreExtraIndicies(void *i_buf, size_t sz);

    void ini_generic_font_state();

    static OutputDevice _OutputDev_Default;
public:
    //render states
    RenderState *CreateBlankRenderState();
    RenderState *CreateNewRenderState(RenderStateDescriptor desc);
    void SetRenderState(RenderState *state);
    RenderState *GetCurrentRenderState();
    void RestoreLastRenderState();
    void RestoreDefaultRenderState();

    void VertexDefineBegin(size_t v_obj_sz);
    void DefineVertexPart(i32 part_index, __mu_glVInf inf);
    void DefineIntegerVertexPart(i32 part_index, __mu_glVInf inf);
    void VertexDefineEnd();

    static void DeleteRenderState(RenderState *state);

    //render functions
    void SetShader(Shader *shader);
    Shader* GetCurrentShader();
    void RenderBegin(i32 w = -1, i32 h = -1);
    void RenderContinue();
    void PushVerts(void *verts, size_t n_verts, bool auto_flush_old);
    void SetVerts(void *verts, size_t n_verts, bool flush_current);
    void PushIndicies(void *indicies, size_t n_indicies, bool auto_flush_old);
    void SetIndicies(void *indicies, size_t n_indicies, bool flush_current);
    void RenderFlush();
    void ClearOutput();
    void LockState(); //locks rendering and output
    void UnlockState();

    //tesselation
    void SetTesselationVertNum(size_t n_tes);

    //text
    void RenderString(struct FontInst *font, f32 x, f32 y, f32 z, const char* str, GenericFontProperties prop);
    void RenderString(struct FontInst *font, f32 x, f32 y, f32 z, const char* str, CustomFontProperties prop);
    f32 ComputeStringWidth(struct FontInst *font, const char* str);
    f32 ComputeStringHeight(struct FontInst *font, const char* str);

    //frame buffers and stuff
    void SetOutputDevice(OutputDevice* device);
    void RestoreDefaultOutputDevice();

    //idek
    u32 getOutputWidth();
    u32 getOutputHeight();

    //constructors
    graphics();
    graphics(RenderStateDescriptor desc);
    graphics(RenderState *def_state);

    void free() {
        if (state == this->default_state)
            state = nullptr;
        delete[] this->default_state;
    }

    void Resize(u32 w, u32 h);

    friend bool render_precheck(graphics *g);
};