#include "bitmap.hpp"
#include "logger.hpp"

static Logger l;

#define BMP_SIG 0x4d42
#define BMP_HEADER_SZ 40

int write_bmp_header(Bitmap* bmp, ByteStream* stream) {
    const u16 bpp = bmp->header.bitsPerPixel;
    if (bpp & 7 || bpp > 32) {
        if (bpp != 4 || bpp != 1) {
            std::cout << "bmp error: invalid number of bits per pixel!" << std::endl;
            return 1;
        }
    }

    std::cout << "Bmp Header Write" << std::endl;
    std::cout << "Sz: " << BMP_HEADER_SZ << std::endl;
    std::cout << "Width: " << bmp->header.w << std::endl;
    std::cout << "Height: " << bmp->header.h << std::endl;

    stream->writeUInt32(BMP_HEADER_SZ); //write header size
    stream->writeUInt32(bmp->header.w);
    stream->writeUInt32(bmp->header.h);
    stream->writeUInt16(bmp->header.colorPlanes);

    if (bpp <= 24)
        stream->writeUInt16(bpp);
    else 
        stream->writeUInt16(24);

    stream->writeUInt32(bmp->header.compressionMode);
    stream->writeUInt32(0);
    stream->writeInt32(bmp->header.hResolution);
    stream->writeInt32(bmp->header.vResolution);
    stream->writeUInt32(bmp->header.nPalleteColors);
    stream->writeUInt32(bmp->header.importantColors);
    return 0;
}

//TODO: 
//fix rgb bitmaps -> rgba but with 0 for alpha
//fix order of rgba bitmaps -> argb instead of rgba
i32 BitmapParse::WriteToFile(std::string src, Bitmap* bmp) {
    if (
        src.length() <= 0 ||
        !bmp ||
        !bmp->data
    ) return 1;

    std::cout << "Writing bmp stream" << std::endl;

    ByteStream datStream;

    datStream.int_mode = IntFormat_LittleEndian;

    const size_t by_pp = bmp->header.bitsPerPixel >> 3;
    const size_t nPix= (bmp->header.w * bmp->header.h),
                 nPixBytes = nPix * by_pp;

    bmp->header.fSz = nPix * (mu_min(bmp->header.bitsPerPixel, 24) >> 3);

    write_bmp_header(bmp, &datStream);

    const size_t datPos = datStream.size();

    //write bitmap data

    //create file stream
    ByteStream oStream;

    oStream.int_mode = IntFormat_LittleEndian;

    oStream.writeUInt16(BMP_SIG); //sig
    oStream.writeUInt32(datStream.size() + bmp->header.fSz + 14); //how many bytes in file
    oStream.writeUInt32(0); // reserved
    oStream.writeUInt32(oStream.size() + sizeof(u32) + datPos); //data offset

    std::cout << "header sz: " << datStream.size() << std::endl;
    l.LogHex(datStream.getBytePtr(), 16);

    std::cout << "Data offset: " << oStream.size() + sizeof(u32) + datPos << std::endl;
    //oStream.writeBytes(datStream.getBytePtr(), datStream.size());
    oStream.writeBytes(datStream.getBytePtr(), datStream.size());

    i32 p;
    byte *b_data;

    switch (by_pp) {
    case 2:
        oStream.writeBytes(bmp->data, bmp->header.fSz);
        break;
    case 3:
        b_data = bmp->data;
        for (p = 0; p < nPixBytes; p += by_pp) {
            oStream.writeUInt24(
                b_data[p+2] | (b_data[p+1] << 8) | (b_data[p] << 16)
            );
        }
        break;
    case 4:
        b_data = bmp->data;
        for (p = 0; p < nPixBytes; p += by_pp) {
            oStream.writeUInt24(
                b_data[p+2] | (b_data[p+1] << 8) | (b_data[p] << 16)
            );
        }
        break;
    }

    l.LogHex(oStream.getBytePtr(), 100);

    datStream.free();

    std::cout << "Writing Bmp to: " << src << std::endl;
    FileWrite::writeToBin(src, oStream.getBytePtr(), oStream.size());

    oStream.free();

    return 0;
}

Bitmap Bitmap::CreateBitmap(size_t w, size_t h) {
    if (w < 0 || h < 0)
        return { 0 };

    const size_t allocSz = w * h;

    BitmapHeader header = {
        .fSz = allocSz,
        .bmpSig = BMP_SIG,
        .w = w, 
        .h = h
    };

    Bitmap res = {
        .header = header,
        .data = new byte[allocSz]
    };

    ZeroMem(res.data, allocSz);

    return res;
}

void Bitmap::Free(Bitmap* bmp)  {
    if (!bmp) return;

    if (bmp->data)
        delete[] bmp->data;

    bmp->data = nullptr;
    bmp->header = {};
}