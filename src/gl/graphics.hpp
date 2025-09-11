#pragma once
#include <iostream>
#include "window.hpp"
#include "Shader.hpp"
#include "mesh.hpp"
#include "vertex.hpp"

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
    
    void texAttach(u32 w, u32 h);
    void depthStencilAttach(u32 w, u32 h);
    void depthAttach(u32 w, u32 h);

    u64 specialVal = 0; //reserved for derived classes
public:
    FrameBuffer(fb_type ty, u32 w, u32 h);
    u32 getHandle() {
        return this->handle;
    }
    void free() {
        if (this->handle != 0)
            glDeleteFramebuffers(1, &this->handle); 
    }
    friend class graphics;
};

struct graphicsState {
    u32 vao, vbo, ibo;
    Shader *s = nullptr;
    size_t nv = 0;
    enum class __gs_fmt {
        _null,
        _static,
        _dynamic
    } g_fmt = __gs_fmt::_null;

    struct {
        size_t v_obj_sz = 0;
    } __int_prop;
};

class graphics {
private:
    graphicsState interalState;
    graphicsState *cur_state = nullptr;
    Window *win = nullptr;
    mat4 proj_matrix;
    size_t _c_vert = 0, mush_offset = 0;
    Vertex *vmem = nullptr, *vstore = nullptr;

    void vmem_alloc();
    void vmem_clear();

    void bind_vao() {
        if (!this->cur_state)
            return;

        if (this->cur_state->vao != 0)
            glBindVertexArray(this->cur_state->vao);
        else
           std::cout << "Failed to bind to vao!" << std::endl;
    }

    Shader *def_shader = nullptr;
    Shader *s = nullptr;

    enum ReserveState {
        None,
        Mush,
        MeshBind,
        VertexDef
    } rs_state = ReserveState::None;
    bool using_foreign_gs = false;

    bool shader_bound = false;
    void shader_bind();
    void shader_unbind();

    f32 winW, winH;

public:
    graphics(Window *w) {
        if (w != nullptr)
            this->win = w;
        else
            return;

        this->winW = this->win->w;
        this->winH = this->win->h;
    }
    void Load();
    void WinResize(const size_t w, const size_t h);
    void render_begin();
    void render_flush();
    void render_noflush();
    void render_no_geo_update();
    void render_bind();
    void flush();
    void push_verts(void *v, size_t n);
    void mush_begin();
    void mush_render(Mesh *m);
    void mush_end();
    void mesh_single_bind(Mesh* m);
    void mesh_unbind();
    void setCurrentShader(Shader *s);
    void bindMeshToVbo(Mesh *m);

    void vertexStructureDefineBegin(size_t vObjSz);
    void defineVertexPart(i32 n, __mu_glVInf inf);
    void vertexStructureDefineEnd();

    //graphics states
    void iniStaticGraphicsState();
    void iniDynamicGraphicsState(size_t nBufferVerts);
    bool useGraphicsState(graphicsState *gs);
    void useDefaultGraphicsState();

    //frame buffers
    void setCurrentFrameBuffer(FrameBuffer *fb);
    void restoreDefFrameBuffer();

    Shader* getCurrentShader();
    //void DrawImage();
    //void FillRect(float x, float y, float w, float h);
    void free();
    const size_t getEstimatedMemoryUsage();
};