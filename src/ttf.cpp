#include "ttf.hpp"

constexpr u32 ttf_magic = 0x5f0f3cf5;

const std::string iTableTags[] = {
    "glyf",
    "head",
    "loca",
    "cmap",
    "hmtx"
};

//old
/*bool _strCompare(std::string a, std::string b) {
    const char *aCstr = a.c_str(), *bCstr = b.c_str();

    const size_t len = a.length();

    if (len != b.length())
        return false;

    for (size_t i = 0; i < len; i++) {
        if (aCstr[i] != bCstr[i])
            return false;
    }

    return true;
}*/

enum iTable getITableEnum(offsetTable tbl) {
    i32 p = 0;
    for (const std::string itm : iTableTags) {
        if (_strCompare(itm, tbl.tag))
            return (enum iTable)p;
        p++;
    }

    return iTable_NA;
}

//reference: https://developer.apple.com/fonts/TrueType-Reference-Manual/

//ttfSream stuff
i16 ttfStream::readFWord() {
    return this->readInt16();
}

u16 ttfStream::readUFWord() {
    return this->readUInt16();
}

f32 ttfStream::readF2Dot14() {
    return (f32)(this->readInt16()) / (f32)(1 << 14);
}

f32 ttfStream::readFixed() {
    return (f32)(this->readInt32()) / (f32)(1 << 16);
}

ttfLongDateTime ttfStream::readDate() {
    return (ttfLongDateTime)this->readUInt64();
}

std::string ttfStream::readString(size_t sz) {
    char* b = new char[sz], * cur = b;

    for (size_t i = 0; i < sz; i++)
        *cur++ = this->readByte();

    std::string res = std::string((const char*)b,sz);
    delete[] b;
    return res;
}

offsetTable readOffsetTable(ttfStream* stream) {
    offsetTable res;

    char tagBytes[] = {
        (char)stream->readByte(),
        (char)stream->readByte(),
        (char)stream->readByte(),
        (char)stream->readByte()
    };

    res.tag = std::string(tagBytes, 4);
    res.checkSum = stream->readUInt32();
    res.off = stream->readUInt32();
    res.len = stream->readUInt32();

    return res;
};

//loca
//dosent need a struct but heres how it works for reference:
/*
if head.idxToLocFormat == 0 (short) ->
    u16 -> offset to char n
elif header.idxToLocFormat == 1 (long) ->
    u32 -> offset to char n
*/

/**
 *
 * read_header_table
 *
 * just reads and parses a header table from a stream
 *
 */
table_head read_header_table(ttfStream* stream) {
    table_head res;

    res.fVersion = stream->readFixed();
    res.fontRevision = stream->readFixed();
    res.checkSumAdjust = stream->readUInt32();
    res.magic = stream->readUInt32();
    res.flags = stream->readUInt16();
    res.unitsPerEm = stream->readUInt16();
    res.created = stream->readDate();
    res.modified = stream->readDate();
    res.xMin = stream->readFWord();
    res.yMin = stream->readFWord();
    res.xMax = stream->readFWord();
    res.yMax = stream->readFWord();
    res.macStyle = stream->readUInt16();
    res.lowestRecPPEM = stream->readUInt16();
    res.fontDirectionHint = stream->readInt16();
    res.idxToLocFormat = stream->readInt16();
    res.glyphDataFormat = stream->readInt16();

    return res;
}

/**
 *
 * read_offset_tables
 *
 * reads the offset to the tables in
 * the ttf and saves important tables
 * in the file's memory
 *
 */
void read_offset_tables(ttfStream* stream, ttfFile* f) {
    f->scalarType = stream->readUInt32();
    const size_t nTables = stream->readUInt16();
    f->searchRange = stream->readUInt16();
    f->entrySelector = stream->readUInt16();
    f->rangeShift = stream->readUInt16();

    std::cout << "Found " << nTables << " tables!!" << std::endl;

    std::vector<offsetTable> res;

    //read the offset tables
    //TODO: checksum for le tables
    for (size_t i = 0; i < nTables; i++) {
        offsetTable table = readOffsetTable(stream);

        enum iTable tEnum = getITableEnum(table);
        table.iTag = tEnum;

        //if it's an important table then get it yk
        switch (tEnum) {
            //itable and read in header table thing ma bob
        case iTable_head: {
            f->head_table = table;
            size_t oPos = stream->seek(table.off);
            f->header = read_header_table(stream);
            stream->seek(oPos);
            break;
        }
        case iTable_glyph: {
            f->glyph_table = table;
            break;
        }
        case iTable_loca: {
            f->loca_table = table;
            break;
        }
        case iTable_cmap: {
            f->cmap_table = table;
            break;
        }
        default:
            break;
        }

        res.push_back(
            table
        );
    }

    f->tables = res;
}

/**
 *
 * getGlyphOffset
 *
 * function to take value from loca table
 * and get the position in the glyph table
 * of the target glyph (tChar)
 *
 */
u32 getGlyphOffset(ttfStream* stream, ttfFile* f, u32 tChar) {
    if (f->loca_table.iTag != iTable_loca)
        return 0;

    size_t rPos = stream->seek(f->loca_table.off);

    u32 offset = 0;

    switch (f->header.idxToLocFormat) {
    //short
    case 0: {
        size_t pOff = tChar << 1;
        stream->seek(stream->tell() + pOff);
        offset = stream->readUInt16();
        stream->seek(rPos);
        break;
    }
    //long / int
    case 1: {
        size_t pOff = tChar << 2;
        stream->seek(stream->tell() + pOff);
        offset = stream->readUInt32();
        stream->seek(rPos);
        break;
    }
    default:
        return 0;
    }

    return offset;
}

//cmap4
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
    void *segValBlock = nullptr;
};

void free_cmap_format_4(cmap_format_4* c) {
    _safe_free_a(c->segValBlock);
    ZeroMem(c, sizeof(cmap_format_4));
}

size_t decode_char_from_cmap4(cmap_format_4 table, u16 w_char) {
    i32 c_idx = 0;

    while (table.endCode[c_idx] <= w_char) {
        c_idx++;
        
        if (table.endCode[c_idx] == 0xffff)
            break;
    }

    if (table.startCode[c_idx] > w_char) {
        std::cout << "Character "<< w_char <<" not present in ttf!" << std::endl;
    }

    const u16 iro = table.idRangeOffset[c_idx];
    size_t glyphIndex = 0;

    if (iro == 0) {
        glyphIndex = (w_char + table.idDelta[c_idx]) % 0x10000; //modulo 65536
    } else {
        size_t loc = (c_idx + (table.idRangeOffset[c_idx] >> 1)) + (w_char - table.startCode[c_idx]);
        if (loc >= table.segCount) {
            std::cout << "tff error: failed to map glyph (cmap 4): out of range!" << std::endl;
        }
        glyphIndex = (table.idRangeOffset[loc] + table.idDelta[c_idx]) % 0x10000;
    }

    return glyphIndex;
}

cmap_format_4 cmap_4(ttfStream *stream) {
    if (!stream) return;

    cmap_format_4 table = {
        .table_len = stream->readUInt16(),
        .lang = stream->readUInt16(),
        .segCount = (u16) (stream->readUInt16() >> 1), //reading in segcount * 2 so we need to divide by 2 or rsh 1
        
        //DO NOT USE SINCE THEY COULD BE SKETCHY
        //also not needed
        .searchRange = stream->readUInt16(),
        .entrySelector = stream->readUInt16(),
        .rangeShift = stream->readUInt16()
    };

    if (table.segCount == 0) {
        std::cout << "ttf error: failed to read cmap4, not enough segments!" << std::endl;
        return;
    }

    //allocate a lot of memory
    constexpr size_t nSegValues = 4;
    u16 *segValueBlock = new u16[table.segCount * nSegValues];
    ZeroMem(segValueBlock, table.segCount * nSegValues);

    const size_t segSz = table.segCount * sizeof(u16);
    table.segValBlock = segValueBlock;

    //assign parts of the block
    table.endCode =       segValueBlock + segSz * 0;
    table.startCode =     segValueBlock + segSz * 1;
    table.idDelta =       (i16*) (segValueBlock + segSz * 2);
    table.idRangeOffset = segValueBlock + segSz * 3;

    forrange(table.segCount) {
        table.endCode[i] = stream->readUInt16();
    }

    table.reservePad = stream->readUInt16();

    if (table.reservePad != 0) {
        std::cout << "ttf error, invalid reserve pad!" << std::endl;
    }

    forrange(table.segCount) {
        table.startCode[i] = stream->readUInt16();
    }

    forrange(table.segCount) {
        table.idDelta[i] = stream->readInt16();
    }

    forrange(table.segCount) {
        table.idRangeOffset[i] = stream->readUInt16();
    }

    return table;
}

//cmap12
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

void free_cmap_format_12(cmap_format_12* c) {
    if (c->segValBlock)
        _safe_free_a(c->segValBlock);
    ZeroMem(c, sizeof(cmap_format_12));
}

cmap_format_12 cmap_12(ttfStream *stream) {
    if (!stream) return;

    const u16 fmt = stream->readUInt16();

    if (fmt != 12) {
        std::cout << "failed to read cmap 12 table: table is not cmap 12!" << std::endl;
        return {};
    }

    const u16 rsv = stream->readUInt16();

    if (rsv != 0) {
        std::cout << "warning: cmap 12 reserve is not zero!" << std::endl;
    }

    cmap_format_12 table = {
        .len = stream->readUInt32(),
        .lang = stream->readUInt32(),
        .nGroups = stream->readUInt32()
    };

    if (table.nGroups == 0)
        return table;

    table.groups = new cmap12_group[table.nGroups];
    ZeroMem(table.groups, table.nGroups);
    table.segValBlock = (void*) table.groups;

    i32 i;

    for (i = 0; i <  table.nGroups; ++i) {
        cmap12_group g = {
            .start_cc = stream->readUInt32(),
            .end_cc = stream->readUInt32(),
            .start_gc = stream->readUInt32()
        };

        table.groups[i] = g;
    }

    return table;
}

size_t decode_char_from_cmap12(cmap_format_12 table, u32 uw_char) {
    if (table.nGroups == 0 || !table.groups)
        return 0;

    i32 idx = 0;

    cmap12_group g = table.groups[0];

    while (g.end_cc <= uw_char) {
        idx++;

        if (idx >= table.nGroups) 
            return 0;
    }

    if (g.start_cc > uw_char) return 0;

    return g.start_gc + (uw_char - g.start_cc);
}

/**
 *
 * getUnicodeOffset
 *
 * function to take value from cmap table
 * and get the position in the loca table
 * of the target character (tChar). Then
 * gets location in glyph table based off
 * of loca table.
 *
 */
u32 getUnicodeOffset(ttfStream* stream, ttfFile* f, u32 tChar) {
    const size_t mapOff = f->cmap_table.off;
    const size_t rPos = stream->seek(mapOff);

    //read data from the cmap table
    const u16 version = stream->readUInt16();
    const u16 nSubTables = stream->readUInt16();

    //mode
    if (f->platform == CMap_null) {
        f->platform = (enum CMapMode)stream->readInt16();
        f->encodingId = stream->readInt16();

        const u32 off = stream->readUInt32();
        stream->seek(off);
        const u16 cmap_format = stream->readUInt16();

        switch (cmap_format) {
        case 4:

            break;
        case 12:
            break;
        default:
            std::cout << "ttf error: unsupported cmap format: " << cmap_format << std::endl;
            break;
        }
    }
    else
        //since we've already read it, skip the map type and platform specific ID
        stream->skip(sizeof(i16) * 2);

    stream->seek(rPos);

    return 0;
}

/**
 *
 * read_glyph
 *
 *
 * Reads a glyph from the ttf file.
 * Returns a glyph with the points and flags
 */
Glyph read_glyph(ttfStream* stream, ttfFile* f, u32 loc) {
    Glyph res;
    if (stream == nullptr || f == nullptr) {
        std::cout << "ttf error: invalid stream or file!" << std::endl;
        return res;
    }

    const size_t rPos = stream->seek(f->glyph_table.off + loc);

    //read some glyph data
    res.nContours = stream->readInt16();
    res.xMin = stream->readFWord();
    res.yMin = stream->readFWord();
    res.xMax = stream->readFWord();
    res.yMax = stream->readFWord();

    if (res.nContours == 0) {
        std::cout << "ttf warning: found no contours!" << std::endl;
        return res;
    }

    if (res.nContours < 0) {
        std::cout << "ttf warning: compound glyph!" << std::endl;
        return read_glyph(stream, f, 0);
    }

    //for now we can only read simple glyphs
    i32* contourEnds = new i32[res.nContours];
    size_t nPoints = 0;

    size_t i;

    for (i = 0; i < res.nContours; i++) {
        contourEnds[i] = stream->readUInt16();

        if (contourEnds[i] > nPoints)
            nPoints = contourEnds[i];
    }

    nPoints++; //number of points we need to read

    //skip over byte instructions
    const size_t nInstructions = stream->readUInt16();
    stream->skip(nInstructions);

    //flags
    byte* flags = new byte[nPoints];

    for (i = 0; i < nPoints; i++) {
        byte flag = stream->readByte();
        flags[i] = flag;

        if ((bool)GetFlagValue(flag, PointFlag_repeat)) {
            size_t repeatAmount = stream->readByte();

            while (repeatAmount--)
                flags[++i] = flag;
        }
    }

    //now do point stuff
    Point* glyphPoints = new Point[nPoints];

    //x coordinates
    for (i = 0; i < nPoints; i++) {
        const byte flag = flags[i];
        size_t xSz = ((size_t)!((bool)GetFlagValue(flag, PointFlag_xSz))) + 1;
        bool xMode = (bool)GetFlagValue(flag, PointFlag_xMode);
        i32 xSign = xSz == 1 ? xMode ? 1 : -1 : 1;

        if ((bool)(xSz - 1) && xMode) { //skip offset if offset flag is set to zero
            if (i <= 0) {
                glyphPoints[i].x = 0;
                continue;
            }
            glyphPoints[i].x = glyphPoints[i - 1].x;
            continue;
        }
        else {
            switch (xSz) {
            case 1: {
                glyphPoints[i].x = stream->readByte() * xSign;
                break;
            }
            case 2: {
                glyphPoints[i].x = stream->readInt16();
                break;
            }
            default:
                return res;
            }

            if (i > 0)
                glyphPoints[i].x += glyphPoints[i - 1].x;
        }
    }

    //y coordinates
    for (i = 0; i < nPoints; i++) {
        const byte flag = flags[i];
        size_t ySz = ((size_t)!((bool)GetFlagValue(flag, PointFlag_ySz))) + 1;
        bool yMode = (bool)GetFlagValue(flag, PointFlag_yMode);
        i32 ySign = ySz == 1 ? yMode ? 1 : -1 : 1;

        if ((bool)(ySz - 1) && yMode) {
            if (i <= 0) {
                glyphPoints[i].y = 0;
                continue;
            }
            
            glyphPoints[i].y = glyphPoints[i - 1].y;
            continue;
        }
        else {
            switch (ySz) {
            case 1: {
                glyphPoints[i].y = stream->readByte() * ySign;
                break;
            }
            case 2: {
                glyphPoints[i].y = stream->readInt16();
                break;
            }
            default:
                return res;
            }

            if (i > 0)
                glyphPoints[i].y += glyphPoints[i - 1].y;
        }
    }

    //set res stuff
    res.points = glyphPoints;
    res.flags = flags;
    res.contourEnds = contourEnds;
    res.nPoints = nPoints;

    stream->seek(rPos);
    return res;
}

Glyph read_unicode_glyph(ttfStream* stream, ttfFile *f, u32 code) {
    return {};
}

ttfFile create_err_file(int code) {
    ttfFile res;
    //TODO: add error stuff for le ttf file
    return res;
}

ttfFile ttfParse::ParseTTFFile(std::string src) {
    file fBytes = FileWrite::readFromBin(src);

    if (fBytes.dat == nullptr || fBytes.len <= 0)
        return create_err_file(1);

    ttfFile r = ttfParse::ParseBin(fBytes.dat, fBytes.len);
    delete[] fBytes.dat;

    return r;
}

ttfFile ttfParse::ParseBin(byte* dat, size_t sz) {

    //create file and le byte stream
    ttfFile f;
    ttfStream fStream = ttfStream(dat, sz);

    //read offset tables and other important info
    read_offset_tables(&fStream, &f);

    fStream.free();

    return f;
}


Glyph ttfParse::ReadTTFGlyph(std::string src, u32 id) {
    Glyph tGlyph;
    file fBytes = FileWrite::readFromBin(src);

    if (fBytes.dat == nullptr || fBytes.len <= 0) {
        std::cout << "ttf error: failed to read file!" << std::endl;
        return tGlyph;
    }

    ttfFile f;
    ttfStream fStream = ttfStream(fBytes.dat, fBytes.len);

    read_offset_tables(&fStream, &f);

    u32 offset = getGlyphOffset(&fStream, &f, id);
    tGlyph = read_glyph(&fStream, &f, offset);

    delete[] fBytes.dat;
    fStream.free();

    return tGlyph;
}