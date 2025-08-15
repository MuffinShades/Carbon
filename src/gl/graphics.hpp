#pragma once
#include <iostream>
#include "window.hpp"
#include "Shader.hpp"
#include "mesh.hpp"
#include "vertex.hpp"

class graphics {
private:
    Window *win = nullptr;
    mat4 proj_matrix;
    size_t _c_vert = 0, nv_store = 0;
    Vertex *vmem = nullptr, *vstore = nullptr;
    bool mesh_bound = false;
    void vmem_alloc();
    void vmem_clear();
    void bind_vao() {
        if (this->vao != 0)
            glBindVertexArray(this->vao);
        else
            std::cout << "Failed to bind to vao!" << std::endl;
    }
    Shader *s = nullptr;
public:
    f32 winW, winH;
    u32 vao = 0, vbo = 0;
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
    void flush();
    void push_verts(Vertex *v, size_t n);
    void mesh_bind(Mesh* m);
    void mesh_unbind();
    void setCurrentShader(Shader *s);
    Shader* getCurrentShader();
    //void DrawImage();
    //void FillRect(float x, float y, float w, float h);
    void free();
    const size_t getEstimatedMemoryUsage();
};