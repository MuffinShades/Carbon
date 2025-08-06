#include "graphics.hpp"
#include "../game/assetManager.hpp"
#include <glad.h>

#define BYTE_TO_CONST_CHAR(b) const_cast<const char*>(reinterpret_cast<char*>((b)))
#define gpu_dynamic_alloc(sz) glBufferData(GL_ARRAY_BUFFER, sz, nullptr, GL_DYNAMIC_DRAW)

#define BATCH_NQUADS 0xffff
#define BATCH_SIZE BATCH_NQUADS * 6

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
    GLuint* vptr = &this->vao;
    vao_create(this->vao);
    glBindVertexArray(vao);

    //buffer allocation
    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    gpu_dynamic_alloc(BATCH_SIZE * sizeof(Vertex));
    this->vmem_alloc();

    define_vattrib_struct(0, Vertex, posf);
    define_vattrib_struct(1, Vertex, tex);
    //define_vattrib_struct(2, Vertex, modColor);
    //define_vattrib_struct(3, Vertex, texId);

    //
}

#define PROJ_ZMIN  0.1f
#define PROJ_ZMAX  100.0f

//when le window resizes
void graphics::WinResize(const size_t w, const size_t h) {
    this->winW = (f32) w;
    this->winH = (f32) h;

    glViewport(0.0f, 0.0f, this->winW, this->winH);

    //if (this->proj_matrix != nullptr && this->proj_matrix->m != nullptr)
    //    this->proj_matrix->attemptFree();

    /*this->proj_matrix = mat4::CreateOrthoProjectionMatrix(
        this->winW, 
        0.0f, 
        this->winH, 
        0.0f, 
        PROJ_ZMIN, 
        PROJ_ZMAX
    );*/

    this->proj_matrix = mat4::CreatePersepctiveProjectionMatrix(
        90.0f,
        this->winW / this->winH,
        0.1f, 100.0f
    );

    forrange(16) std::cout << this->proj_matrix.m[i] << std::endl;
}

void graphics::push_verts(Vertex *v, size_t n) {
    if (!v || n == 0)
        return;
    if (!this->vmem) {
        std::cout << "Warning vmem is not allocated! Did you call graphics::Load?" << std::endl;
        this->vmem_alloc();
        return;
    }

    if ((this->_c_vert + n) * sizeof(Vertex) > BATCH_SIZE) {
        std::cout << "Warning reached end of allocated gpu memory!" << std::endl;
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

void graphics::render_begin() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);

    if (!this->s) return;
    this->s->use();
}

void graphics::render_flush() {
    //copy over buffer data to gpu memory
    vbo_bind(this->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * this->_c_vert, (void *) this->vmem);

    //set program variables
    this->s->SetMat4("proj_mat", &this->proj_matrix);

    this->bind_vao();
    

    vbo_bind(0);
    glDrawArrays(GL_TRIANGLES, 0, this->_c_vert);
    this->vmem_clear();
}

const size_t graphics::getEstimatedMemoryUsage() {
    return sizeof(Vertex) * BATCH_SIZE;
}

void graphics::free() {
    _safe_free_a(this->vmem);
    this->_c_vert = 0;
}

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
    if (s) this->s = s;
}

Shader* graphics::getCurrentShader() {
    return this->s;
}