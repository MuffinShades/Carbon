#include "jpeg.hpp"
#include "bytestream.hpp"
#include "bitstream.hpp"
#include "filewrite.hpp"
#include "logger.hpp"

static Logger l;

constexpr size_t IMG_BLOCK_W = 8;
constexpr size_t IMG_BLOCK_H = 8;
constexpr size_t IMG_BLOCK_SZ = IMG_BLOCK_W * IMG_BLOCK_H;
constexpr size_t MAX_CODES = 0x100;
constexpr u32 APP_JPG_SIG = 0x4a464946; // "JFIF"
constexpr size_t Q_MATRIX_SIZE = IMG_BLOCK_SZ;

constexpr size_t zig_zag[] = {
    0,  1,   5,  6, 14, 15, 27, 28,
    2,  4,   7, 13, 16, 26, 29, 42,
    3,  8,  12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

//change to double if goofniess occurs
static const float idct_matrix[] = {
    0.707107f,  0.707107f,  0.707107f,  0.707107f,  0.707107f,  0.707107f,  0.707107f,  0.707107f,
    0.980785f,  0.831470f,  0.555570f,  0.195090f, -0.195090f, -0.555570f, -0.831470f, -0.980785f,
    0.923880f,  0.382683f, -0.382683f, -0.923880f, -0.923880f, -0.382683f,  0.382683f,  0.923880f,
    0.831470f, -0.195090f, -0.980785f, -0.555570f,  0.555570f,  0.980785f,  0.195090f, -0.831470f,
    0.707107f, -0.707107f, -0.707107f,  0.707107f,  0.707107f, -0.707107f, -0.707107f,  0.707107f,
    0.555570f, -0.980785f,  0.195090f,  0.831470f, -0.831470f, -0.195090f,  0.980785f, -0.555570f,
    0.382683f, -0.923880f,  0.923880f, -0.382683f, -0.382683f,  0.923880f, -0.923880f,  0.382683f,
    0.195090f, -0.555570f,  0.831470f, -0.980785f,  0.980785f, -0.831470f,  0.555570f, -0.195090f
};

enum jpg_block_id {
    jpg_block_null = 0x00, //For errors
    jpg_block_soi = 0xffd8, //Start Of Image
    jpg_block_header = 0xffe0,
    jpg_block_qtable = 0xffdb,
    jpg_block_sof = 0xffc0, //start of frame
    jpg_block_hufTable = 0xffc4,
    jpg_block_scanStart = 0xffda,
    jpg_block_eoi = 0xffd9, //End Of Image
    jpg_block_unknown = 0xffff
};

enum class huff_table_id {
    luma_DC,
    luma_AC,
    chroma_DC,
    chroma_AC,
    nHuffTables
};

struct imgBlock {
    byte *dat = nullptr;
    const size_t sz = IMG_BLOCK_SZ;
};

struct huff_table {
    size_t n_symbols;
    size_t *code_lens;
    byte inf;
    size_t table_idx;
};

struct q_table {
    byte qDest;
    byte m[Q_MATRIX_SIZE];
};

struct component_inf {
    u8 hRes = 0, vRes = 0; //horizontal and vertical resolutions
    u8 q_idx = 0; //index of quantization table
    u8 id = 0; //component id
};

struct jpg_header {
    u8 precision = 8;
    size_t w = 0, h = 0;
    u16 maxHRes, maxVRes;
    size_t nChannels = 3;
    component_inf channelInf[3];
};

struct app_header {
    u16 version;
    byte units;
    u16 densityX, densityY;
    byte thumbX, thumbY;
};

struct JpgContext {
    jpg_header header;
    app_header app_header;
    huff_table huffTables[static_cast<size_t>(huff_table_id::nHuffTables)];
};

enum class component_id {
    Unknown = 0,
    Luma = 1,
    ChromaR = 3,
    ChromaB = 2,
    I = 4,
    Q = 5
};

/**
 * 
 * decode_huf_table
 * 
 * decodes huffman table from byte stream
 * 
 * Notes about the inf:
 * 
 * 4 bits -> Tc, table class
 *      0 -> DC or lossless
 *      1 -> AC
 * 
 * 4 bits -> Th, table destination
 * 
 */
huff_table decode_huf_table(ByteStream* stream) {
    const size_t tableSz = stream->readUInt16();
    const byte ht_inf = stream->readByte();

    huff_table tab = {
        .inf = ht_inf
    };

    //process inf a bit
    const flag tableType = EXTRACT_BYTE_FLAG(ht_inf, 4);
    const size_t tablePos = ht_inf & 15;

    if (tablePos > 1) {
        std::cout << "Jpeg Warning: huffman table is out of bounds!" << std::endl;
    }

    tab.table_idx = ((tablePos << 1) | tableType);
    
    //extract the codelengths
    const size_t MAX_CODE_LENGTH = 16, MAX_CODE_VAL = 0xff;
    
    //
    tab.code_lens = new size_t[MAX_CODE_VAL];
    ZeroMem(tab.code_lens, MAX_CODE_VAL);
    
    //decode how many symbols there are per code
    byte symPerCode[MAX_CODE_LENGTH];
    
    for (auto& s : symPerCode)
        tab.n_symbols += (s = stream->readByte());
        
    //decode what symbols have what code
    size_t curCodeLen = 1;
    for (auto c : symPerCode) {
        while (c--) {
            byte tChar = stream->readByte(); //get ref character
            tab.code_lens[tChar] = curCodeLen;
        }
        curCodeLen++;
    }

    //TODO: generate the trees -> using the canonical tree thingys in balloon
    
    //
    return tab;
}

app_header decode_app_header(ByteStream* stream) {
    if (!stream)
        return {};

    IntFormat oFormat = stream->int_mode;
    stream->int_mode = IntFormat_BigEndian;

    const size_t headerLen = stream->readUInt32(), endJmp = stream->tell() + headerLen;
    const size_t JFIF_label = stream->readUInt32();

    if (JFIF_label != APP_JPG_SIG) {
        std::cout << "Jpeg Error: Invalid app header sig!" << std::endl;
        return {};
    }

    const app_header h = {
        .version = stream->readUInt16(),

    };

    stream->seek(endJmp);
    stream->int_mode = oFormat;
    return h;
}

q_table decode_q_table(ByteStream* stream) {
    const size_t tableSz = stream->readUInt16();
    q_table table;

    table.qDest = stream->readByte();

    if (table.qDest != 0 && table.qDest != 1) {
        std::cout << "Jpeg Error: Invalid quantization table destination! Expected 0 (luma) or 1 (chroma)" << std::endl;
        return table;
    }

    ZeroMem(table.m, Q_MATRIX_SIZE);

    //read in the qTable values
    u32 i, tv;

    for (i = 0; i < Q_MATRIX_SIZE; i++) {
        table.m[i] = (tv = stream->readByte());

        //all 0xff's must be preceded by a 0x00 if it isn't a header!!
        if (tv == 0xFF) {
            const byte zByte = stream->readByte();

            if (zByte != 0) {
                std::cout << "Jpeg Error: Failed to decoded quantization table. Not enough entries!" << std::endl;
                stream->stepBack(2);
                break;
            }
        }
    }

    return table;
}

static void skip_chunk(ByteStream *stream) {
    const size_t chunkSz = stream->readUInt16();
    stream->skip(chunkSz);
}

jpg_header decode_sof(ByteStream* stream) {
    if (!stream) return {};

    const size_t blockSz = stream->readUInt16();

    u32 i, resInf, cInf;

    jpg_header h = {};

    //Possible TODO: read everything as a u48 and use bit manipulation to decode the individual values (sketchy)
    h.precision = stream->readByte();
    h.h = stream->readUInt16();
    h.w = stream->readUInt16();
    h.nChannels = stream->readByte();

    //read channel info (stored in u24s)
    for (i = 0; i < h.nChannels; i++) {
        cInf = stream->readUInt24();
        component_inf I;

        resInf = (cInf >> 8) & 0xff;

        I.id = (cInf >> 16) & 0xff;
        I.q_idx = cInf & 0xff;
        I.vRes = resInf & 0xf;
        I.hRes = (resInf >> 4) & 0xf;

        h.channelInf[i] = I;
    }
}

#define JPG_GET_N_BLOCK_IN_DIR(dir) (((dir) >> 3) + (((dir) & 7) > 0))

u32 jpg_decodeIData(ByteStream* stream, JpgContext* jContext) {
    if (!stream || !jContext) {
        l.Error("Jpg Error: invalid stream or context!");
        return 1;
    }

    const size_t nXBlocks = JPG_GET_N_BLOCK_IN_DIR(jContext->header.w),
                 nYBlocks = JPG_GET_N_BLOCK_IN_DIR(jContext->header.h);

    /*
    
    How the blocks are stored:

              nXBlocks
    +--------------------------+
    | { [Yx4][Crx2][Cbx2] }...
    | ...
    |
    +--------------------------+

    */

    //loop over every image block
    size_t bx, by;

    u32 lumaRef = 0, chromaRef_r = 0, chromaRef_b = 0;

    for (by = 0; by < nYBlocks; ++by) {
        for (bx = 0; bx < nXBlocks; ++bx) {
            
        }
    }

    return 0;
}



jpg_block_id handle_block(ByteStream* stream, JpgContext* jContext) {
    if (!stream || !jContext) {
        l.Error("Jpg Error: invalid stream or context!");
        return jpg_block_null;
    }

    //look for the start of a block
    u8 blockFF;

    //look for a hex code in the form 0xFFXX, where XX is not 00
    while (
        (blockFF = stream->readByte()) != 0xFF ||
        stream->nextByte() == 0x00
    ) {
        //do nothing :D
        if (!stream->canAdv()) {
            std::cout << "Jpg Warning: Reached stream end without finding a block!!" << std::endl;
            break;
        }
    }

    //get block code
    const u16 blockId = 0xFF00 | stream->readByte();

    std::cout << "Decoded Block: " << blockId << std::endl;
    //l.LogObjHex(blockId);

    bool invalidBlock = false;
    
    switch (blockId) {
        case jpg_block_soi: {
            std::cout << "Found Stat of Image!" << std::endl;
            break;
        }
        case jpg_block_hufTable: {
            huff_table decodedTable = decode_huf_table(stream);
            break;
        }
        case jpg_block_header: {
            jContext->app_header = decode_app_header(stream);
            break;
        }
        case jpg_block_qtable: {
            q_table table = decode_q_table(stream);
            break;
        }
        case jpg_block_scanStart: {
            u32 decodeStat;
            if (decodeStat = jpg_decodeIData(stream, jContext)) {
                std::cout << "Jpeg Error: Failed to decode image data [" << decodeStat << "]" << std::endl;
            }
            break;
        }
        case jpg_block_eoi: {
            std::cout << "Found End of Image!" << std::endl;
            break;
        }
        default: {
            //thorw error or smth
            invalidBlock = true;
            skip_chunk(stream);
            break;
        }
    }
    
    return invalidBlock ? jpg_block_null : (jpg_block_id) blockId;
}

jpeg_image JpegParse::DecodeBytes(byte *dat, size_t sz) {
    if (dat == nullptr || sz <= 0) return {};
    
    ByteStream stream = ByteStream(dat, sz);

    stream.__printDebugInfo();

    JpgContext ctx;
    
    for (;;) {
        jpg_block_id c_block = handle_block(&stream, &ctx);

        const bool atStreamEnd = !stream.canAdv();

        if (c_block == jpg_block_eoi || atStreamEnd) {
            if (atStreamEnd)
                std::cout << "Jpeg Warning: reached end of stream with no end block!" << std::endl;
            break;
        }
    }
    
    return {};
}

jpeg_image JpegParse::Decode(const std::string src) {
    jpeg_image rs;
    if (src == "" || src.length() <= 0)
        return rs;
    file fDat = FileWrite::readFromBin(src);

    //error check
    if (!fDat.dat) {
        std::cout << "Failed to read jpeg..." << std::endl;
        return rs;
    }
    rs = JpegParse::DecodeBytes(fDat.dat, fDat.len);
    delete[] fDat.dat;
    return rs;
}