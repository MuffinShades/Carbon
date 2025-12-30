#include "ttf.hpp"

constexpr u32 ttf_magic = 0x5f0f3cf5;

const std::string iTableTags[] = {
    "glyf",
    "head",
    "loca",
    "cmap",
    "hmtx",
    "hhea"
};

/*

Format:

There are n bytes per range


First byte is a settings byte

Bit 1: range or coglomeration of ranges (ie all the latins combined)
Bit 2-3: size in bytes of each range - 1
Bits 4-8: reserved

Byte 2: number of ranges if range conglomeration flag is false
        if range conglomeration flag is true this specifies how many subsets are used

Range Conglomeration Flag set to true -->
    list of enum codes for all the sub ranges

Range Conglomeration Flag set to false -->
    min then max for a given range and then
*/

const u8 uncode_range_decode[] = {
    0b00000000, 0x01, 0x41, 0x5a, //simple alphabet
    0b00000000, 0x01, 0x00, 0xff, //utf-8
    0b00100000, 0x01, 0x00, 0x00, 0xff, 0xff, //utf-16
    0b00000000, 0x01, 0x20, 0x7f, //Latin Basic
    0b00000000, 0x01, 0xa0, 0xff, //Latin-1 Sup
    0b00100000, 0x01, 0x01, 0x00, 0x01, 0x7f, // Latin Ext-a
    0b00100000, 0x01, 0x01, 0x80, 0x02, 0x4f, // Latin Ext-b
    0b10000000, 0x04, 0x02, 0x03, 0x04, 0x05, // Latin Full
    0b00100000, 0x01, 0x02, 0x50, 0x02, 0xaf, //IPA Ext
    0b00100000, 0x01, 0x02, 0xb0, 0x02, 0xff, //spacing mod letters
    0b00100000, 0x01, 0x03, 0x00, 0x03, 0x6f, //diacritical marks
    0b00100000, 0x01, 0x03, 0x70, 0x03, 0xff, //greeek and coptic
    0b00100000, 0x01, 0x04, 0x00, 0x04, 0xff, //cryillic
    0b00100000, 0x01, 0x05, 0x00, 0x05, 0x2f, //cryillic sup
    0b00100000, 0x01, 0x05, 0x30, 0x05, 0x8f, //armenian
    0b00100000, 0x01, 0x05, 0x90, 0x05, 0xff, //hebrew
    0b00100000, 0x01, 0x06, 0x00, 0x06, 0xff, //arabic
    0b00100000, 0x01, 0x07, 0x00, 0x07, 0x4f, //syriac
    0b00100000, 0x01, 0x07, 0x80, 0x07, 0xbf, //thaana
    0b00100000, 0x01, 0x09, 0x00, 0x09, 0x7f, //Devanagari
    0b00100000, 0x01, 0x09, 0x80, 0x09, 0xff, //Bengali
    0b00100000, 0x01, 0x0a, 0x00, 0x0a, 0x7f, //Gurmukhi
    0b00100000, 0x01, 0x0a, 0x80, 0x0a, 0xff, //Gujarati
    0b00100000, 0x01, 0x0b, 0x00, 0x0b, 0x7f, //Oriya
    0b00100000, 0x01, 0x0b, 0x80, 0x0b, 0xff, //Tamil
    0b00100000, 0x01, 0x0c, 0x00, 0x0c, 0x7f, //Telugu
    0b00100000, 0x01, 0x0c, 0x80, 0x0c, 0xff, //Kannada
    0b00100000, 0x01, 0x0d, 0x00, 0x0d, 0x7f, //Malayalam
    0b00100000, 0x01, 0x0d, 0x80, 0x0d, 0xff, //Sinhala
    0b00100000, 0x01, 0x0e, 0x00, 0x0e, 0x7f, //Thai
    0b00100000, 0x01, 0x0e, 0x80, 0x0e, 0xff, //Lao
    0b00100000, 0x01, 0x0f, 0x00, 0x0f, 0xff, //Tibetan
    0b00100000, 0x01, 0x10, 0x00, 0x10, 0x9f, //myanmar
    0b00100000, 0x01, 0x10, 0xa0, 0x10, 0xff, //georgian
    0b00100000, 0x01, 0x11, 0x00, 0x11, 0xff, //hangul jamo
    0b00100000, 0x01, 0x12, 0x00, 0x13, 0x7f, //Ethiopic
    0b00100000, 0x01, 0x13, 0xa0, 0x13, 0xff, //cherokee
    0b00100000, 0x01, 0x14, 0x00, 0x16, 0x7f, //Unified Canadian Aboriginal Syllabics
    0b00100000, 0x01, 0x16, 0x80, 0x16, 0x9f, //Ogham
    0b00100000, 0x01, 0x16, 0xa0, 0x16, 0xff, //Runic
    0b00100000, 0x01, 0x17, 0x00, 0x17, 0x1f, //Tagalog
    0b00100000, 0x01, 0x17, 0x20, 0x17, 0x3f, //Hanunoo
    0b00100000, 0x01, 0x17, 0x40, 0x17, 0x5f, //Buhid
    0b00100000, 0x01, 0x17, 0x60, 0x17, 0x7f, //Tagbanwa
    0b00100000, 0x01, 0x17, 0x80, 0x17, 0xff, //Khmer
    0b00100000, 0x01, 0x18, 0x00, 0x18, 0xaf, //Mongolian
    0b00100000, 0x01, 0x19, 0x00, 0x19, 0x4f, //Limbu
    0b00100000, 0x01, 0x19, 0x50, 0x19, 0x7f, //Tai Le
    0b00100000, 0x01, 0x19, 0xe0, 0x19, 0xff, //Khmer Symbols
    0b00100000, 0x01, 0x1d, 0x00, 0x1d, 0x7f, //Phoenetic Extensions
    0b00100000, 0x01, 0x1e, 0x00, 0x1e, 0xff, //Latin Extended Additional
    0b00100000, 0x01, 0x1f, 0x00, 0x1f, 0xff, //Greek Extended
    0b00100000, 0x01, 0x20, 0x00, 0x20, 0x6f, //General Punctuation
    0b00100000, 0x01, 0x20, 0x70, 0x20, 0x9f, //Superscripts and Subscripts
    0b00100000, 0x01, 0x20, 0xa0, 0x20, 0xcf, //Curreny Symbols
    0b00100000, 0x01, 0x20, 0xd0, 0x20, 0xff, //Combining Diacritical Marks for Symbols (wtf)
    0b00100000, 0x01, 0x21, 0x00, 0x21, 0x4f, //Letterlike Symbols
    0b00100000, 0x01, 0x21, 0x50, 0x21, 0x8f, //Number Forms
    0b00100000, 0x01, 0x21, 0x90, 0x21, 0xff, //Arrows
    0b00100000, 0x01, 0x22, 0x00, 0x22, 0xff, //Mathematical Operators
    0b00100000, 0x01, 0x23, 0x00, 0x23, 0xff, //Miscellaneous Technical
    0b00100000, 0x01, 0x24, 0x00, 0x24, 0x3f, //control pictures
    0b00100000, 0x01, 0x24, 0x40, 0x24, 0x5f, //optical character recognition
    0b00100000, 0x01, 0x24, 0x60, 0x24, 0xff, //enclosed alphanumerics
    0b00100000, 0x01, 0x25, 0x00, 0x25, 0x7f, //Box Drawing
    0b00100000, 0x01, 0x25, 0x80, 0x25, 0x9f, //Block Elements
    0b00100000, 0x01, 0x25, 0xa0, 0x25, 0xff, //geometric shapes
    0b00100000, 0x01, 0x26, 0x00, 0x26, 0xff, //miscellaneous symbols
    0b00100000, 0x01, 0x27, 0x00, 0x27, 0xbf, //dingbats
    0b00100000, 0x01, 0x27, 0xc0, 0x27, 0xef, //misc math symbols-a
    0b00100000, 0x01, 0x27, 0xf0, 0x27, 0xff, //sup-arrows-a
    0b00100000, 0x01, 0x28, 0x00, 0x28, 0xff, //braille patterns
    0b00100000, 0x01, 0x29, 0x00, 0x29, 0x7f //sup arrows b
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
        return 0;
    }

    const u16 iro = table.idRangeOffset[c_idx];
    size_t glyphIndex = 0;

    if (iro == 0) {
        glyphIndex = (w_char + table.idDelta[c_idx]) % 0x10000; //modulo 65536
    } else {
        size_t loc = (c_idx + (table.idRangeOffset[c_idx] >> 1)) + (w_char - table.startCode[c_idx]);
        if (loc >= table.segBlockSz) {
            std::cout << "tff error: failed to map glyph (cmap 4): out of range!" << std::endl;
            return 0;
        }
        glyphIndex = *(table.idRangeOffset + loc + table.idDelta[c_idx]) % 0x10000;
    }

    return glyphIndex;
}

cmap_format_4 cmap_4(ttfStream *stream) {
    if (!stream) return {};

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

    /*std::cout << "table len: " << table.table_len << std::endl;
    std::cout << "table lang: " << table.lang << std::endl;
    std::cout << "table seg Count: " << table.segCount << std::endl;*/

    if (table.segCount == 0) {
        std::cout << "ttf error: failed to read cmap4, not enough segments!" << std::endl;
        return {};
    }

    //allocate a lot of memory
    constexpr size_t nSegValues = 4;
    size_t totalBlockAlloc = table.segCount * nSegValues;
           table.nGlyphIds = (table.table_len - (totalBlockAlloc << 1)) >> 1;
           totalBlockAlloc += table.nGlyphIds; //glyphIdArray

    table.segBlockSz = totalBlockAlloc << 1; //multiply by 2 to account for everything being a u16 

    u16 *segValueBlock = new u16[totalBlockAlloc];
    ZeroMem(segValueBlock, totalBlockAlloc);

    const size_t segSz = table.segCount;
    table.segValBlock = segValueBlock;

    //assign parts of the block
    table.endCode =               segValueBlock + segSz * 0;
    table.startCode =             segValueBlock + segSz * 1;
    table.idDelta =       (i16*) (segValueBlock + segSz * 2);
    table.idRangeOffset =         segValueBlock + segSz * 3;
    table.glyphIdArr =            segValueBlock + segSz * 4;

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

    forrange(table.nGlyphIds) {
        table.glyphIdArr[i] = stream->readUInt16();
    }

    return table;
}

//cmap12
void free_cmap_format_12(cmap_format_12* c) {
    if (c->segValBlock)
        _safe_free_a(c->segValBlock);
    ZeroMem(c, sizeof(cmap_format_12));
}

cmap_format_12 cmap_12(ttfStream *stream) {
    if (!stream) return {};

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

//spacing tables and stuff
bool read_hhea(ttfStream *stream, ttfFile* f) {
    if (!stream || !f)
        return 1;

    stream->seek(f->hhea_table.off);

    const u32 version = stream->readUInt32();

    f->h_inf.ascent = stream->readFWord();
    f->h_inf.descent = stream->readFWord();
    f->h_inf.lineGap = stream->readFWord();
    f->h_inf.advanceWidthMax = stream->readUFWord();
    f->h_inf.minLeftSideBearing = stream->readFWord();

    return 0;
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
    //mode
    if (f->platform == CMap_null) {
        const size_t mapOff = f->cmap_table.off;
        const size_t rPos = stream->seek(mapOff);

        //read data from the cmap table
        const u16 version = stream->readUInt16();
        const u16 nSubTables = stream->readUInt16();

        f->platform = (enum CMapMode)stream->readUInt16();
        f->encodingId = stream->readUInt16();

        const u32 off = stream->readUInt32();
        std::cout << "Cmap Offset: " << off << std::endl;
        size_t cm_pos = stream->tell() + (off - 12);
        std::cout << "Cmap Pos: " << cm_pos << std::endl;
        stream->seek(cm_pos);
        const u16 cmap_format = stream->readUInt16();

        f->cmap_id = cmap_format;

        switch (cmap_format) {
        case 4:
            f->cmap_fmt.fmt_4 = cmap_4(stream);
            break;
        case 12:
            f->cmap_fmt.fmt_12 = cmap_12(stream);
            break;
        default:
            std::cout << "ttf error: unsupported cmap format: " << cmap_format << std::endl;
            break;
        }

        stream->seek(rPos);
    }

    switch (f->cmap_id) {
        case 4:
            if (tChar > 0xffff) {
                std::cout << "ttf warning: selected font does not support 32-bit characters!" << std::endl;
                return 0;
            }
            return decode_char_from_cmap4(f->cmap_fmt.fmt_4, (u16) tChar);
        case 12:
            return decode_char_from_cmap12(f->cmap_fmt.fmt_12, tChar);
        default:
            std::cout << "ttf error: failed to find cmap!" << std::endl;
            break;
    }

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

    std::cout << "Min Y: " << res.yMin << std::endl;
    std::cout << "Max Y: " << res.yMax << std::endl;

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

    u32 _id = getUnicodeOffset(&fStream, &f, id);
    u32 offset = getGlyphOffset(&fStream, &f, _id);
    tGlyph = read_glyph(&fStream, &f, offset);

    delete[] fBytes.dat;
    fStream.free();

    return tGlyph;
}

//extracts range data from the table thing
struct _RangeData {
    size_t nRanges = 0;
    u32 *min = nullptr;
    u32 *max = nullptr;
    bool good = false;
} extract_range_data(UnicodeRange charRange) {
    _RangeData r;

    bool found_range = false;
    i32 cur_range_id = 0, b = 0;

    constexpr size_t bMax = sizeof(uncode_range_decode) / sizeof(u8);

    std::cout << "Max: " << bMax << std::endl;

    i32 i, j;

    do {
        if (b + 2 >= bMax) {
            cur_range_id = -1;
            break;
        }

        u8 flg = uncode_range_decode[b++];
        size_t n = uncode_range_decode[b++];

        if (n == 0) {
            cur_range_id++;
            continue;
        }

        const size_t nb_per_pos = ((flg >> 4) & 3) + 1;
        const size_t nb_per_range = nb_per_pos << 1;

        if ((flg >> 7) & 1) {

        } else {
            if (cur_range_id != (u32) charRange) {
                b += n * (nb_per_pos * 2);
                continue;
            }

            std::cout << "n ranges: " << n  << " | " << nb_per_pos << std::endl;

            found_range = true;

            r.nRanges = n;
            r.min = new u32[n];
            r.max = new u32[n];

            for (i = 0; i < n; i++) {
                if (b + (nb_per_range - 1) >= bMax) {
                    _safe_free_a(r.min);
                    _safe_free_a(r.max);
                    ZeroMem(&r, 1);
                    std::cout << "error: invalid internal table!" << std::endl;
                    return r;
                }

                r.min[i] = (r.max[i] = 0);

                //decode min
                for (j = nb_per_pos - 1; j >= 0; j--)
                    r.min[i] |= (uncode_range_decode[b++] << (j << 3)) & 0xff;

                //decode max
                for (j = nb_per_pos - 1; j >= 0; j--)
                    r.max[i] |= (uncode_range_decode[b++] << (j << 3)) & 0xff;

                std::cout << "Range: " << r.min[i] << " --> " << r.max[i] << std::endl;
            }
        }
    } while (cur_range_id != (u32) charRange);

    if (cur_range_id < 0 || !found_range) {
        std::cout << "ttf error: invalid range!" << std::endl;
        return r;
    }

    r.good = true;

    std::cout << "N: " << r.nRanges << std::endl; 

    return r;
}

GlyphSet ttfParse:: GenerateGlyphSet(std::string src, UnicodeRange charRange) {
    GlyphSet gs = {
        .rangeId = charRange
    };

    file fBytes = FileWrite::readFromBin(src);

    if (!fBytes.dat || fBytes.len <= 0) {
        std::cout << "ttf error: failed to read file!" << std::endl;
        return gs;
    }

    ttfFile f;
    ttfStream fStream = ttfStream(fBytes.dat, fBytes.len);

    //extract range info
    _RangeData rd = extract_range_data(charRange);

    if (!rd.good) {
        std::cout << "ttf error: bad range" << std::endl;
        _safe_free_a(fBytes.dat);
        return gs;
    }

    //
    read_offset_tables(&fStream, &f);

    i32 r, ucode_i, tg = 0;
    gs.nGlyphs = 0;

    gs.rangeLocations = new _rLoc[rd.nRanges];
    ZeroMem(gs.rangeLocations, rd.nRanges);

    for (r = 0; r < rd.nRanges; r++) {
        gs.nGlyphs += rd.max[r] - rd.min[r];
    }

    gs.glyphs = new Glyph[gs.nGlyphs];
    ZeroMem(gs.glyphs, gs.nGlyphs);
    
    //read in the glyphs
    for (r = 0; r < rd.nRanges; r++) {
        gs.rangeLocations[r] = {
            .start = rd.min[r],
            .i = (u32) tg
        };
        
        std::cout << "gn inc: " << gs.nGlyphs << " " << rd.max[r] << " " << rd.min[r] << std::endl;

        for (ucode_i = rd.min[r]; ucode_i < rd.max[r]; ucode_i++) {
            u32 loc = getUnicodeOffset(&fStream, &f, ucode_i);
            u32 offset = getGlyphOffset(&fStream, &f, loc);
            gs.glyphs[tg++] = read_glyph(&fStream, &f, offset);
        }
    }

    return gs;
}