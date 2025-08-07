#include "assetManager.hpp"
#include "../json.hpp"
#include "../asset.hpp"
#include "../filewrite.hpp"

std::vector<_AssetStore> AssetManager::_store;
Asset *AssetManager::_map = nullptr;
JStruct AssetManager::map_struct;

i32 AssetManager::compileDat(std::string map, std::string dat_out) {
    if (map.length() <= 0)
        return 1;

    //read map file
    std::cout << "LOCATION: " << Path::GetOSPath(map) << std::endl;
    file map_fDat = FileWrite::readFromBin(Path::GetOSPath(map));

    if (map_fDat.len <= 0 || !map_fDat.dat) {
        if (map_fDat.dat)
            _safe_free_a(map_fDat.dat);

        return 1;
    }

    //now parse file
    //WARNING: buffer overflow could occur here although im not 100% sure so dont quote me ok
    JStruct dMap = jparse::parseStr(const_cast<const char*>(reinterpret_cast<char*>(map_fDat.dat)));
    _safe_free_a(map_fDat.dat);

    if (dMap.body.size() <= 0)
        return 2;

    std::cout << "Found: " << dMap.body.size() << " items" << std::endl;

    AssetParse::WriteToFile(Path::GetOSPath(dat_out), escapeStrBackslashes("{\"map\":\"" + map + "\"}"));

    std::string baseDir, aMap, outputFile;

    for (auto& itm : dMap.body) {

        //create asset file
        baseDir = itm.body->FindToken("dir").rawValue;
        if (baseDir.length() == 0) 
            continue;

        aMap = baseDir + itm.body->FindToken("map_target").rawValue;
        if (aMap.length() == 0) 
            continue;
        else 
            aMap = Path::GetOSPath(aMap);

        outputFile = itm.body->FindToken("output_file").rawValue;
        if (outputFile.length() == 0) 
            continue;
        else 
            outputFile = Path::GetOSPath(outputFile);

        //parse the map file
        file _map = FileWrite::readFromBin(aMap);

        if (!_map.dat || _map.len == 0) {
            if (_map.dat)
                _safe_free_a(_map.dat);

            std::cout << "Asset Manager Warning! Failed to read from map: " << aMap << std::endl;

            continue;
        }

        //convert map data to string
        std::string map_dat = std::string(
            const_cast<const char*>(
                reinterpret_cast<char*>(_map.dat)
            ),
            _map.len
        );

        AssetParse::WriteToFile(outputFile, map_dat, Path::GetOSPath(baseDir));
        _safe_free_a(_map.dat);
    }

    return 0;
}

Asset* _create_asset_copy(Asset* a) {
    Asset* r = new Asset;
    in_memcpy(r, a, sizeof(Asset));
    in_memcpy(&r->inf, &a->inf, sizeof(AssetDescriptor));
    r->__nb_free = true;
    return r;
}

Asset *AssetManager::ReqAsset(std::string id, std::string core_map_path, std::string core_map_id, bool store) {
    if (!_map) {
        std::cout << "Core Loc: " << Path::GetOSPath(core_map_path) << " | " << Path::GetOSPath(core_map_path) << std::endl;
        std::cout << "Id: " << core_map_id << std::endl;

        Asset e = AssetParse::ExtractAssetFromFile(
            Path::GetOSPath(core_map_path),
            core_map_id
        );

        if (e.bytes == nullptr || e.sz <= 0) {
            std::cout << "Error couldnt load map, INVALID PATH" << std::endl;
            return nullptr;
        }

        _map = _create_asset_copy(&e);
        std::string mapDat = CovertBytesToString(_map->bytes, _map->sz, true);
        std::cout << "MAP DAT: " << mapDat << std::endl;
        map_struct = jparse::parseStr(mapDat.c_str());

        //DO NOT FREE ASSEST E SINCE THE DATA IS NOW OWNED BY THE COPY!!!
        //also _map does not need to be a pointer ;-;
    }

    std::string file = "";

    const char* id_c = id.c_str();
    const size_t idLen = id.length();
    char* _c = const_cast<char*>(id_c);
    
    do { 
        file += *_c;
    } while ((size_t)(_c - id_c) < idLen  && *++_c != '.');

    const size_t subLen = (size_t)(_c - id_c) + 1;

    std::cout << "FILE: " << file << std::endl;

    JToken fTok = map_struct.FindToken(file);

    if (!fTok.body) {
        std::cout << "Failed to find token: " << file << std::endl;
        return nullptr;
    }

    file = Path::GetOSPath(fTok.body->FindToken("output_file").rawValue);

    const size_t f_len = file.length();

    //check store first
    for (auto& _s : _store)
        if (_strCompare(_s.id, id, true, id.length()))
            return _create_asset_copy(_s.asset);

    Asset extracted = AssetParse::ExtractAssetFromFile(file, id.substr(subLen));

    //store assset if needed
    if (store) {
        Asset* re = nullptr;
        _store.push_back({
            .id = std::string(id),
            .asset = (re = _create_asset_copy(&extracted))
        });

        return re;
    }

    return _create_asset_copy(&extracted);
}

void AssetManager::free() {
    for (auto& a : _store)
        if (a.asset != nullptr) {
            _safe_free_a(a.asset->bytes);
            delete a.asset;
        }
}