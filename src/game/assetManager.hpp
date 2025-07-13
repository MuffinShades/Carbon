#pragma once
#include <iostream>
#include "../msutil.hpp"
#include "settings.hpp"
#include "../crosspath.hpp"
#include "../asset.hpp"
#include "../json.hpp"

#define GLOBAL_MAP_PATH Path::GetOSPath("assets/global_map.json")

struct _AssetStore {
    std::string id;
    Asset* asset;
};

class AssetManager {
private:
    static std::vector<_AssetStore> _store;
    static Asset* _map;
    static JStruct map_struct;
public:
    static Asset *ReqAsset(std::string id, std::string core_map_path, std::string core_map_id, bool store = true);
    static i32 compileDat(std::string map, std::string dat_out);
    static void free();
};