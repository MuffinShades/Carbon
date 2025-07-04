#include "mp3.hpp"
#include "filewrite.hpp"
#include "id3.hpp"
#include "bitstream.hpp"

constexpr u32 beId3Tag = 0x00494433; //"ID3"

enum audio_mode {
    am_single,
    am_dual,
    am_joint_stereo,
    am_stereo
};

struct mp3_decode_context {

};

struct mp3_frame_header {
    bit mpeg_version;
    u8 layer;
    bool protect;
    size_t bit_rate, freq;
    bool padding;
    audio_mode mode;
    u8 mode_ext;
    bool copyright;
    bool isACopy;
    u8 emphasis;
};

struct mp3_frame {
    mp3_frame_header header;
};

/**
 * 
 * sync_stream
 * 
 * syncs the bitstream to the mp3 frames by looking for 
 * multiple 12bit syncwords.
 * 
 * nSyncVerify -> speicifes the number of syncwords to 
 * look for the verify the alleged mp3 file is good
 * 
 */
bool sync_stream(BitStream* stream, const size_t nSyncVerify = 2) {
    if (!stream) return false;

    //start sync process
    size_t tSync, syncSeek = 0;

    for (tSync = 0; tSync < nSyncVerify;) {

        //possible sync found
        if (stream->nextByte() == 0xff) {
            size_t cBit = stream->bitTell(), backStep = 0;

            //jump to first 1 bit
            while (stream->readBit() == 1)
                stream->bitSeek(cBit - backStep++); //do bs++ instead of ++bs since we wont need to seek a bit ahead at the end then
               
            //skip to end of known bits
            stream->bitSeek(cBit + 8);

            size_t foundBits = backStep + 8;

            while (stream->readBit() == 1)
                foundBits++;

            //found a possible sync word
            if (foundBits == 12) {
                syncSeek = cBit - backStep;
                tSync++;
            }
        }

        if (!stream->canAdv())
            return false;
    }

    //seek to the sync word that was found
    stream->bitSeek(syncSeek);
}

mp3_frame_header decode_mp3_frame_header(BitStream* stream) {
    if (!stream) return {};

    //TODO: decode frame header

    return {};
}

mp3_frame decode_mp3_frame(BitStream* stream, mp3_decode_context* ctx, bool sync = true) {
    if (!stream || !ctx) return {};

    //quick stream sync
    if (sync && !sync_stream(stream, 1)) {
        std::cout << "mp3 error: failed to sync the stream!" << std::endl;
        return {};
    }

    mp3_frame frame = {
        .header = decode_mp3_frame_header(stream)
    };

    //TODO: decode frame data

    return frame;
}


/*

0: success
1: invalid params

*/
u32 decode_mp3_audio(BitStream* stream, mp3_stream* rs, mp3_decode_context* ctx) {
    if (!stream || !rs || !ctx) {
        std::cout << "mp3 error: invalid decode params!" << std::endl;
        return 1;
    }

    if (!sync_stream(stream)) {
        std::cout << "mp3 error: failed to sync the stream!" << std::endl;
        return 2;
    }

    bool pre_sync = true;

    for (;;pre_sync = false) {
        const mp3_frame decodedFrame = decode_mp3_frame(stream, ctx, pre_sync);
    }

    return 0;
}

mp3_stream Mp3Parse::DecodeBytes(byte* data, size_t sz) {
    if (!data || sz == 0)
        return {};


    BitStream stream = BitStream(data, sz);

    ID3_tag id3;

    if (stream.readUInt(3) == beId3Tag) {
        stream.stepBack(3);
        
        id3 = extract_id3_data(&stream);
    }

    mp3_stream rs;


}

mp3_stream Mp3Parse::Decode(std::string src) {
    mp3_stream rs;
    if (src == "" || src.length() <= 0)
        return rs;
    file fDat = FileWrite::readFromBin(src);

    //error check
    if (!fDat.dat) {
        std::cout << "Failed to read mp3..." << std::endl;
        return rs;
    }
    rs = Mp3Parse::DecodeBytes(fDat.dat, fDat.len);
    delete[] fDat.dat;
    return rs;
}