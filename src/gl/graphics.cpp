#include "graphics.hpp"
#include "../game/assetManager.hpp"
#include <glad.h>

#define BYTE_TO_CONST_CHAR(b) const_cast<const char*>(reinterpret_cast<char*>((b)))
#define gpu_dynamic_alloc(sz) glBufferData(GL_ARRAY_BUFFER, sz, nullptr, GL_DYNAMIC_DRAW)

#define BATCH_NQUADS 0xffff
#define BATCH_SIZE 0x8fffff

#define vertex_float_attrib(i, s, sz, off) glVertexAttribPointer(i, s, GL_FLOAT, GL_FALSE, sz, (void *)off); \
    glEnableVertexAttribArray(i)

#define FL_SZ 4
#define define_vattrib_struct(i, type, target) \
    glEnableVertexAttribArray(i); \
    glVertexAttribPointer(i, sizeof(type::target) / FL_SZ, GL_FLOAT, GL_FALSE, sizeof(type), (void *)offsetof(type, target))

#define vao_create(vao) glGenVertexArrays(1, &vao)

#define vbo_bind(vbo) glBindBuffer(GL_ARRAY_BUFFER, (vbo))

void graphics::Load() {
    //opengl settings
    //glEnable(GL_DEPTH_TEST);
	//glEnable(GL_ALPHA_TEST);
	//glDepthMask(GL_TRUE);
	//glDepthFunc(GL_LEQUAL);

    //vertex array
    vao_create(this->core_vao);
    glBindVertexArray(core_vao);
    this->vao = this->core_vao;

    //buffer allocation
    glGenBuffers(1, &this->core_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->core_vbo);
    gpu_dynamic_alloc(BATCH_SIZE * sizeof(Vertex));
    this->vbo = this->core_vbo;
    this->vmem_alloc();

    define_vattrib_struct(0, Vertex, posf);
    define_vattrib_struct(1, Vertex, n);
    define_vattrib_struct(2, Vertex, tex);

    //define_vattrib_struct(2, Vertex, modColor);
    //define_vattrib_struct(3, Vertex, texId);

    //
}

void graphics::iniStaticGraphicsState() {
    if (!this->cgs || this->cgs->g_fmt != graphicsState::__gs_fmt::_null)
        return;
    //vertex array
    vao_create(this->vao);
    glBindVertexArray(vao);

    //buffer allocation
    glGenBuffers(1, &this->vbo);
    vbo_bind(this->vbo);

    define_vattrib_struct(0, Vertex, posf);
    define_vattrib_struct(1, Vertex, n);
    define_vattrib_struct(2, Vertex, tex);

    this->cgs->vao = this->vao;
    this->cgs->vbo = this->vbo;

    this->cgs->g_fmt = graphicsState::__gs_fmt::_static;
}

bool graphics::useGraphicsState(graphicsState *gs) {
    if (!this->using_gs)
        this->nv_store = this->_c_vert;
    else if (this->cgs) {
        this->cgs->vao = this->vao;
        this->cgs->vbo = this->vbo;
        this->cgs->nv = this->_c_vert;
    }

    this->vao = gs->vao;
    this->vbo = gs->vbo;
    this->_c_vert = gs->nv;
    this->cgs = gs;
    this->using_gs = true;

    if (!this->vao || !this->vbo)
        return false;

    return true;
}

void graphics::useDefaultGraphicsState() {
    if (this->cgs) {
        this->cgs->vao = this->vao;
        this->cgs->vbo = this->vbo;
        this->cgs->nv = this->_c_vert;
    }

    this->vao = this->core_vao;
    this->vbo = this->core_vbo;
    this->_c_vert = this->nv_store;
    this->cgs = nullptr;
    this->using_gs = false;
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

void graphics::push_verts(Vertex *v, size_t n) {
    if (this->mushing) {
        std::cout << "Error, cannot render when mushing!" << std::endl;
        return;
    }

    if (this->mesh_bound) {
        std::cout << "Error, cannot render when mesh is bound!" << std::endl;
        return;
    }

    if (!v || n == 0)
        return;
    if (!this->vmem) {
        std::cout << "Warning vmem is not allocated! Did you call graphics::Load?" << std::endl;
        this->vmem_alloc();
        return;
    }

    size_t bsz;

    if ((bsz = (this->_c_vert + n) * sizeof(Vertex)) > BATCH_SIZE) {
        std::cout << "Warning reached end of allocated gpu memory! " << bsz << " / " << BATCH_SIZE << " | Adding: " << n << " verts!" << std::endl;
        return;
    }

    in_memcpy(this->vmem + this->_c_vert * sizeof(Vertex), v, n * sizeof(Vertex));
    this->_c_vert += n;
}

void graphics::vmem_alloc() {
    this->free();
    this->vmem = new Vertex[BATCH_SIZE];
    ZeroMem(this->vmem, BATCH_SIZE);
}

void graphics::vmem_clear() {
    ZeroMem(this->vmem, BATCH_SIZE);
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

void graphics::render_begin() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!this->s) return;
    if (!this->shader_bound)
        this->shader_bind();
}

void graphics::render_flush() {
    if (this->mushing) {
        std::cout << "cannot render when mushing!" << std::endl;
        return;
    }

    this->render_noflush();
    this->flush();
}

void graphics::bindMeshToVbo(Mesh *m) {
    if (!this->cgs)
        return;

    if (this->cgs->g_fmt != graphicsState::__gs_fmt::_static) {
        std::cout << "Cannot bind mesh to non static graphics state!" << std::endl;
        return;
    }

    this->bind_vao();

    vbo_bind(this->vbo);

    //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * m->size(), (void *) m->data());
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m->size(), (void *) m->data(), GL_STATIC_DRAW);

    this->_c_vert = m->size();

    vbo_bind(0);
}

void graphics::render_noflush() {
    if (this->mushing) {
        std::cout << "cannot non mush render when mushing!" << std::endl;
        return;
    }

    //copy over buffer data to gpu memory
    vbo_bind(this->vbo);

    if (this->cgs) {
        if (this->cgs->g_fmt == graphicsState::__gs_fmt::_static) {
            this->s->use();
            return;
        }
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * this->_c_vert, (void *) this->vmem);
    }

    //set program variables
    this->bind_vao();
    this->s->SetMat4("proj_mat", &this->proj_matrix);
    
    glDrawArrays(GL_TRIANGLES, 0, this->_c_vert);
}

void graphics::render_bind() {
    this->bind_vao();
}

void graphics::render_no_geo_update() {
    if (this->mushing) {
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
    if (!this->using_gs)
        this->vmem_clear();
}

const size_t graphics::getEstimatedMemoryUsage() {
    return sizeof(Vertex) * BATCH_SIZE;
}

void graphics::free() {
    _safe_free_a(this->vmem);
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
        if (!this->using_gs)
            this->def_shader = s;
        this->s = s;
    }
    this->shader_bind();
}

Shader* graphics::getCurrentShader() {
    return this->s;
}

void graphics::mesh_single_bind(Mesh *m) {
    if (this->mushing) {
        std::cout << "cannot bind to mesh when mushing!" << std::endl;
        return;
    }

    const Vertex* dat;
    size_t nv;
    if (!m || !(dat = m->data()) || (nv = m->size()) == 0) return;

    //store and swap
    if (!this->mesh_bound) {
        this->vstore = this->vmem;
        this->nv_store = this->_c_vert;
    }
    this->vmem = const_cast<Vertex*>(dat);
    this->_c_vert = nv;

    this->mesh_bound = true;
}

void graphics::mesh_unbind() {
    if (this->mushing) {
        std::cout << "cannot mesh unbind when mushing!" << std::endl;
        return;
    }

    //swap
    if (this->vstore) {
        this->vmem = this->vstore;
        this->_c_vert = this->nv_store;
        this->vstore = nullptr;
    }

    this->mesh_bound = false;
}

void graphics::mush_begin() {
    this->mesh_unbind();
    this->mushing = true;
    this->mush_offset = 0;
    vbo_bind(this->vbo);
}

void graphics::mush_render(Mesh *m) {
    if (!m)
        return;

    if (!this->mushing) {
        std::cout << "Cannot mush render since mushing has not began!" << std::endl;
        return;
    }

    const size_t nv = m->size(), nv_bytes = nv * sizeof(Vertex);

    if ((this->mush_offset + nv_bytes) > BATCH_SIZE) {
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
    if (!this->mushing) return;

    //final render
    this->s->SetMat4("proj_mat", &this->proj_matrix);
    this->bind_vao();

    glDrawArrays(GL_TRIANGLES, 0, this->_c_vert);
    //////////////


    this->mushing = false;
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void graphics::setCurrentFrameBuffer(FrameBuffer *fb) {
    if (!fb) return;

    const u32 fbo = fb->getHandle();

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}