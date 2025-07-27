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
    Window win = Window("moop", 500, 500);

    graphics g = graphics(&win);

    g.Load();

    g.WinResize(500,500);

    Vertex testFace[] = {
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f,

        1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f
    };

    Mesh m;

    m.setMeshData(testFace, 6);

    Camera cam = Camera({0.0f, 1.0f, -1.0f});

    cam.setTarget({0.5f, 0.5f, 1.0f});

    Shader s = Shader::LoadShaderFromResource(
            "moop.pak", 
            "Globe.Map", 
            "Graphics.Shaders.Core.Vert", 
            "Graphics.Shaders.Core.Frag"
        );

    mat4 lookMat = cam.getLookMatrix();

    g.setCurrentShader(&s);

    while (win.isRunning()) {
        glClearColor(0.2, 0.7, 1.0, 1.0);

        s.SetMat4("cam_mat", &lookMat);

        g.push_verts((Vertex*)m.data(), m.size());
        g.render_flush();

        win.Update();
    }

    m.free();
    g.free();
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP