#pragma once
#include <iostream>
#include "filewrite.hpp"
#include "bytestream.hpp"

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

//some flagg stuff for getting a glyph
enum PointFlag {
    PointFlag_onCurve,
    PointFlag_xSz,
    PointFlag_ySz,
    PointFlag_repeat,
    PointFlag_xMode,
    PointFlag_yMode
};

#define GetFlagValue(f, p) ((f) & (1 << (p))) //macro this function cause it's so small

static inline i32 ModifyFlagValue(i32 f, i32 p, i32 v) {
    return (f & (0xff ^ (1 << p))) | ((v & 1) << p); //weird ass bit manipulation thingy
}

typedef long long ttfLongDateTime;

enum iTable {
    iTable_glyph,
    iTable_head,
    iTable_loca,
    iTable_cmap,
    iTable_hmtx,
    iTable_NA
};

//ttf file stuff starts here
struct offsetTable {
    u32 checkSum;
    size_t off;
    size_t len;
    std::string tag;
    enum iTable iTag = iTable_NA;
};

/**
 *
 * All the structs for a ttf file
 *
 * yeah it's a lot
 *
 */
//head
struct table_head {
    float fVersion;
    float fontRevision;
    u32 checkSumAdjust;
    u32 magic;
    u16 flags;
    u16 unitsPerEm;
    ttfLongDateTime created;
    ttfLongDateTime modified;
    float xMin, yMin, xMax, yMax;
    u16 macStyle;
    u16 lowestRecPPEM;
    i16 fontDirectionHint;
    i16 idxToLocFormat;
    i16 glyphDataFormat;
};

class ttfStream : public ByteStream {
public:
    MSFL_EXP short readFWord();
    MSFL_EXP unsigned short readUFWord();
    MSFL_EXP float readF2Dot14();
    MSFL_EXP float readFixed();
    MSFL_EXP std::string readString(size_t sz);
    MSFL_EXP ttfLongDateTime readDate();
    ttfStream() : ByteStream() {
        this->int_mode = IntFormat_BigEndian;
    }
    ttfStream(byte* dat, size_t sz) : ByteStream(dat,sz) {
        this->int_mode = IntFormat_BigEndian;
    }
};

struct Glyph {
    i16 nContours;
    float xMin, yMin, xMax, yMax;
    Point* points = nullptr;
    byte* flags = nullptr;
    i32* contourEnds = nullptr;
    size_t nPoints;
    i32* modifiedContourEnds = nullptr;
};

enum CMapMode {
    CMap_Unicode,
    CMap_Mac,
    CMap_Reserved,
    CMap_Microsoft,
    CMap_null
};

//cmap formats
struct cmap_format_4 {
    u16 
        table_len, 
        lang, 
        segCount = 0, 
        searchRange, 
        entrySelector, 
        rangeShift, 
        *endCode = nullptr, 
        reservePad,
        *startCode = nullptr;
    i16 *idDelta = nullptr;
    u16 *idRangeOffset = nullptr;
    u16 *glyphIdArr = nullptr;
    size_t nGlyphIds = 0, segBlockSz = 0;
    void *segValBlock = nullptr;
};

struct cmap12_group {
    u32 start_cc, //start char code
        end_cc, //end char code
        start_gc; //start glyph code
};

struct cmap_format_12 {
    u32 len,
        lang,
        nGroups;

    //groups and segValBlock are linked
    //only delete this object with free_cmap_format_12
    cmap12_group *groups = nullptr;
    void *segValBlock = nullptr;
};

struct h_char_inf {
    i16 ascent;
    i16 descent;
    i16 lineGap;
    u16 advanceWidthMax;
    u16 minLeftSideBearing;
    u16 minRightSideBearing;
    i16 xMaxExtent;
    f32 caret_slope = 0.0f;
    bool vertical_caret_slope = false;
    i16 caretOffset;
    i16 metricDataFormat;
    u16 nLongHorMetrics;
};

struct char_metric {

};

struct char_set {

};

//le file
class ttfFile {
public:
    union {
        cmap_format_12 fmt_12;
        cmap_format_4 fmt_4;
    } cmap_fmt = {};
    u32 cmap_id = 0xff;
    u32 scalarType;
    u16 searchRange;
    u16 entrySelector;
    u16 rangeShift;
    offsetTable head_table;
    offsetTable loca_table;
    offsetTable glyph_table;
    offsetTable cmap_table;
    offsetTable hhea_table;
    offsetTable hmtx_table;
    CMapMode platform = CMap_null;
    i32 encodingId = 0;
    std::vector<offsetTable> tables;
    table_head header;
    h_char_inf h_inf;
};

enum class UnicodeRange {
    Utf8,
    Utf16,
    Latin_Basic,
    Latin_1_Sup,
    Latin_Exa,
    Latin_Exb,
    Latin_Full,
    IPA_Ext,
    Spacing_Mod_Letters

};

class ttfParse {
public:
    MSFL_EXP static ttfFile ParseTTFFile(std::string src);
    MSFL_EXP static ttfFile ParseBin(byte* dat, size_t sz);
    MSFL_EXP static Glyph ReadTTFGlyph(std::string src, u32 id);
    MSFL_EXP static Glyph ReadUnicodeGlyph(std::string src, u32 id);
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif