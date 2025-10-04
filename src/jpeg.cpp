#include "jpeg.hpp"
#include "bytestream.hpp"
#include "bitstream.hpp"
#include "filewrite.hpp"
#include "logger.hpp"
#include "balloon.hpp"

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
constexpr f32 idct_matrix[] = {
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

enum class huff_table_id_fragment {
    luma = 0b000,
    chroma = 0b010,
    AC = 0b101,
    DC = 0b100
};

enum class huff_table_id {
    luma_DC,
    luma_AC,
    chroma_DC,
    chroma_AC,
    nHuffTables,
    unknown = 0xffff
};

/*

component -> luma or chroma
current -> AC or DC

*/
inline static huff_table_id getTableId(huff_table_id_fragment component, huff_table_id_fragment current) {
    const u32 id = ((u32)component & 3) | ((u32)current & 3);
    if (id < (u32)huff_table_id::nHuffTables)
        return (huff_table_id)id;
    else
        return huff_table_id::unknown;
}

struct imgBlock {
    byte *dat = nullptr;
    const size_t sz = IMG_BLOCK_SZ;
};

struct huff_table {
    size_t n_symbols;
    u32 *code_lens = nullptr;
    byte inf;
    huff_table_id table_id;
    huffman_node* root = nullptr;
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
    u16 maxHRes = 0, maxVRes = 0;
    size_t nChannels = 3;
    component_inf channelInf[3];
    size_t channelDataSz[3];
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

    const u32 id = ((tablePos << 1) | tableType);

    if (id < (u32) huff_table_id::nHuffTables)
        tab.table_id = (huff_table_id)id;
    else 
        tab.table_id = huff_table_id::unknown;
    
    //extract the codelengths
    const size_t MAX_CODE_LENGTH = 16, MAX_CODE_VAL = 0xff;
    
    //
    tab.code_lens = new u32[MAX_CODE_VAL];
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
    tab.root = Huffman::BitLengthsToTree(tab.code_lens, MAX_CODE_VAL, MAX_CODE_VAL);
    
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

        h.maxHRes = mu_max(h.maxHRes, I.hRes);
        h.maxVRes = mu_max(h.maxVRes, I.vRes);
        h.channelInf[i] = I;
    }

    return h;
}

#define JPG_GET_N_BLOCK_IN_DIR(dir) (((dir) >> 3) + (((dir) & 7) > 0))

i32 ConvertCode(const u32 code, const u32 extra) {
    const i32 l = 1 << (code - 1);

    if (extra >= l)
        return extra;
    else
        return ((signed) extra) + ((-1 << l) + 1);
}

void jpg_transform_raw_block(i32* block_buffer) {
    //perform IDCT

    //then rescale from -128 -> 127 to 0 -> 255
}

void jpg_build_block(JpgContext* jContext, BitStream* b_stream, huff_table_id_fragment component, i32* block_buffer, i32& old_dc_coeff) {
    if (!jContext || !block_buffer || !b_stream) return;

    const huff_table_id dc_table_select = getTableId(component, huff_table_id_fragment::DC);

    if (dc_table_select == huff_table_id::unknown) {
        std::cout << "Jpeg error! Cannot decode invalid component: " << (u32)component << std::endl;
    }

    //decode dc coeff and update ref coeff
    const u32 dc_code = Huffman::DecodeSymbol(b_stream, jContext->huffTables[(size_t)dc_table_select].root),
              dc_extra = b_stream->readBits(dc_code);
    const i32 dc_coeff = ConvertCode(dc_code, dc_extra) + old_dc_coeff;

    old_dc_coeff = dc_coeff;

    block_buffer[zig_zag[0]] = dc_coeff; //TODO: scale with quantization matrix

    //decode ac coeffs
    const huff_table_id ac_table_select = getTableId(component, huff_table_id_fragment::AC);

    huffman_node* ac_tree = jContext->huffTables[(size_t)ac_table_select].root;

    size_t p = 1;
    u32 ac_code, ac_extra, ac_coeff;

    while (p < 64) {
        ac_code = Huffman::DecodeSymbol(b_stream, ac_tree);

        //codes > 15 indicate zero skip then a code
        if (ac_code > 15)
            p += ac_code >> 4;

        //
        ac_extra = b_stream->readBits(ac_code & 0xf);

        if (p < 64) {
            ac_coeff = ConvertCode(ac_code, ac_extra);
            //simply convert zig zag here
            block_buffer[zig_zag[p++]] = ac_coeff; //TODO: scale with quantization matrix
        }
    }

    //
}

u32 jpg_decode_channel_blocks(JpgContext* jContext, byte **blockData, u8 channelSelect) {
    if (!blockData || !jContext) return 1;

    const u32 h_res = jContext->header.channelInf[channelSelect].hRes,
              v_res = jContext->header.channelInf[channelSelect].vRes;

    u32 h, v;
    
    for (v = 0; v < v_res; v++) {
        for (h = 0; h < h_res; h++) {

        }
    }

    return 0;
}

u32 jpg_decodeIData(ByteStream* stream, JpgContext* jContext) {
    if (!stream || !jContext) {
        l.Error("Jpg Error: invalid stream or context!");
        return 1;
    }

    if (jContext->header.w == 0 || jContext->header.h == 0) {
        l.Error("Jpg Error: image dimensions are zero or no header has been found!");
        return 2;
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

    byte **blockData = new byte*[jContext->header.nChannels];

    forrange(jContext->header.nChannels) {
        f32 h_ratio = (f32) jContext->header.channelInf[i].hRes / (f32) jContext->header.maxHRes,
            v_ratio = (f32) jContext->header.channelInf[i].vRes / (f32) jContext->header.maxVRes;
        const size_t nb = (size_t)(h_ratio * nXBlocks) * (size_t)(v_ratio * nYBlocks);
        blockData[i] = new byte[nb];
        ZeroMem(blockData, nb); //TODO: maybe remove the zeromem since it isn't really needed
        jContext->header.channelDataSz[i] = nb;
    }

    for (by = 0; by < nYBlocks; by++) {
        for (bx = 0; bx < nXBlocks; bx++) {
            
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
            std::cout << "Found Start of Image!" << std::endl;
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
        case jpg_block_sof: {
            jContext->header = decode_sof(stream);
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