#ifndef CARBON_GAME_MAIN_HPP
#define CARBON_GAME_MAIN_HPP
#include <iostream>
#include "../msutil.hpp"
#include "../gl/window.hpp"
#include "assetManager.hpp"
#include "../gl/graphics.hpp"
#include "../gl/mesh.hpp"
#include "../gl/Camera.hpp"
#include "../gl/geometry/cube.hpp"
#include "../gl/Texture.hpp"
#include "player.hpp"
#include "game.hpp"
#include "../gl/atlas.hpp"
#include "world.hpp"

f64 mxp = 0.0, myp = 0.0;

Player p = Player({0.0f,0.0f,0.0f});

void mouse_callback(GLFWwindow* win, f64 xp, f64 yp) {
    if (!win) return;

    f64 dx = xp - mxp, dy = yp - myp;

    p.mouseUpdate(dx, dy);

    mxp = xp;
    myp = yp;
}

bool mouseEnabled = false;

void mouseEnableUpdate(GLFWwindow* win) {
    if (!win) return;
    if (mouseEnabled)
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);  
    else
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
}

void kb_callback(GLFWwindow* win, i32 key, i32 scancode, i32 action, i32 mods) {
    if (!win) return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS) {
            mouseEnabled = !mouseEnabled;
            mouseEnableUpdate(win);
        }
        break;
    }
}

//Camera cam = Camera({0.0f, 1.0f, -1.0f});

constexpr size_t WIN_W = 900, WIN_H = 750;

mat4 lookMat;

graphics *g = nullptr;
BindableTexture *tex = nullptr;
TexAtlas atlas;
Shader s;

Mesh m;
mat4 mm = mat4(1.0f);

World w = World(69);
Chunk testChunk;

/*

TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##

Make mushing faster

When generating the world combine the chunk meshes into bigger meshes that are less likely
to be modified. Be smarter with memory and maybe use some sort of stack system when creating
chunks to try and stitch as much as the world together within a single mesh opposed to one
mesh per chunk which leads to HUGE performance dips. (2,000fps -> <70fps)

TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##TODO##

*/

void render() {
    g->render_begin();
    //g->mush_begin();

    //mm = mat4::Rotate(mm, 0.001f, {1.0f, 2.0f, 3.0f});

    lookMat = p.getCam()->getLookMatrix();

    vec3 camPos = p.getCam()->getPos();

    s.SetMat4("cam_mat", &lookMat);
    s.SetMat4("model_mat", &testChunk.modelMat);
    s.SetVec3("cam_pos", &camPos);

    //g->render_no_geo_update();

    //w.render(g, p.getCam());

    g->bindMeshToVbo(&m);
    g->render_flush();

    //g->mush_end();
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
    mouseEnableUpdate(win.wHandle);

    g = new graphics(&win);

    g->Load();

    g->vertexStructureDefineBegin(sizeof(Vertex));
    g->defineVertexPart(0, vertexClassPart(Vertex, posf));
    g->defineVertexPart(1, vertexClassPart(Vertex, n));
    g->defineVertexPart(2, vertexClassPart(Vertex, tex));
    g->vertexStructureDefineEnd();

    s = Shader::LoadShaderFromFile("src/shaders/def_vert.glsl", "src/shaders/def_frag.glsl");

    tex = new BindableTexture("assets/vox/rocc2.png");
    atlas = TexAtlas(tex->width(), tex->height(), 225, 225);

    //w = World(69);
    w.SetAtlas(&atlas);
    //testChunk = w.genChunk(vec3(3, 0, 3));

    //BindableTexture tex = BindableTexture("moop.pak", "Global.Globe.Map", "Global.Vox.Tex.atlas");

    TexPart clip = atlas.getImageIndexPart(8, 11); //command block
    m.setMeshData((Vertex*) Geo::Cube::GenFace(CubeFace::North, vec4(clip.tl.x, clip.tl.y, clip.br.x - clip.tl.x, clip.br.y - clip.tl.y)), 6, true);

    /*Shader s = Shader::LoadShaderFromResource(
        "moop.pak", 
        "Global.Globe.Map", 
        "Global.Graphics.Shaders.Core.Vert", 
        "Global.Graphics.Shaders.Core.Frag"
    );*/

    g->setCurrentShader(&s);

    mm = mat4::CreateTranslationMatrix({0.0f, 0.0f, -3.0f});

    g->WinResize(WIN_W,WIN_H);

    tex->bind();

    g->push_verts((Vertex*)testChunk.mesh.data(), testChunk.mesh.size());
    //g->render_flush();

    g->render_noflush();

    glfwSwapInterval(0);

    //w.chunkBufIni();
    std::cout << "GPTR: " << (uintptr_t)g << std::endl;
    //w.genChunks(g);

    while (win.isRunning()) {
        glClearColor(0.2, 0.7, 1.0, 1.0);

        //tick
        p.tick(&win);

        //render
        render();

        win.Update();
    }

    m.free();
    //g->free();
    _safe_free_b(g);
    _safe_free_b(tex);
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP