#ifndef CARBON_GAME_MAIN_HPP
#define CARBON_GAME_MAIN_HPP
#include <iostream>
#include "../msutil.hpp"
#include "../gl/window.hpp"
#include "assetManager.hpp"

extern i32 game_main() {
    Path::SetProjectDir(PROJECT_DIR);
    std::cout << "Compiling Assets..." << std::endl;
    i32 code = AssetManager::compileDat("../assets/global_map.json", "moop.pak");
    std::cout << "Asset Compliationo exited with code: " << code << std::endl;

    Window::winIni();
    Window win = Window("moop", 500, 500);

    while (win.isRunning()) {
        win.Update();
    }
    return 0;
}
#endif //CARBON_GAME_MAIN_CPP