#include "jpeg.hpp"
#include "bytestream.hpp"
#include "bitstream.hpp"

constexpr size_t IMG_BLOCK_W = 8;
constexpr size_t IMG_BLOCK_H = 8;
constexpr size_t IMG_BLOCK_SZ = IMG_BLOCK_W * IMG_BLOCK_H;
constexpr size_t MAX_CODES = 0x100;

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

struct imgBlock {
    byte *dat = nullptr;
    const size_t sz = IMG_BLOCK_SZ;
};

struct huff_table {
    size_t n_symbols;
    size_t *code_lens;
    byte inf;
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
huff_table decode_huf_table(ByteStream *stream) {
    const size_t tableSz = stream->readUInt16();
    const byte ht_inf = stream->readByte();
    
    huff_table tab = {
        .inf = ht_inf
    };
    
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

static void skip_chunk(ByteStream *stream) {
    const size_t chunkSz = stream->readUInt16();
    stream->skip(chunkSz);
}

jpg_block_id handle_block(ByteStream *stream) {
    const u16 blockId = stream->readUInt16();
    
    switch (blockId) {
        case jpg_block_soi: {
            
            break;
        }
        case jpg_block_hufTable: {
            huff_table decodedTable = decode_huf_table(stream);
            break;
        }
        default: {
            //thorw error or smth
            skip_chunk(stream);
            break;
        }
    }
    
    return jpg_block_null;
}

jpeg_image JpegParse::DecodeBytes(byte *dat, size_t sz) {
    if (dat == nullptr || sz <= 0) return {};
    
    ByteStream stream = ByteStream(dat, sz);
    
    handle_block(&stream);
    
    return {};
}