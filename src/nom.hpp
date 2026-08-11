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

enum class neaIntSz {
    _8bit  = 0b00,
    _16bit = 0b01,
    _32bit = 0b10,
    _64bit = 0b11
};

//WARNING: increasing changing this value can cause files to take up crazy amounts of storage!
/*

i.e.

__nea_max_hash = 17 --> min bytes per hash table = 2^17 = 131 Kilobytes
__nea_max_hash = 32 --> min bytes per hash table = 2^32 = 4 Gigabytes
__nea_max_hash = 64 --> min bytes per hash table = 2^64 = 16 Zettabytes (1 zettabyte = 1,000,000,000 terrabytes)

*/
constexpr size_t __nea_max_hash = 16;

struct nomsettings {
    size_t chunk_id_len = 1;
    IntFormat endian = IntFormat_BigEndian;
    neaIntSz defISz = neaIntSz::_32bit;
    size_t maxHashBits = 8;
    bool delBlankAssets = true;
};

class omn {
public:
    static void WriteToFile(std::string opath, nomfile f, nomsettings ns);
};