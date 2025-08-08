#ifndef CARBON_GAME_MAIN_HPP
#define CARBON_GAME_MAIN_HPP
#include <iostream>
#include "../msutil.hpp"
#include "../gl/window.hpp"
#include "assetManager.hpp"
#include "../gl/graphics.hpp"
#include "../gl/mesh.hpp"
#include "../gl/Camera.hpp"
#include "cube.hpp"
#include "../gl/Texture.hpp"
#include "player.hpp"
#include "game.hpp"
#include "../gl/atlas.hpp"

f64 mxp = 0.0, myp = 0.0;

Player p = Player({0.0f,0.0f,0.0f});

void mouse_callback(GLFWwindow* win, f64 xp, f64 yp) {
    if (!win) return;

    f64 dx = xp - mxp, dy = yp - myp;

    p.mouseUpdate(dx, dy);

    mxp = xp;
    myp = yp;
}


void kb_callback(GLFWwindow* win, i32 key, i32 scancode, i32 action, i32 mods) {
    if (!win) return;

}

//Camera cam = Camera({0.0f, 1.0f, -1.0f});

constexpr size_t WIN_W = 900, WIN_H = 750;

mat4 lookMat;

graphics *g = nullptr;
BindableTexture *tex = nullptr;
TexAtlas atlas;
Shader s = Shader::LoadShaderFromFile("src/shaders/def_vert.glsl", "src/shaders/def_frag.glsl");

Mesh m;
mat4 mm = mat4(1.0f);

void render() {
    g->render_begin();

    tex->bind();

    mm = mat4::Rotate(mm, 0.01f, {1.0f, 2.0f, 3.0f});

    lookMat = p.getCam()->getLookMatrix();

    s.SetMat4("cam_mat", &lookMat);
    s.SetMat4("model_mat", &mm);

    g->push_verts((Vertex*)m.data(), m.size());
    g->render_flush();
}

extern i32 game_main() {
    Path::SetProjectDir(PROJECT_DIR);

    //std::cout << "Compiling Assets..." << std::endl;
    //i32 code = AssetManager::compileDat("assets/global_map.json", "compass.pak");
    //std::cout << "Asset Compliation exited with code: " << code << std::endl;

    Window::winIni();
    Window win = Window(":D", WIN_W, WIN_H);

    glfwSetCursorPosCallback(win.wHandle, mouse_callback);
    glfwSetKeyCallback(win.wHandle, kb_callback);

    g = new graphics(&win);

    g->Load();

    tex = new BindableTexture("assets/vox/alphaTextureMC.png");
    atlas = TexAtlas(tex->width(), tex->height(), 16, 16);

    //BindableTexture tex = BindableTexture("moop.pak", "Global.Globe.Map", "Global.Vox.Tex.atlas");

    m.setMeshData((Vertex*) Cube::GetFace(CubeFace::North), 6);

    /*Shader s = Shader::LoadShaderFromResource(
        "moop.pak", 
        "Global.Globe.Map", 
        "Global.Graphics.Shaders.Core.Vert", 
        "Global.Graphics.Shaders.Core.Frag"
    );*/

    g->setCurrentShader(&s);

    mm = mat4::CreateTranslationMatrix({0.0f, 0.0f, -3.0f});

    g->WinResize(WIN_W,WIN_H);

    while (win.isRunning()) {
        glClearColor(0.2, 0.7, 1.0, 1.0);

        //tick
        p.tick(&win);

        //render
        render();

        win.Update();
    }

    m.free();
    g->free();
    _safe_free_b(g);
    _safe_free_b(tex);
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP