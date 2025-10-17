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
#include "../phy/RigidBody3.hpp"

//player
Player p = Player({0.0f,0.0f,-1.0f});

//input code
f64 mxp = 0.0, myp = 0.0;

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

//Physics Scene
RigidBody3 body1 = RigidBody3(rb_simple_type::cuboid, vec3(1.0, 1.0, 1.0), 1.0f, Material::Plastic);
RBodyScene3 scene;

void kb_callback(GLFWwindow* win, i32 key, i32 scancode, i32 action, i32 mods) {
    if (!win) return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS) {
            mouseEnabled = !mouseEnabled;
            mouseEnableUpdate(win);
        }
        break;
    case GLFW_KEY_Q:
        if (action == GLFW_PRESS) {
            std::cout << "!!!" << std::endl;
            //scene.setGravity(-9.8f * 0.01f);
            Force F = {
                .pos = vec3(0.0f, 0.1f, 0.0f),
                .F = vec3(2.0f, 2.0f, 2.0f)
            };
            body1.addForce(F);
        }
        break;
    }
}

constexpr size_t WIN_W = 900, WIN_H = 750;

graphics *g = nullptr;
BindableTexture *tex = nullptr;
TexAtlas atlas;
Shader s;

mat4 lookMat;

void scene_setup() {
    scene.addBody(&body1);
}

void render() {
    g->render_begin();

    lookMat = p.getCam()->getLookMatrix();

    vec3 camPos = p.getCam()->getPos();

    s.SetMat4("cam_mat", &lookMat);
    s.SetVec3("cam_pos", &camPos);

    scene.render(g, (Camera*) p.getCam());
}

extern i32 game_main() {
    Path::SetProjectDir(PROJECT_DIR);

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

    //shader setup
    s = Shader::LoadShaderFromFile("src/shaders/def_vert.glsl", "src/shaders/def_frag.glsl");
    g->setCurrentShader(&s);

    //texture setup
    tex = new BindableTexture("assets/vox/rocc2.png");
    atlas = TexAtlas(tex->width(), tex->height(), 225, 225);

    TexPart clip = atlas.getImageIndexPart(8, 11); //command block

    //tex->bind();

    //setup the physics scene
    scene_setup();
    
    //window resize / seutp
    g->WinResize(WIN_W,WIN_H);
    glfwSwapInterval(0);

    f32 lastFrame = 0,t,dt;

    while (win.isRunning()) {
        glClearColor(0.2, 0.7, 1.0, 1.0);

        t = glfwGetTime();
        dt = t - lastFrame;
        lastFrame = t;

        //tick
        p.tick(&win, dt);
        scene.tick(dt);

        //render
        render();

        win.Update();
    }

    _safe_free_b(g);
    _safe_free_b(tex);
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP