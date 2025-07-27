#pragma once
#include <iostream>
#include "window.hpp"
#include "Shader.hpp"

struct Vertex {
    float posf[3];
    float tex[2];
};

class graphics {
private:
    Window *win = nullptr;
    mat4 proj_matrix;
    size_t _c_vert = 0;
    Vertex *vmem = nullptr;
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
    float winW, winH;
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
    void render_flush();
    void push_verts(Vertex *v, size_t n);
    void setCurrentShader(Shader *s);
    Shader* getCurrentShader();
    //void DrawImage();
    //void FillRect(float x, float y, float w, float h);
    void free();
    const size_t getEstimatedMemoryUsage();
};