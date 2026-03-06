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

graphicsState graphics::generic_font_state;

void graphics::Load() {
    //opengl settings
    //glEnable(GL_DEPTH_TEST);
	//glEnable(GL_ALPHA_TEST);
	//glDepthMask(GL_TRUE);
	//glDepthFunc(GL_LEQUAL);

    this->interalState = {
        .g_fmt = graphicsState::__gs_fmt::_dynamic
    };

    this->cur_state = &this->interalState;

    //vertex array
    vao_create(this->cur_state->vao);
    glBindVertexArray(this->cur_state->vao);
    

    //buffer allocation
    //glGenBuffers(1, &this->cur_state->vbo);
    //glBindBuffer(GL_ARRAY_BUFFER, this->cur_state->vbo);

    //define_vattrib_struct(2, Vertex, modColor);
    //define_vattrib_struct(3, Vertex, texId);

    //
}

void graphics::iniStaticGraphicsState() {
    if (!this->cur_state || this->cur_state->g_fmt != graphicsState::__gs_fmt::_null)
        return;
    //vertex array
    vao_create(this->cur_state->vao);

    //buffer allocation
    glGenBuffers(1, &this->cur_state->vbo);

    this->cur_state->g_fmt = graphicsState::__gs_fmt::_static;
}

void graphics::iniDynamicGraphicsState(size_t nBufferVerts) {
    if (!this->cur_state || this->cur_state->g_fmt != graphicsState::__gs_fmt::_null)
        return;

    this->batch_size = nBufferVerts;
    //vertex array
    vao_create(this->cur_state->vao);

    //buffer allocation
    glGenBuffers(1, &this->cur_state->vbo);

    if (this->cur_state->vmem)
        _safe_free_a(this->cur_state->vmem);
    this->cur_state->vmem = nullptr;

    this->cur_state->g_fmt = graphicsState::__gs_fmt::_dynamic;
}

bool graphics::useGraphicsState(graphicsState *gs) {
    if (!this->using_foreign_gs)
        this->interalState.nv = this->_c_vert;
    else if (this->cur_state) {
        this->cur_state->nv = this->_c_vert;
    }

    this->_c_vert = gs->nv;
    this->cur_state = gs;
    this->using_foreign_gs = true;

    if (!this->cur_state->vao || !this->cur_state->vbo)
        return false;

    return true;
}

void graphics::useDefaultGraphicsState() {
    if (this->cur_state) {
        this->cur_state->nv = this->_c_vert;
    }

    this->_c_vert = this->interalState.nv;
    this->cur_state = &this->interalState;
    this->using_foreign_gs = false;
}

#define PROJ_ZMIN  0.1f
#define PROJ_ZMAX  100.0f

//when le window resizes
void graphics::WinResize(const size_t w, const size_t h) {
    this->winW = (f32) w;
    this->winH = (f32) h;

    glViewport(0.0f, 0.0f, this->winW, this->winH);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);  
    glCullFace(GL_FRONT);

    this->proj_matrix = mat4::CreatePersepctiveProjectionMatrix(
        90.0f,
        this->winW / this->winH,
        0.1f, 1000.0f
    );
}

void graphics::push_verts(void *v, size_t n) {
    if (!this->cur_state) {
        std::cout << "Error, no state bound!" << std::endl;
        return;
    }

    if (this->rs_state != ReserveState::None) {
        return;
    }

    if (!v || n == 0)
        return;
    if (!cur_state->vmem) {
        std::cout << "Warning vmem is not allocated! Did you call graphics::Load?" << std::endl;

        if (cur_state->__int_prop.v_obj_sz > 0)
            this->vmem_alloc(this->batch_size * cur_state->__int_prop.v_obj_sz);
        return;
    }

    size_t bsz;
    const size_t vos = this->cur_state->__int_prop.v_obj_sz;

    if ((bsz = (this->_c_vert + n) * vos) > this->batch_size) {
        std::cout << "Warning reached end of allocated gpu memory! " << bsz << " / " << this->batch_size << " | Adding: " << n << " verts!" << std::endl;
        return;
    }

    //std::cout << "Adding: " << n << " verts" << std::endl;

    in_memcpy(
        ((byte*)cur_state->vmem) + (this->_c_vert * vos),
         v, n * vos
    );

    this->_c_vert += n;
}

size_t graphics::vert_space() {
    const size_t vos = this->cur_state->__int_prop.v_obj_sz;
    return (this->batch_size - this->_c_vert * vos) / vos + 1;
}

void graphics::vmem_alloc(size_t sz) {
    if (!cur_state->vbo_alloc) {
        if (!cur_state->vbo) {
            glGenBuffers(1, &cur_state->vbo);
        }
        glBindBuffer(GL_ARRAY_BUFFER, cur_state->vbo);
        gpu_dynamic_alloc(sz);
        cur_state->vbo_alloc = true;
    }

    this->free_state();
    cur_state->vmem = (void*) new byte[sz];

    if (!cur_state->vmem) {
        std::cout << "failed to allocate vmemory!" << std::endl;
    }

    ZeroMem<byte>((byte*)cur_state->vmem, sz);
}

void graphics::vmem_clear() {
    if (!cur_state->vmem) {

        if (cur_state->__int_prop.v_obj_sz > 0)
            this->vmem_alloc(this->batch_size * cur_state->__int_prop.v_obj_sz);
        this->_c_vert = 0;
        return;
    }

    ZeroMem(cur_state->vmem, this->batch_size * cur_state->__int_prop.v_obj_sz);
    this->_c_vert = 0;
}

void graphics::shader_bind() {
    this->shader_bound = true;
    this->s->use();
}

void graphics::shader_unbind() {
    this->shader_bound = false;
    glUseProgram(0);
}

void graphics::render_begin(bool clear) {
    if (this->using_default_device && clear)
        glViewport(0, 0, this->winW, this->winH);
    if (clear)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!cur_state->vmem && cur_state->__int_prop.v_obj_sz > 0)
        this->vmem_alloc(this->batch_size * cur_state->__int_prop.v_obj_sz);
    if (!this->s) return;
    if (!this->shader_bound)
        this->shader_bind();
    
    glBindVertexArray(this->cur_state->vao);
    vbo_bind(this->cur_state->vbo);
}

void graphics::render_flush() {
    if (this->rs_state == ReserveState::Mush) {
        std::cout << "cannot render when mushing!" << std::endl;
        return;
    }

    this->render_noflush();
    this->flush();
}

void graphics::bindMeshToVbo(Mesh *m) {
    if (!this->cur_state)
        return;

    if (this->cur_state->g_fmt != graphicsState::__gs_fmt::_static) {
        std::cout << "Cannot bind mesh to non static graphics state!" << std::endl;
        return;
    }

    this->bind_vao();

    vbo_bind(this->cur_state->vbo);

    //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * m->size(), (void *) m->data());
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m->size(), (void *) m->data(), GL_STATIC_DRAW);

    this->_c_vert = m->size();

    vbo_bind(0);
}

void graphics::render_noflush() {
    if (!this->cur_state)
        return;

    if (this->rs_state == ReserveState::Mush) {
        std::cout << "cannot non mush render when mushing!" << std::endl;
        return;
    }

    //copy over buffer data to gpu memory
    vbo_bind(this->cur_state->vbo);

    const size_t vos = cur_state->__int_prop.v_obj_sz;

    if (cur_state->g_fmt == graphicsState::__gs_fmt::_static) {
        this->s->use();
        return;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, vos * this->_c_vert, (void *) cur_state->vmem);
    }

    //set program variables
    this->bind_vao();
    //this->s->SetMat4("proj_mat", &this->proj_matrix);
    
    glDrawArrays(GL_TRIANGLES, 0, this->_c_vert);
}

void graphics::render_bind() {
    this->bind_vao();
}

void graphics::render_no_geo_update() {
    if (this->rs_state == ReserveState::Mush) {
        std::cout << "cannot no geo render when mushing!" << std::endl;
        return;
    };

    this->s->use();
    this->bind_vao();
    this->s->SetMat4("proj_mat", &this->proj_matrix);

    //std::cout << "Drawing " << this->_c_vert << std::endl;

    glDrawArrays(GL_TRIANGLES, 0, this->_c_vert);
}

void graphics::flush() {
    //if (!this->using_foreign_gs)
    //    this->vmem_clear();

    this->_c_vert = 0;
}

const size_t graphics::getEstimatedMemoryUsage() {
    return sizeof(Vertex) * this->batch_size;
}

void graphics::free_state() {
    _safe_free_a(cur_state->vmem);
    this->_c_vert = 0;
}

//banana
/*void graphics::FillRect(float x, float y, float w, float h) {
    vec2 p1 = vec2(x + w, y);
	vec2 p2 = vec2(x, y);
	vec2 p3 = vec2(x + w, y + h);
	vec2 p4 = vec2(x, y + h);

    Vertex vertices[] = {
		//triangle 1 (0, 1, 3)
		p3.x, p3.y, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		p1.x, p1.y, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		p4.x, p4.y, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

		//triangle 2 (1,2,3)
		p1.x, p1.y, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		p2.x, p2.y, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		p4.x, p4.y, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};

    this->push_verts(vertices, 6);
}*/

void graphics::setCurrentShader(Shader *s) {
    if (s) {
        if (!this->using_foreign_gs)
            this->def_shader = s;
        this->s = s;
    }
    this->shader_bind();
}

Shader* graphics::getCurrentShader() {
    return this->s;
}

void graphics::mesh_single_bind(Mesh *m) {
    if (this->rs_state == ReserveState::MeshBind) {
        return;
    }

    if (this->rs_state == ReserveState::Mush) {
        std::cout << "cannot bind to mesh when mushing!" << std::endl;
        return;
    }

    const Vertex* dat;
    size_t nv;
    if (!m || !(dat = m->data()) || (nv = m->size()) == 0) return;

    //store and swap
    if (!this->rs_state != ReserveState::MeshBind) {
        //this->vstore = this->vmem;
        this->interalState.nv = this->_c_vert;
    }
    this->cur_state->vmem = const_cast<Vertex*>(dat);
    this->_c_vert = nv;

    this->rs_state = ReserveState::MeshBind;
}

void graphics::mesh_unbind() {
    if (this->rs_state == ReserveState::Mush) {
        std::cout << "cannot mesh unbind when mushing!" << std::endl;
        return;
    }

    if (this->rs_state != ReserveState::MeshBind)
        return;

    //swap
    /*if (this->vstore) {
        this->vmem = this->vstore;
        this->_c_vert = this->interalState.nv;
        this->vstore = nullptr;
    }*/

    this->_c_vert = this->interalState.nv;

    this->rs_state = ReserveState::None;
}

void graphics::mush_begin() {
    if (!this->cur_state)
        return;

    if (this->rs_state != ReserveState::None) {
        //todo: say something about this
        return;
    }

    this->mesh_unbind();
    this->rs_state = ReserveState::Mush;
    this->mush_offset = 0;
    vbo_bind(this->cur_state->vbo);
}

void graphics::mush_render(Mesh *m) {
    if (!m)
        return;

    if (!this->rs_state != ReserveState::Mush) {
        std::cout << "Cannot mush render since mushing has not began!" << std::endl;
        return;
    }

    const size_t nv = m->size(), nv_bytes = nv * sizeof(Vertex);

    if ((this->mush_offset + nv_bytes) > this->batch_size) {
        //set program variables
        this->s->SetMat4("proj_mat", &this->proj_matrix);
        this->bind_vao();

        glDrawArrays(GL_TRIANGLES, 0, this->_c_vert);

        this->mush_offset = 0;
        this->_c_vert = 0;
    }

    glBufferSubData(GL_ARRAY_BUFFER, this->mush_offset, nv_bytes, (void *) m->data());
    this->_c_vert += nv;
    this->mush_offset += nv_bytes;
}

void graphics::mush_end() {
    if (this->rs_state != ReserveState::Mush) return;

    //final render
    this->s->SetMat4("proj_mat", &this->proj_matrix);
    this->bind_vao();

    glDrawArrays(GL_TRIANGLES, 0, this->_c_vert);
    //////////////


    this->rs_state = ReserveState::None;
    vbo_bind(0);
    this->flush();
}

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
    
void FrameBuffer::texAttach(u32 w, u32 h) {
    this->delete_tex();

    glGenTextures(1, &this->texHandle);

    if (!this->texHandle) {
        std::cout << "Failed to generate framebuffer rgb texture!" << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, this->texHandle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
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

FrameBuffer::FrameBuffer(FrameBuffer::fb_type ty, u32 w, u32 h) {
    this->ty = ty;
    glGenFramebuffers(1, &this->handle);

    switch (ty) {
    case FrameBuffer::Texture:
        this->texAttach(w, h);
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

void graphics::setOutputDevice(OutputDevice *device) {
    if (!device) return;

    u32 fbo;
    FrameBuffer *fb;

    switch (device->type) {
    case OutputDevice::FrameBuffer:
        fb = (FrameBuffer*) device->device;

        if (!fb) {
            std::cout << "Error, null frame buffer output device!" << std::endl;
             return;
        }

        fbo = fb->getHandle();

        std::cout << "bound to fbo: " << fbo << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo); 
        glViewport(0, 0, fb->w, fb->h);

        if (!fb->texHandle && fb->ty == FrameBuffer::Texture)
            fb->texAttach(fb->w, fb->h);

        this->using_default_device = false;
        break;
    default:
        std::cout << "Error, cannot bind to unknown output device!" << std::endl;
        return;
    }
}

void graphics::restoreDefaultOutputDevice() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, this->winW, this->winH);
    this->using_default_device = true;
}

void graphics::vertexStructureDefineBegin(size_t vObjSz) {
    if (!this->cur_state) {
        std::cout << "Failed to start vertex struct definitions!" << std::endl;
        return;
    }

    glBindVertexArray(this->cur_state->vao);
    vbo_bind(this->cur_state->vbo);
    this->cur_state->__int_prop.v_obj_sz = vObjSz;
}

void graphics::vertexStructureDefineEnd() {
    //ok
}

void graphics::defineVertexPart(i32 n, __mu_glVInf inf) {
    glVertexAttribPointer(n, inf.p_sz / sizeof(f32), GL_FLOAT, GL_FALSE, inf.sz, (void*)inf.off);
    glEnableVertexAttribArray(n);
}

void graphics::defineIntegerVertexPart(i32 n, __mu_glVInf inf) {
    glVertexAttribIPointer(n, inf.p_sz / sizeof(i32), GL_INT, inf.sz, (void*)inf.off);
    glEnableVertexAttribArray(n); 
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

    bmp.header.bitsPerPixel = 24;
    bmp.header.w = this->w;
    bmp.header.h = this->h;

    glReadPixels(0, 0, this->w, this->h, GL_RGB, GL_UNSIGNED_BYTE, bmp.data);

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


























void graphics2::_IniCurrentGraphicsState(RenderStateDescriptor desc) {
    if (!state) {
        std::cout << "Graphics Warning | Failed to configure render state: current render state is a nullptr! \n\tMake sure you correctly created the currently bound render state." << std::endl;
        return;
    }

    if (!state->_p_inf._null) {
        std::cout << "Graphics Warning | Failed to configure render state: cannot configure null render state! \n\tMake sure you have set a valid render state and are not using a deleted state." << std::endl;
        return;
    }

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

RenderState graphics2::CreateBlankRenderState() {
    return {}; //most underwelming function ever
}

void graphics2::SetRenderState(RenderState *s) {
    if (!s) {
        std::cout << "Graphics Warning | Failed to set render state: current render state is a nullptr! \n\tMake sure you correctly created the currently bound render state." << std::endl;
        return;
    }

    if (!s->_p_inf._null) {
        std::cout << "Graphics Warning | Failed to set render state: cannot configure null render state! \n\tMake sure you have set a valid render state and are not using a deleted state." << std::endl;
        return;
    }

    prev_state = state;
    state = s;
}

RenderState *graphics2::GetCurrentRenderState() {
    return state;
}

void graphics2::RestoreLastRenderState() {
    RenderState *temp = prev_state;
    prev_state = state;
    state = temp;
}

void graphics2::RestoreDefaultRenderState() {
    prev_state = state;
    state = &this->default_state;
}

void graphics2::VertexDefineBegin(size_t v_obj_sz) {
    if (!state) {
        std::cout << "Graphics Warning | Failed to begin vertex define: current render state is a nullptr! \n\tMake sure you correctly created the currently bound render state." << std::endl;
        return;
    }

    if (!state->_p_inf._null || !state->_p_inf._ini || !state->vbo) {
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

    state->vertex_size = v_obj_sz;
    state->cur_process = RenderState::Process::VertexDefine;
}

void graphics2::DefineVertexPart(i32 part_index, __mu_glVInf inf) {
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

void graphics2::DefineIntegerVertexPart(i32 part_index, __mu_glVInf inf) {
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

void graphics2::VertexDefineEnd() {
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
}

void graphics2::DeleteRenderState(RenderState *state) {
    if (!state)
        return;

    if (state->cur_process == RenderState::Process::Locked)
        std::cout << "Graphics Warning | Deleting locked render state!" << std::endl;

    if (state->vao) glDeleteVertexArrays(1, &state->vao);
    if (state->vbo) glDeleteBuffers(1, &state->vbo);
    if (state->ibo) glDeleteBuffers(1, &state->ibo);

    ZeroMem(state, sizeof(RenderState));

    //ensure that if this state is bound it will properly trigger an error somewhere :)
    state->_p_inf._null = true;
    state->_p_inf._ini = false;
}

//render functions
void graphics2::SetShader(Shader *shader) {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to set shader: invalid render state!" << std::endl;
        return;
    }

    if (state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot set shader whilsts graphics state is locked!" << std::endl;
        return;
    }

    if (!shader || !shader->good()) {
        std::cout << "Graphics Warning | Failed to set shader: bad shader!" << std::endl;
        return;
    }

    shader->use(); //ok now use le shader
    state->cur_shader = shader;
}

Shader* graphics2::GetCurrentShader() {
    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to get shader: invalid render state!" << std::endl;
        return nullptr;
    }

    return state->cur_shader;
}

bool render_precheck(graphics2 *g) {
    if (!g->state || g->state->_p_inf._null || !g->state->_p_inf._ini) {
        std::cout << "Graphics Error | Failed to begin render: invalid render state!" << std::endl;
        return false;
    }

    if (g->state->cur_process == RenderState::Process::Locked) {
        std::cout << "Graphics Warning | Cannot render whilsts process is locked!" << std::endl;
        return false;
    }

    if (g->state->cur_process != RenderState::Process::None) {
        std::cout << "Graphics Warning | Cannot render whilsts state is busy with something else!" << std::endl;
        return false;
    }

    return true;
}

void graphics2::RenderBegin() {
    if (!render_precheck(this)) return;

    glBindVertexArray(state->vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    state->c_vert = 0;
    state->cur_process = RenderState::Process::Render;
}

//TODO: all these actually important functions
void graphics2::PushVerts(void *verts, size_t n_verts, bool auto_flush) {
    if (!verts || n_verts == 0) {
        std::cout << "Graphics Warning | Cannot push invalid verts!" << std::endl;
        return;
    }

    if (!render_precheck(this)) return;

    if (auto_flush && state->c_vert > state->_p_inf._desc.max_batch_verts) {
        this->RenderFlush(false);
    }

    glBindBuffer(GL_ARRAY_BUFFER, state->vao);

    switch (state->g_fmt) {
    case RenderState::__gs_fmt::_dynamic:
        glBufferSubData(GL_ARRAY_BUFFER, state->c_vert * state->vertex_size, n_verts * state->vertex_size, verts);
        state->c_vert += n_verts;
        break;
    case RenderState::__gs_fmt::_static:
        if ((i32) state->suppress < (i32) RenderState::SupressionLevel::NothingWarnings)
            std::cout << "Graphics Warning | Push verts is just set verts when dealing with a static context!" << std::endl;
        glBufferData(GL_ARRAY_BUFFER, state->c_vert * state->vertex_size, verts, GL_STATIC_DRAW);
        if (auto_flush)
            this->RenderFlush(false);
        break;
    }
}

void graphics2::SetVerts(void *verts, size_t n_verts, bool flush_current) {
    if (!verts || n_verts == 0) {
        std::cout << "Graphics Warning | Cannot set invalid verts!" << std::endl;
        return;
    }

    if (!render_precheck(this)) return;

    if (flush_current) {
        this->RenderFlush(false);
    }

    
}

void graphics2::PushIndicies(void *indicies, size_t n_indicies) {
    if (!render_precheck(this)) return;

    if (!indicies || n_indicies == 0) {
        std::cout << "Graphics Warning | Cannot push invalid indicies!" << std::endl;
        return;
    }
}

void graphics2::SetIndicies(void *indicies, size_t n_indicies) {
    if (!render_precheck(this)) return;

    if (!indicies || n_indicies == 0) {
        std::cout << "Graphics Warning | Cannot set invalid indicies!" << std::endl;
        return;
    }
}

void graphics2::RenderFlush(bool clear_buffer = true) {
    if (!render_precheck(this)) return;


}

void graphics2::LockState() {
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

void graphics2::UnlockState() {
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
void graphics2::SetOutputDevice(OutputDevice* device) {
    if (!device) {
        std::cout << "Graphics Error | Attempting to set output device to a nullptr!" << std::endl;
        return;
    }

    if (!state || state->_p_inf._null || !state->_p_inf._ini) {
        std::cout << "Graphics Error | Cannot set output device to an invalid state!" << std::endl;
        return;
    }

    
}

void graphics2::RestoreDefaultOutputDevice() {

}

//constructors
//TODO: incorporate default render state within these johns
graphics2::graphics2() {

}
graphics2::graphics2(RenderStateDescriptor desc) {

}

graphics2::graphics2(RenderState *def_state) {

}