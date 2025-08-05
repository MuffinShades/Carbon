#ifndef CARBON_GAME_MAIN_HPP
#define CARBON_GAME_MAIN_HPP
#include <iostream>
#include "../msutil.hpp"
#include "../gl/window.hpp"
#include "assetManager.hpp"
#include "../gl/graphics.hpp"
#include "../gl/mesh.hpp"
#include "../gl/Camera.hpp"

extern i32 game_main() {
    Path::SetProjectDir(PROJECT_DIR);
    std::cout << "Compiling Assets..." << std::endl;
    i32 code = AssetManager::compileDat("../assets/global_map.json", "compass.pak");
    std::cout << "Asset Compliation exited with code: " << code << std::endl;

    Window::winIni();
    Window win = Window("your momcraft", 500, 500);

    graphics g = graphics(&win);

    g.Load();

    Vertex testFace[] = {
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };

    Mesh m;

    m.setMeshData(testFace, 6);

    Camera cam = Camera({0.0f, 1.0f, -1.0f});

    cam.setTarget({0.0f, 0.0f, 1.0f});

    Shader s = Shader::LoadShaderFromResource(
            "moop.pak", 
            "Globe.Map", 
            "Global.Graphics.Shaders.Core.Vert", 
            "Global.Graphics.Shaders.Core.Frag"
        );

    mat4 lookMat = cam.getLookMatrix();

    f32 T = 0.0f;

    g.setCurrentShader(&s);

    mat4 mm = mat4::CreateTranslationMatrix({0.0f, 0.0f, 10.0f});

    g.WinResize(500,500);

    while (win.isRunning()) {
        glClearColor(0.2, 0.7, 1.0, 1.0);

        //cam.move({0.0f,0.01f,0.0f}, true);

        //mm = mat4::CreateRotationMatrixZ(T, {0.0f, 0.0f, 0.0f});
        //mm = mm * mat4::CreateRotationMatrixY(T, {0.0f, 0.0f, 0.0f});

        //mm = mat4::Translate(mm, {0.0f, 0.0f, 0.01f});
        T += 0.01f;

        lookMat = cam.getLookMatrix();
        s.SetMat4("cam_mat", &lookMat);
        s.SetMat4("model_mat", &mm);

        g.push_verts((Vertex*)testFace, 6);
        g.render_flush();

        //mm = mat4();

        win.Update();
    }

    m.free();
    g.free();
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP