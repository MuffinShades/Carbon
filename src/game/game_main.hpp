#ifndef CARBON_GAME_MAIN_HPP
#define CARBON_GAME_MAIN_HPP
#include <iostream>
#include "../msutil.hpp"
#include "../gl/window.hpp"
#include "assetManager.hpp"
#include "../gl/graphics.hpp"

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

    while (win.isRunning()) {
        //glClearColor(1.0, 0.0, 0.0, 1.0);
        g.FillRect(50, 50, 100, 100);
        g.render_flush();
        win.Update();
    }
    g.free();
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP