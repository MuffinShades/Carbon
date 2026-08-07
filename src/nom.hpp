#pragma once
#include "bytestream.hpp"
#include "crc.hpp"
//#include "crypto.hpp"
#include "json.hpp"
#include "xml.hpp"
#include "filewrite.hpp"

enum class _nomasset_origin {
    Unknown,
    Raw,
    File,
    Internal,
};

enum class CompressionMode {
    None,
    Zlib
};

struct nomasset {
    byte *dat = nullptr;
    size_t len = 0;
    u32 checksum = 0;
    struct {
        struct {
            std::string f_path;
            std::string internal_desc;
            _nomasset_origin oty = _nomasset_origin::Unknown;
        } origin;
        struct {
            char *id_dat;
            u16 *idp_lens;
            size_t nParts;
            bool use_16bit_part_lens = false;
            /*
            
            Format for 
            
            */
        } id;
        struct {
            CompressionMode compression = CompressionMode::None;
        } storage;
    } _side_info;
};

struct nomfile {
    nomasset *assets = nullptr;
    size_t nassets = 0;
};

class omn {
public:
    static void WriteToFile(std::string opath, nomfile f);
};