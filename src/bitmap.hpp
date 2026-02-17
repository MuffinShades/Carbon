#pragma once
#include <iostream>
#include "bytestream.hpp"
#include "filewrite.hpp"

#ifdef MSFL_DLL
#ifdef MSFL_EXPORTS
#define MSFL_EXP __declspec(dllexport)
#else
#define MSFL_EXP __declspec(dllimport)
#endif
#else
#define MSFL_EXP
#endif

#ifdef MSFL_DLL
#ifdef __cplusplus
extern "C" {
#endif
#endif

struct BitmapHeader {
    size_t fSz = 0;
    i16 bmpSig = 0;
    size_t w = 0, h = 0;
    i32 compressionMode = 0;
    u16 colorPlanes = 1;
    u16 bitsPerPixel = 24;
    i32 vResolution = 1, hResolution = 1;
    size_t nPalleteColors = 0;
    size_t importantColors = 0;
};

enum class BitmapStatus {
    Unknown, //Unknown / null bitmap error
    Good, //No bitmap errors
    BadDimensions, //width or height is negative or zero
    BadData, //corrupt bitmap data --> data is nullptr or not bitmap copatible | includes errors related to bitmap file size
    BadSig, //bad bitmap sig
    BadBPP, //bits per pixel is invalid
    BadHeader, //header is fucked up in some way
    BadPointer, //pointer passed to bitmapcheck is bad
    Blank
};

class Bitmap {
public:
    BitmapHeader header;
    byte* data = nullptr;


    ///////////////////////////////////////////////
    static Bitmap CreateBitmap(size_t w, size_t h);
    static void Free(Bitmap* bmp);
    static BitmapStatus BitmapCheck(Bitmap *bmp);
    ///////////////////////////////////////////////
};

class BitmapParse {
public:
    static i32 WriteToFile(std::string src, Bitmap* bmp);
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif