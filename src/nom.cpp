#include "nom.hpp"
#include <type_traits>
#include <assert.h>

constexpr byte dictionary_fmt_1 = 0x01;

const char fih[3] = {'N', 'E', 'A'};

enum class AssetTy {
    Generic = 0,
    Light = 1
};

enum class VerLabel {
    None = 0,
    Alpha = 0x0a,
    Beta = 0x0b,
    Ver = 0x01,
    Dev = 0x02,
    Release = 0x03,
    Debug = 0x04,
    Custom = 0x05
};

struct Version {
    VerLabel label_ty =  VerLabel::None;
    byte major_ver, minor_ver;
    u16 build;
    byte customLabel[4] = {0x3f, 0x3f, 0x3f, 0x3f};
};

void stream_write_version(ByteStream *s, Version ver) {
    if (!s) return;

    s->writeByte((byte) ver.label_ty);
    s->writeByte(ver.major_ver);
    s->writeByte(ver.minor_ver);
    s->writeUInt16(ver.build);
    s->writeBytes(ver.customLabel, 4);
}

const Version nea_ver = {
    VerLabel::Custom,
    0,1,826
};

//prevent against goons tryna exceed 32bits in a hash
#if __nea_max_hash > 32
#error "NEA hash hard max (__nea_max_hash) exceeds 32bits!"
#endif

//will write the primary directory and all sub directory onto the end of the given stream
//will also return the offset of the primary directory in the stream
i64 genDirectoryFmt1(ByteStream *s, nomfile f, nomsettings ns) {
    static_assert(__nea_max_hash <= 32, "NEA hash hard max (__nea_max_hash) exceeds 32bits!");

    if (!s)
        return -1;

    size_t hashBits = fast_log2(f.nassets);

    if (ns.maxHashBits > __nea_max_hash) ns.maxHashBits = __nea_max_hash;

    if (hashBits > ns.maxHashBits)
        hashBits = ns.maxHashBits;

    s->writeByte(dictionary_fmt_1);

    //do some sizing calculations
    nomasset *fa = f.assets;

    if (!fa || f.nassets == 0)
        return -1;

    i32 i;
    nomasset na;

    size_t nbll = 0; //num bytes in a label len

    for (i = 0; i < f.nassets; i++) {
        na = *fa;

        if (!na.dat || na.len == 0) {
            if (ns.delBlankAssets)
                continue;
            
            continue; //uhh.. :3
        }

        nbll = mu_max(nbll, fast_log2(na._side_info.id.idp_lens[0]));

        fa++;
    }
}

void WriteToFile(std::string opath, nomfile f, nomsettings ns) {
    if (opath.length() == 0 || !f.assets || f.nassets == 0)
        return;

    //stream
    ByteStream s = ByteStream();

    s.writeBytes((byte*) const_cast<char*>(fih), 3); //fsig
    s.writeByte((byte) AssetTy::Generic);            //subformat
    stream_write_version(&s, nea_ver);
    byte unicorn_byte = ((((byte) ns.endian) & 1) << 7) | ((((byte) ns.defISz) & 3) << 5);
    //s.writeByte(unicorn_byte);
    //TODO: add stream functions to restore endians

    //first create the whole directory of le assets


    //write to the file
    FileWrite::writeToBin(opath, s.getBytePtr(), s.size());
    s.free();
}