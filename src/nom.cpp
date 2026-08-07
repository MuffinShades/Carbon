#include "nom.hpp"

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

enum class neaIntSz {
    _8bit  = 0b00,
    _16bit = 0b01,
    _32bit = 0b10,
    _64bit = 0b11
};

constexpr IntFormat defEndian = IntFormat_BigEndian;
constexpr neaIntSz defISz = neaIntSz::_32bit;

void WriteToFile(std::string opath, nomfile f) {
    if (opath.length() == 0 || !f.assets || f.nassets == 0)
        return;

    //stream
    ByteStream s = ByteStream();

    s.writeBytes((byte*) const_cast<char*>(fih), 3); //fsig
    s.writeByte((byte) AssetTy::Generic);            //subformat
    stream_write_version(&s, nea_ver);
    const byte unicorn_byte = ((((byte) defEndian) & 1) << 7) | ((((byte) defISz) & 3) << 5);
    s.writeByte(unicorn_byte);
    //TODO: add stream functions to restore endians

    //first create the whole directory of le assets


    //write to the file
    FileWrite::writeToBin(opath, s.getBytePtr(), s.size());
    s.free();
}