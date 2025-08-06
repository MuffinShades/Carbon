#ifndef CARBON_GAME_MAIN_HPP
#define CARBON_GAME_MAIN_HPP
#include <iostream>
#include "../msutil.hpp"
#include "../gl/window.hpp"
#include "assetManager.hpp"
#include "../gl/graphics.hpp"
#include "../gl/mesh.hpp"
#include "../gl/Camera.hpp"

constexpr Vertex testFace[] = {
        //NORTH
0, 0, 1,  0.0f, 1.0f,
1, 1, 1,  1.0f, 0.0f,
1, 0, 1,  1.0f, 1.0f,

1, 1, 1,  1.0f, 0.0f,
0, 0, 1,  0.0f, 1.0f,
0, 1, 1,  0.0f, 0.0f,

//SOUTH
0, 0, 0,  1.0f, 1.0f,
1, 0, 0,  0.0f, 1.0f,
1, 1, 0,  0.0f, 0.0f,

1, 1, 0,  0.0f, 0.0f,
0, 1, 0,  1.0f, 0.0f,
0, 0, 0,  1.0f, 1.0f,

//EAST
0, 1, 1,  1.0f, 0.0f,
0, 0, 0,  0.0f, 1.0f,
0, 1, 0,  0.0f, 0.0f,

0, 0, 0,  0.0f, 1.0f,
0, 1, 1,  1.0f, 0.0f,
0, 0, 1,  1.0f, 1.0f,

//WEST
1, 1, 1,  1.0f, 0.0f,
1, 1, 0,  0.0f, 0.0f,
1, 0, 0,  0.0f, 1.0f,

1, 0, 0,  0.0f, 1.0f,
1, 0, 1,  1.0f, 1.0f,
1, 1, 1,  1.0f, 0.0f,

 //TOP
0, 1, 0,  0.0f, 1.0f,
1, 1, 0,  1.0f, 1.0f,
1, 1, 1,  1.0f, 0.0f,

1, 1, 1,  1.0f, 0.0f,
0, 1, 1,  0.0f, 0.0f,
0, 1, 0,  0.0f, 1.0f,

//BOTTOM
0, 0, 0,  0.0f, 1.0f,
1, 0, 1,  1.0f, 0.0f,
1, 0, 0,  1.0f, 1.0f,

1, 0, 1,  1.0f, 0.0f,
0, 0, 0,  0.0f, 1.0f,
0, 0, 1,  0.0f, 0.0f
    };

f64 mxp = 0.0, myp = 0.0;

constexpr f32 sense = 0.5f;

ControllableCamera cam = ControllableCamera();

void mouse_callback(GLFWwindow* win, f64 xp, f64 yp) {
    if (!win) return;

    f64 dx = xp - mxp, dy = yp - myp;

    cam.changePitch(mu_rad(dy * sense));
    cam.changeYaw(mu_rad(dx * sense));

    mxp = xp;
    myp = yp;
}

//Camera cam = Camera({0.0f, 1.0f, -1.0f});

extern i32 game_main() {
    Path::SetProjectDir(PROJECT_DIR);
    std::cout << "Compiling Assets..." << std::endl;
    i32 code = AssetManager::compileDat("assets/global_map.json", "compass.pak");
    std::cout << "Asset Compliation exited with code: " << code << std::endl;

    Window::winIni();
    Window win = Window("your momcraft", 500, 500);

    glfwSetCursorPosCallback(win.wHandle, mouse_callback);

    graphics g = graphics(&win);

    g.Load();

    Mesh m;

    m.setMeshData((Vertex*) testFace, 36);

    //cam.setTarget({0.0f, 0.0f, 10.0f});

    Shader s = Shader::LoadShaderFromResource(
            "moop.pak", 
            "Globe.Map", 
            "Global.Graphics.Shaders.Core.Vert", 
            "Global.Graphics.Shaders.Core.Frag"
        );

    mat4 lookMat = cam.getLookMatrix();

    f32 T = 0.0f;

    g.setCurrentShader(&s);

    mat4 mm = mat4(1.0f);
    mm = mat4::Translate(mm, {0.0f, 0.0f, -3.0f});
    mm = mat4::Rotate(mm, 1.0f, {1.0f, 2.0f, 3.0f});

    std::cout << "Matt:" << std::endl;
    forrange(16) std::cout << mm.m[i] << " ";
    std::cout << std::endl;

    g.WinResize(500,500);

    while (win.isRunning()) {
        glClearColor(0.2, 0.7, 1.0, 1.0);
        g.render_begin();

        //cam.move({0.0f,0.01f,0.0f}, true);
        
        //mm = mat4::CreateTranslationMatrix({0.0f, 0.0f, 2.0f});
        //mm = mm * mat4::CreateRotationMatrixY(T, {0.0f, 0.0f, 2.0f});
        
        //mm = mm * mat4::CreateRotationMatrixY(T, {0.0f, 0.0f, 2.0f});
        //mm = mm * mat4::CreateRotationMatrixY(T, {0.0f, 0.0f, 0.0f});

        //mm = mat4C
        T += 0.01f;

        lookMat = cam.getLookMatrix();

        s.SetMat4("cam_mat", &lookMat);
        s.SetMat4("model_mat", &mm);

        g.push_verts((Vertex*)m.data(), m.size());
        g.render_flush();

        //mm = mat4();

        win.Update();
    }

    m.free();
    g.free();
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP