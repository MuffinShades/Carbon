#include <iostream>
#include "types.hpp"

class ContentSrc {
public:
    enum class SourceFormat {
        Unknown,
        File,
        Binary
    };
private:
    
    std::string f_src = "";
    byte *f_dat = nullptr;
    size_t sz = 0;
    bool computed_sz = false, computed_fdat = false;
    SourceFormat fmt = SourceFormat::Unknown;
public:
    static ContentSrc FromFile(std::string path);
    static ContentSrc FromBinary(byte *dat, size_t sz, bool make_copy = false, bool free_src = false);
    size_t size();
    byte *data();
    std::string src();
    SourceFormat format();
    void free();
};