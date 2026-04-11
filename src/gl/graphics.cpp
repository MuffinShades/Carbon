#include "graphics.hpp"
#include <glad.h>

#define BYTE_TO_CONST_CHAR(b) const_cast<const char*>(reinterpret_cast<char*>((b)))
#define gpu_dynamic_alloc(sz) glBufferData(GL_ARRAY_BUFFER, sz, nullptr, GL_DYNAMIC_DRAW)

#define BATCH_NQUADS 0xffff
#define BATCH_SIZE 0xfffff

#define define_vattrib_struct(i, type, target) \
    glEnableVertexAttribArray(i); \
    glVertexAttribPointer(i, sizeof(type::target) / FL_SZ, GL_FLOAT, GL_FALSE, sizeof(type), (void *)offsetof(type, target))

#define vertex_float_attrib(i, s, sz, off) glVertexAttribPointer(i, s, GL_FLOAT, GL_FALSE, sz, (void *)off); \
    glEnableVertexAttribArray(i)

#define FL_SZ 4
#define define_vattrib_struct(i, type, target) \
    glEnableVertexAttribArray(i); \
    glVertexAttribPointer(i, sizeof(type::target) / FL_SZ, GL_FLOAT, GL_FALSE, sizeof(type), (void *)offsetof(type, target))

#define vao_create(vao) glGenVertexArrays(1, &vao)

#define vbo_bind(vbo) glBindBuffer(GL_ARRAY_BUFFER, (vbo))

void FrameBuffer::bind() {
    if (!this->handle)
        std::cout << "Warning: binding to zero framebuffer! Framebuffer may have failed to generate!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, this->handle);
}

void FrameBuffer::delete_tex() {
    if (this->texHandle)
        glDeleteTextures(1, &this->texHandle);

    this->texHandle = 0;
}
    
void FrameBuffer::texAttach(u32 w, u32 h, FrameBufferExtInf ex_inf) {
    this->delete_tex();

    glGenTextures(1, &this->texHandle);

    if (!this->texHandle) {
        std::cout << "Failed to generate framebuffer rgb texture!" << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, this->texHandle);

    glTexImage2D(GL_TEXTURE_2D, 0, ex_inf.color_fmt, w, h, 0, ex_inf.data_color_fmt, GL_UNSIGNED_BYTE, nullptr);

    if (ex_inf.mipmap)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, this->handle);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->texHandle, 0);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    this->w = w;
    this->h = h;
}

void FrameBuffer::depthStencilAttach(u32 w, u32 h) {
    this->delete_tex();

    glGenTextures(1, &this->texHandle);

    if (!this->texHandle) {
        std::cout << "Failed to generate framebuffer depth/stencil texture!" << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, this->texHandle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, this->handle);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH24_STENCIL8, GL_TEXTURE_2D, this->texHandle, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::depthAttach(u32 w, u32 h) {
    this->delete_tex();

    glGenTextures(1, &this->texHandle);

    if (!this->texHandle) {
        std::cout << "Failed to generate framebuffer depth/stencil texture!" << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, this->texHandle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, this->handle);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->texHandle, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::FrameBuffer(FrameBuffer::fb_type ty, u32 w, u32 h, FrameBufferExtInf ex_inf) {
    this->ty = ty;
    glGenFramebuffers(1, &this->handle);

    switch (ty) {
    case FrameBuffer::Texture:
        this->texAttach(w, h, ex_inf);
        break;
    case FrameBuffer::Depth:
        this->depthAttach(w, h);
        break;
    case FrameBuffer::Render:
        std::cout << "TODO: rbo!" << std::endl;
        break;
    default:
        std::cout << "Error: invalid frame buffer type: " << (u32)ty << std::endl;
        this->ty = FrameBuffer::Unknown;
        this->free();
        break;
    }
}

Bitmap FrameBuffer::extractBitmap() {
    if (!this->handle || this->w == 0 || this->h == 0)
        return {};

    if (this->ty != FrameBuffer::Texture) {
        std::cout << "Warning: cannot extract bitmap on non texture framebuffers!" << std::endl;
        return {};
    }

    Bitmap bmp;

    bmp.header.fSz = this->w * this->h * 4;

    bmp.data = new byte[bmp.header.fSz];

    ZeroMem(bmp.data, bmp.header.fSz);

    bmp.header.bitsPerPixel = 32;
    bmp.header.w = this->w;
    bmp.header.h = this->h;

    glReadPixels(0, 0, this->w, this->h, GL_RGBA, GL_UNSIGNED_BYTE, bmp.data);

    return bmp;
}

void FrameBuffer::extractToBitmap(Bitmap *map) {
    if (!map ) return;

    const size_t readW = mu_min(this->w, map->header.w), readH = mu_min(this->h, map->header.h);

    if (!map->data) {
        map->header.w = this->w;
        map->header.h = this->h;
        map->header.bitsPerPixel = 32;

        map->data = new byte[this->w * this->h * 4];

        glReadPixels(0, 0, this->w, this->h, GL_RGBA, GL_UNSIGNED_BYTE, map->data);
    } else {
        if (map->header.w == 0 || map->header.h == 0) return;

        switch (map->header.bitsPerPixel) {
        case 8:
            glReadPixels(0, 0, readW, readH, GL_R, GL_UNSIGNED_BYTE, map->data);
            break;
        case 16:
            glReadPixels(0, 0, readW, readH, GL_RG, GL_UNSIGNED_BYTE, map->data);
            break;
        case 24:
            glReadPixels(0, 0, readW, readH, GL_RGB, GL_UNSIGNED_BYTE, map->data);
            break;
        case 32:
            glReadPixels(0, 0, readW, readH, GL_RGBA, GL_UNSIGNED_BYTE, map->data);
            break;
        default:
            std::cout << "Failed to write framebuffer to bitmap! Strange bit depth: " << map->header.bitsPerPixel << std::endl;
        }
    }
}
























OutputDevice graphics::_OutputDev_Default = {
    .device = nullptr,
    .type = OutputDevice::DefaultOutputDevice
};

void graphics::_IniCurrentGraphicsState(RenderStateDescriptor desc) {
    if (!state) {
        std::cout << "Graphics Warning | Failed to configure render state: current render state is a nullptr! \n\tMake sure you correctly created the currently bound render state." << std::endl;
        return;
    }

    /*if (!state->_p_inf._null) {
        std::cout << "Graphics Warning | Failed to configure render state: cannot configure null render state! \n\tMake sure you have set a valid render state and are not using a deleted state." << std::endl;
        return;
    }*/

    state->_p_inf._null = false;
    state->_p_inf._desc = desc;

    //create buffer objects
    glGenVertexArrays(1, &state->vao);
    glGenBuffers(1, &state->vbo);

    if (desc.use_indicies) {
        glGenBuffers(1, &state->ibo);

        if (!state->ibo) {
            std::cout << "Graphics Error | Failed to configure render state: failed to create IBO!" << std::endl;
            return;
        }
    }

    if (!state->vao) {
        std::cout << "Graphics Error | Failed to configure render state: failed to create VAO!" << std::endl;
        return;
    }

    if (!state->vao) {
        std::cout << "Graphics Error | Failed to configure render state: failed to create VBO!" << std::endl;
        return;
    }

    glBindVertexArray(state->vao);

    //dynamic and static related things
    if (desc.dynamic) {
        state->g_fmt = RenderState::__gs_fmt::_dynamic;
        glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        glBufferData(GL_ARRAY_BUFFER, desc.max_batch_verts * desc.vertex_size, 0, GL_DYNAMIC_DRAW);

        if (desc.use_indicies) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
            glBufferData(GL_ARRAY_BUFFER, desc.max_batch_indicies, 0, GL_DYNAMIC_DRAW);
        }
    } else {
        state->g_fmt = RenderState::__gs_fmt::_static;
    }

    glBindVertexArray(0);

    state->_p_inf._ini = true;
}

void delete_overflow_buffer(_OverflowBufferCell *root) {
    while (root) {
        _safe_free_a(root->dat);
        auto *nx = root->next;
        _safe_free_b(root);
        root = nx;
    }
}

void graphics::_StoreExtraVerts(void *v_buf, size_t sz) {
    if (!v_buf || sz == 0) {
        std::cout << "(Internal) Graphics Error | Cannot store invalid extra verticies!" << std::endl;
        return;
    }

    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "(Internal) Graphics Error | Failed to store extra verts: invalid render state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "(Internal) Graphics Warning | Cannot store extra verts whilsts render state is locked!" << std::endl;
        return;
    }

    //TODO: store the extra verts and handle all that
    if (state->vtx_overflow) {
        const size_t lCopy = 0;

        if (lCopy >= sz) {

        } else {

        }
    }
}

void graphics::_StoreExtraIndicies(void *i_buf, size_t sz) {
    if (!i_buf || sz == 0) {
        std::cout << "(Internal) Graphics Error | Cannot store invalid extra indicies!" << std::endl;
        return;
    }

    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "(Internal) Graphics Error | Failed to store extra verts: invalid render state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "(Internal) Graphics Warning | Cannot store extra verts whilsts render state is locked!" << std::endl;
        return;
    }

    //TODO: store the extra indicies and handle all that
}

RenderState *graphics::CreateBlankRenderState() {
    RenderState *rs = new RenderState;
    ZeroMem(rs, 1);
    return rs; //most underwelming function ever
}

RenderState *graphics::CreateNewRenderState(RenderStateDescriptor desc) {
    RenderState *rs = new RenderState;
    ZeroMem(rs, 1);
    rs->_p_inf._desc = desc;
    return rs;
}

void graphics::SetRenderState(RenderState *s) {
    if (!s) {
        std::cout << "Graphics Warning | Failed to set render state: current render state is a nullptr! \n\tMake sure you correctly created the currently bound render state." << std::endl;
        return;
    }

    if (s->_p_inf._null) {
        std::cout << "Graphics Warning | Failed to set render state: cannot set null render state! \n\tMake sure you have set a valid render state and are not using a deleted state." << std::endl;
        return;
    }

    if (state) {
        if (state->_p_inf._protect.nBindings > 0)
            state->_p_inf._protect.nBindings--;
        else {
            std::cout << "Graphics Warning | State is bound but also isn't... schrodinger's render state" << std::endl;
            state->_p_inf._protect.nBindings = 0;
        }
    }

    prev_state = state;
    state = s;

    glViewport(0, 0, state->dim.w, state->dim.h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    FrameBuffer *fb = nullptr;

    if (state->oDevice) {
    switch (state->oDevice->type) {
    case OutputDevice::FrameBuffer:
        fb = (FrameBuffer*) state->oDevice->device;

        if (!fb) {
            std::cout << "Graphics Warning | Restoring state outputdevice to default! Because of invalid output device" << std::endl;
            this->RestoreDefaultOutputDevice();
            break;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fb->getHandle());
        break;
    case OutputDevice::Unknown:
        std::cout << "Graphics Warning | Output device is unknown! Restoring default output device." << std::endl;
        this->RestoreDefaultOutputDevice();
        break;
    }
    }

    if (!s->_p_inf._ini)
        this->_IniCurrentGraphicsState(s->_p_inf._desc);

    s->_p_inf._protect.nBindings++;
}

RenderState *graphics::GetCurrentRenderState() {
    return state;
}

void graphics::RestoreLastRenderState() {
    RenderState *temp = prev_state;
    prev_state = state;
    state = temp;
}

void graphics::RestoreDefaultRenderState() {
    prev_state = state;
    state = this->default_state;
}

void graphics::VertexDefineBegin(size_t v_obj_sz) {
    if (!state) {
        std::cout << "Graphics Warning | Failed to begin vertex define: current render state is a nullptr! \n\tMake sure you correctly created the currently bound render state." << std::endl;
        return;
    }

    if (state->_p_inf._null || !state->_p_inf._ini || !state->vbo) {
        std::cout << "Graphics Warning | Failed to begin vertex define: cannot define vertex for undefined render state! \n\tVBO: " << state->vbo << "\n\tnull: " << state->_p_inf._null << "\n\tini: " << state->_p_inf._ini << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot call vertex define related functions whilsts in a locked state" << std::endl;
        return;
    }

    const RenderStateDescriptor desc = state->_p_inf._desc;

    //check for descrenpency between vobjsz and stored size and fix it
    if (state->vertex_size != v_obj_sz && desc.dynamic) {
        glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        glBufferData(GL_ARRAY_BUFFER, desc.max_batch_verts * v_obj_sz, 0, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    glBindVertexArray(state->vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    state->vertex_size = v_obj_sz;
    state->cur_process = RenderState::Process::VertexDefine;
}

void graphics::DefineVertexPart(i32 part_index, __mu_glVInf inf) {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error Failed to define vertex part: invalid render state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot call vertex define related functions whilsts in a locked state" << std::endl;
        return;
    }

    if (state->cur_process != RenderState::Process::VertexDefine) {
        std::cout << "Graphics Error | Cannot define vertex part: make sure you called VertexDefineBegin first!" << std::endl;
        return;
    }

    glVertexAttribPointer(part_index, inf.p_sz / sizeof(f32), GL_FLOAT, GL_FALSE, inf.sz, (void*)inf.off);
    glEnableVertexAttribArray(part_index);
}

void graphics::DefineIntegerVertexPart(i32 part_index, __mu_glVInf inf) {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to define vertex part: invalid render state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot call vertex define related functions whilsts in a locked state" << std::endl;
        return;
    }

    if (state->cur_process != RenderState::Process::VertexDefine) {
        std::cout << "Graphics Error | Cannot define vertex part: make sure you called VertexDefineBegin first!" << std::endl;
        return;
    }

    glVertexAttribIPointer(part_index, inf.p_sz / sizeof(i32), GL_INT, inf.sz, (void*)inf.off);
    glEnableVertexAttribArray(part_index); 
}

void graphics::VertexDefineEnd() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to end vertex define: invalid render state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot call vertex define related functions whilsts in a locked state" << std::endl;
        return;
    }

    if (state->cur_process != RenderState::Process::VertexDefine)
        std::cout << "Graphics Warning | Ending vertex define that never began!" << std::endl;

    state->cur_process = RenderState::Process::None;
    state->_p_inf._vert_def = true;
}

void graphics::DeleteRenderState(RenderState *state) {
    if (!state)
        return;

    if (state->cur_process == RenderState::Process::Locked)
        std::cout << "Graphics Warning | Deleting locked render state!" << std::endl;

    if (state->vao) glDeleteVertexArrays(1, &state->vao);
    if (state->vbo) glDeleteBuffers(1, &state->vbo);
    if (state->ibo) glDeleteBuffers(1, &state->ibo);

    ZeroMem(state, sizeof(RenderState));


    //ensure that if this state is bound it will properly trigger an error somewhere :)
    _safe_free_a(state);
}

//render functions
void graphics::SetShader(Shader *shader) {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to set shader: invalid render state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot set shader whilsts graphics render state is locked!" << std::endl;
        return;
    }

    if (!shader || !shader->good()) {
        std::cout << "Graphics Warning | Failed to set shader: bad shader!" << std::endl;
        return;
    }

    state->cur_shader = shader;
}

Shader* graphics::GetCurrentShader() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to get shader: invalid render state!" << std::endl;
        return nullptr;
    }

    return state->cur_shader;
}

bool render_precheck(graphics *g) {
    if (!g->state || g->state->_p_inf._null || !g->state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to begin render: invalid render state!" << std::endl;
        return false;
    }

    if (g->state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot render whilsts render state is locked!" << std::endl;
        return false;
    }

    if (g->state->cur_process != RenderState::Process::None && g->state->cur_process != RenderState::Process::Render) {
        std::cout << "Graphics Warning | Cannot render whilsts state is busy with something else!" << std::endl;
        return false;
    }

    if (!g->state->_p_inf._vert_def) {
        std::cout << "Graphics Error | Failed to render: vertex structure is not defined.\n\tMake sure you call the VertexDefine family of functions to define the structure of your vertex!" << std::endl;
        return false;
    }

    return true;
}

void graphics::RenderBegin(i32 w, i32 h) {
    if (!render_precheck(this)) return;

    if (w >= 0)
        state->dim.w = w;

    if (h >= 0)
        state->dim.h = h;

    glViewport(0,0,state->dim.w, state->dim.h);
    glBindVertexArray(state->vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    state->c_vert = 0;
    state->cur_process = RenderState::Process::Render;
}

void graphics::RenderContinue() {
    if (!render_precheck(this)) return;

    glBindVertexArray(state->vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    
    state->cur_process = RenderState::Process::Render;
}

//TODO: all these actually important functions
void graphics::PushVerts(void *verts, size_t n_verts, bool auto_flush_old) {
    if (!verts || n_verts == 0) {
        std::cout << "Graphics Warning | Cannot push invalid verts!" << std::endl;
        return;
    }

    if (!render_precheck(this)) return;

    if (n_verts > state->_p_inf._desc.max_batch_verts) {
        std::cout << "Graphics Warning | Cannot push " << n_verts << " verticies at once!\n\tPush geometry in chunks and render each chunk or increase vertex batch size!" << std::endl;
        return;
    }

    if (state->c_vert + n_verts >= state->_p_inf._desc.max_batch_verts) {
        if (auto_flush_old) {
            if (state->_p_inf._desc.use_indicies) {
                _StoreExtraVerts(verts, n_verts * state->vertex_size);
            } else {
                this->RenderFlush(false);
                this->RenderBegin();
                state->cur_shader->usaveRestore();
            }
        } else {
            std::cout << "Graphics Warning | Extra verticies! Max sure to called RenderFlush or enable auto flush!" << std::endl;

            if (state->_p_inf._desc.auto_store_extra)
                _StoreExtraVerts(verts, n_verts * state->vertex_size);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    switch (state->g_fmt) {
    case RenderState::__gs_fmt::_dynamic:
        glBufferSubData(GL_ARRAY_BUFFER, state->c_vert * state->vertex_size, n_verts * state->vertex_size, verts);
        state->c_vert += n_verts;
        break;
    case RenderState::__gs_fmt::_static:
        if ((i32) state->suppress < (i32) RenderState::SupressionLevel::NothingWarnings)
            std::cout << "Graphics Warning | Push verts is just set verts when dealing with a static context!" << std::endl;
        glBufferData(GL_ARRAY_BUFFER, n_verts * state->vertex_size, verts, GL_STATIC_DRAW);
        state->c_vert = n_verts;
        break;
    }
}

void graphics::SetVerts(void *verts, size_t n_verts, bool flush_current) {
    if (!verts || n_verts == 0) {
        std::cout << "Graphics Warning | Cannot set invalid verts!" << std::endl;
        return;
    }

    if (!render_precheck(this)) return;

    if (n_verts > state->_p_inf._desc.max_batch_verts) {
        std::cout << "Graphics Warning | Cannot set " << n_verts << " verticies at once!\n\tPush geometry in chunks and render each chunk or increase vertex batch size!" << std::endl;
        return;
    }

    if (flush_current) {
        this->RenderFlush();
        this->RenderBegin();
    }

    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    switch (state->g_fmt) {
    case RenderState::__gs_fmt::_dynamic:
        glBufferSubData(GL_ARRAY_BUFFER, 0, n_verts * state->vertex_size, verts);
        state->c_vert = n_verts;
        break;
    case RenderState::__gs_fmt::_static:
        glBufferData(GL_ARRAY_BUFFER, n_verts * state->vertex_size, verts, GL_STATIC_DRAW);
        state->c_vert = n_verts;
        break;
    }
}

void graphics::PushIndicies(void *indicies, size_t n_indicies, bool auto_flush_old) {
    if (!render_precheck(this)) return;

    if (!state->_p_inf._desc.use_indicies) {
        std::cout << "Graphics Error | Cannot push indicies when render state does not use indicies!" << std::endl;
        return;
    }

    if (!indicies || n_indicies == 0) {
        std::cout << "Graphics Warning | Cannot push invalid indicies!" << std::endl;
        return;
    }

    if (n_indicies > state->_p_inf._desc.max_batch_indicies) {
        std::cout << "Graphics Warning | Cannot push " << n_indicies << " indicies at once!\n\tPush geometry in chunks and render each chunk or increase indicie batch size!" << std::endl;
        return;
    }

    if (state->c_ind + n_indicies >= state->_p_inf._desc.max_batch_indicies) {
        if (auto_flush_old) {
            this->RenderFlush(false);
            this->RenderBegin();
            state->cur_shader->usaveRestore();
        } else {
            std::cout << "Graphics Warning | Extra indicies! Max sure to called RenderFlush or enable auto flush!" << std::endl;

            if (state->_p_inf._desc.auto_store_extra)
                _StoreExtraIndicies(indicies, n_indicies * state->vertex_size);
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);

    switch (state->g_fmt) {
    case RenderState::__gs_fmt::_dynamic:
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, state->c_ind * state->indc_sz, n_indicies * state->vertex_size, indicies);
        state->c_ind += n_indicies;
        break;
    case RenderState::__gs_fmt::_static:
        if ((i32) state->suppress < (i32) RenderState::SupressionLevel::NothingWarnings)
            std::cout << "Graphics Warning | PushIndicies is just SetIndicies when dealing with a static context!" << std::endl;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_indicies * state->vertex_size, indicies, GL_STATIC_DRAW);
        state->c_ind = n_indicies;
        break;
    }
}

void graphics::SetIndicies(void *indicies, size_t n_indicies, bool flush_current) {
    if (!render_precheck(this)) return;

    if (!state->_p_inf._desc.use_indicies) {
        std::cout << "Graphics Error | Cannot set indicies when render state does not use indicies!" << std::endl;
        return;
    }

    if (!indicies || n_indicies == 0) {
        std::cout << "Graphics Warning | Cannot set invalid indicies!" << std::endl;
        return;
    }

    if (n_indicies > state->_p_inf._desc.max_batch_indicies) {
        std::cout << "Graphics Warning | Cannot set " << n_indicies << " indicies at once!\n\tPush geometry in chunks and render each chunk or increase indicie batch size!" << std::endl;
        return;
    }

    if (flush_current) {
        this->RenderFlush();
        this->RenderBegin();
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);

    switch (state->g_fmt) {
    case RenderState::__gs_fmt::_dynamic:
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, n_indicies * state->vertex_size, indicies);
        state->c_ind = n_indicies;
        break;
    case RenderState::__gs_fmt::_static:
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_indicies * state->vertex_size, indicies, GL_STATIC_DRAW);
        state->c_ind = n_indicies;
        break;
    }
}

void graphics::RenderFlush(bool qstack_clr) {
    if (!render_precheck(this)) return;

    size_t nPoints = state->c_vert;

    if (state->_p_inf._desc.use_indicies) {
        nPoints = state->c_ind;
    }

    state->cur_shader->use();
    glBindVertexArray(state->vao);
    
    if (state->_p_inf._desc.use_indicies)
        glDrawElements(state->_p_inf._desc.render_primitive, nPoints, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(state->_p_inf._desc.render_primitive, 0, nPoints);

    if (qstack_clr)
        state->cur_shader->clr_qstack();
    
    glBindVertexArray(0);

    state->c_vert = 0;
    state->c_ind = 0;

    state->cur_process = RenderState::Process::None;
}

void graphics::ClearOutput() {
    if (!render_precheck(this)) return;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void graphics::LockState() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot lock invalid state!" << std::endl;
        return;
    }

    if (state->cur_process != RenderState::Process::None) {
        std::cout << "Graphics Error | Cannot lock state whilst it is doing something else :P" << std::endl;
        return;
    }

    state->cur_process = RenderState::Process::Locked;
}

void graphics::UnlockState() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot unlock invalid state!" << std::endl;
        return;
    }

    if (state->cur_process != RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Attempting to unlock state that isn't locked!" << std::endl;
        return;
    }

    state->cur_process = RenderState::Process::None;
}

//frame buffers and stuff
void graphics::SetOutputDevice(OutputDevice* device) {
    if (!device) {
        std::cout << "Graphics Error | Attempting to set output device to a nullptr!" << std::endl;
        return;
    }

    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot set output device to an invalid state!" << std::endl;
        return;
    }

    u32 fbo;
    FrameBuffer *fb;

    switch (device->type) {
    case OutputDevice::FrameBuffer:
        fb = (FrameBuffer*) device->device;

        if (!fb) {
            std::cout << "Graphics Error | Cannot set output device: null frame buffer output device!" << std::endl;
             return;
        }

        fbo = fb->getHandle();

        state->dim.w = fb->w;
        state->dim.h = fb->h;


        std::cout << "setting framebuffer " << fb->w << "x" << fb->h << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo); 
        glViewport(0, 0, fb->w, fb->h);

        if (!fb->texHandle && fb->ty == FrameBuffer::Texture)
            fb->texAttach(fb->w, fb->h, {});
        
        break;
    default:
        std::cout << "Error, cannot bind to unknown output device!" << std::endl;
        return;
    }
}

void graphics::RestoreDefaultOutputDevice() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot change output device of an invalid state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Error | Cannot change output device whilsts render state is locked!" << std::endl;
        return;
    }

    state->oDevice = &_OutputDev_Default;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//constructors
//TODO: incorporate default render state within these johns
graphics::graphics() {
    this->state = new RenderState;
    RenderStateDescriptor def_desc;
    this->_IniCurrentGraphicsState(def_desc);
    this->state->_p_inf._protect.nBindings++;
    this->default_state = this->state;
}

graphics::graphics(RenderStateDescriptor desc) {
    this->state = new RenderState;
    this->default_state = this->state;
    this->_IniCurrentGraphicsState(desc);
    this->state->_p_inf._protect.nBindings++;
}

graphics::graphics(RenderState *def_state) {
    this->default_state = def_state;
    this->state = def_state;
    this->state->_p_inf._protect.nBindings++;
}

u32 graphics::getOutputWidth() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot get width from invalid state!" << std::endl;
        return 0;
    }

    return state->dim.w;
}

u32 graphics::getOutputHeight() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot get height from invalid state!" << std::endl;
        return 0;
    }

    return state->dim.h;
}

void graphics::Resize(u32 w, u32 h) {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot resize invalid state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Error | Cannot resize whilsts render state is locked!" << std::endl;
        return;
    }

    glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_CULL_FACE);  
   // glCullFace(GL_FRONT);

    state->dim.w = w;
    state->dim.h = h;
}

void graphics::SetTesselationVertNum(size_t n_tes) {
    glPatchParameteri(GL_PATCH_VERTICES, n_tes);
}