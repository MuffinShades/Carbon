#include "nom.hpp"
#include <type_traits>
#include <assert.h>

enum class ChunkType {
    Padd = 0x00,
    Reservation = 0x0c,
    DirectLoca = 0x01,
    OffsetLoca = 0x02,
    File = 0x03,
    RawDat = 0x04,
    CompressedDat = 0x06,
    DataPeek = 0x05,
    Directory = 0x08,
    Log = 0x09,
    IStats = 0x0a,
    Cert = 0x0b,
    Null = 0xff
};


struct AfileGenContext {
    nomfile file;
    nomsettings ns;
    ByteStream *stream;
    struct {
        u8 cidLen = 1;
        size_t nOffsetBytes = 8;
    } ext_inf;
};

template<class _Ty> void mu_swap(_Ty *a, _Ty *b) {
    const _Ty temp = *a;
    *a = *b;
    *b = temp;
}

template<class _Ty> i32 _partition(_Ty *arr, i32 (*cmp)(_Ty&, _Ty&), i32 low, i32 high) {
    _Ty p = arr[high];

    i32 i = low - 1;
    
    for (i32 j = low; j < high; j++) {
        if (cmp(arr[j], p)) {
            i++;
            mu_swap(arr + i, arr + j);
        }
    }

    mu_swap(arr + i + 1, arr + high);

    return i + 1;
}

//quick sort function
//cmp is a lambda that compares the two values given to it
// ie [](Obj a, Obj b) return i32;
//returning a 0 is equivalent to the values being equal
//returning < 0 is equivalent to value a being less then value b
//returning > 0 is equivalent to value a being greater then value b
//Ie: sort from least to greatest --> return a-b
//Ie: sort from greatest to least --> return b-a
template<class _Ty> void mu_qsort(_Ty *arr, i32 (*cmp)(_Ty&, _Ty&), size_t len, i32 _low = 0, i32 _high = 0x7fffffff) {
    if (!arr || _low >= _high)
        return;

    if (_high == 0x7fffffff) _high = len - 1;
    
    i32 p = _partition(arr, cmp, _low, _high);

    mu_qsort(arr, cmp, len, _low, p - 1);
    mu_qsort(arr, cmp, len, p + 1, _high);
}

constexpr byte dictionary_fmt_1 = 0x01;

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

struct ChunkHeader {
    ChunkType ty = ChunkType::Null;
    size_t len = 0;
    u32 checksum = 0;
};

struct Chunk {
    ChunkHeader h;
    byte *dat;
};

struct SubChunkHeader {
    byte *dat = nullptr;
    size_t len = 0;
};

i64 stream_write_chunk(AfileGenContext ctx, Chunk c, SubChunkHeader sch = {}, bool computeChecksum = false) {
    ByteStream *stream = ctx.stream;

    size_t woff = stream->tell();

    if (!stream || !c.dat || c.h.len == 0 || c.h.ty == ChunkType::Null)
        return -1;

    stream->setMode(ctx.ns.endian); //set proper endian

    //write the chunk type
    if (ctx.ext_inf.cidLen <= 8) {
        stream->writeUInt((u64)c.h.ty, ctx.ext_inf.cidLen);
    } else {
        std::cout << "Error: cid > 8 is currently not supported!" << std::endl;
        return -1;
    }

    //write the chunk length
    const size_t lg2len = fast_log2(c.h.len),
                 nbToRepCL = (lg2len >> 3) + ((lg2len & 7) > 0);

    if (nbToRepCL > 8) {
        std::cout << "Very odd error: cannot represent chunk length with more than 8 bytes!" << std::endl;
        c.h.len &= 0xffffffffffffffffULL;
        stream->writeUInt(c.h.len, 8);
    } else {
        stream->writeUInt(c.h.len, nbToRepCL);
    }

    //write check sum
    if (!computeChecksum)
        stream->writeUInt32(c.h.checksum);
    else {
        std::cout << "Warning: checksum not implemented!" << std::endl;
        stream->writeUInt32(0);
    }

    //write subchunk header
    if (sch.dat && sch.len > 0)
        stream->writeBytes(sch.dat, sch.len);

    //write chunk data
    stream->writeBytes(c.dat, c.h.len);

    return (signed) (woff & ((1ULL << 63ULL) - 1ULL));
}

#include "balloon.hpp"

Chunk genAssetDataChunk(nomasset a) {
    Chunk res = {
        .h = {
            .ty = ChunkType::Null
        }
    };

    if (!a.dat || a.len == 0)
        return res;

    res.h.checksum = a.checksum;
    res.h.len = a.len;
    res.dat = a.dat;

    if (a._side_info.storage.compression != CompressionMode::None) {
        res.h.ty = ChunkType::CompressedDat;

        switch (a._side_info.storage.compression) {
        case CompressionMode::Zlib:
            //compress the chunk data
            balloon_result cres = Balloon::Deflate(res.dat, res.h.len);

            if (!cres.data || cres.sz == 0) {
                std::cout << "Asset error: failed to compress data! Zlib failed" << std::endl;
                if (cres.data)
                    _safe_free_a(cres.data);
            }

            //set data to the compressed data
            res.dat = cres.data;
            res.h.len = cres.sz;

            break;
        default:
            std::cout << "Asset error: unknown compression method " << (u32) a._side_info.storage.compression << std::endl;
            break;
        }
    } else {
        res.h.ty = ChunkType::RawDat;
    }

    return res;
}

const Version nea_ver = {
    VerLabel::Custom,
    0,1,826
};

//prevent against goons tryna exceed 32bits in a hash
#if __nea_max_hash > 32
#error "NEA hash hard max (__nea_max_hash) exceeds 32bits!"
#endif

i32 __as_comp(nomasset &a, nomasset &b) {
    if (!a._side_info.good)
        return -1;

    if (!b._side_info.good)
        return 1;

    asset_id aid = a._side_info.id,
             bid = b._side_info.id;

    if (aid.idp_lens[0] != bid.idp_lens[0])
        return a.len - b.len;

    i32 i = 0;

    while (aid.id_dat[i] == bid.id_dat[i])
        i++;

    return ((i32)aid.id_dat[i]) - ((i32) bid.id_dat[i]);
}

void computeAssetPathHashes(nomfile &f) {
    i32 i,j;

    nomasset *fa = f.assets;
    asset_id fid;
    char *idat;
    size_t ilen;

    if (!fa || f.nassets == 0)
        return;

    for (i = 0; i < f.nassets; i++) {
        fid = fa->_side_info.id;
        
        if (!fid.id_dat || !fid.idp_lens || fid.nParts == 0 || fid.p_hash)
            continue;

        fa->_side_info.id.p_hash = new u32[fid.nParts];
        idat = fid.id_dat;

        for (j = 0; j < fid.nParts; j++) {
            ilen = fid.idp_lens[j];
            fa->_side_info.id.p_hash[j] = compute_basic_hash_32_inline(32, idat, ilen);
            idat += ilen;
        }

        fa++;
    }
}

struct dirEntry {
    asset_id id;
    size_t off;
};

struct directorGenContext1 {

};

void _addDirectorFmt1(directorGenContext1 ctx) {

}

//will write the primary directory and all sub directory onto the end of the given stream
//will also return the offset of the primary directory in the stream
i64 genDirectoryFmt1(ByteStream *s, nomfile f, nomsettings ns) {
    static_assert(__nea_max_hash <= 32, "NEA hash hard max (__nea_max_hash) exceeds 32bits!");

    if (!s)
        return -1;

    size_t hashBits = fast_log2(f.nassets);
    hashBits = ((hashBits >> 3) + ((hashBits & 7) > 0)) << 3;

    if (ns.maxHashBits > __nea_max_hash) ns.maxHashBits = __nea_max_hash;

    if (hashBits > ns.maxHashBits)
        hashBits = ns.maxHashBits;

    s->writeByte(dictionary_fmt_1);

    //do some sizing calculations
    nomasset *fa = f.assets;

    if (!fa || f.nassets == 0)
        return -1;

    computeAssetPathHashes(f);
    mu_qsort<nomasset>(fa, &__as_comp, f.nassets);

    i32 i;
    nomasset na;

    size_t nbll = 0; //num bits in a label len

    struct s_sector {
        size_t nss; //num sub sectors
        size_t *llen; //length of items in the sector
        u64 *off; //offsets
    };

    struct s_subsec {
        size_t ne; //num entries
        char *labels; //label for each entry (kinda form of shared memory)
        u64 *off; //offsets
    };

    //create da hash table
    const size_t hashEntryBytes = hashBits >> 3;
    const size_t hz = f.nassets * hashEntryBytes;
    byte *hashTable = new byte[hz];
    ZeroMem(hashTable, hz);

    size_t maxLen = 0, n_unqLens = 1, uLen = fa[0]._side_info.id.idp_lens[0];

    size_t l;

    for (i = 0; i < f.nassets; i++) {
        na = *fa;

        if (!na.dat || na.len == 0) {
            if (ns.delBlankAssets)
                continue;
            
            continue; //uhh.. :3
        }

        l = na._side_info.id.idp_lens[0];

        nbll = mu_max(nbll, fast_log2(l));
        maxLen = mu_max(maxLen, l);

        if (l != uLen) {
            uLen = l;
            n_unqLens++;
        }

        fa++;
    }

    s_sector *p_sectors = new s_sector[n_unqLens];

    const size_t hashSz = 1 << hashBits;
    
    //compute the ideal int size for the offsets

    for (i = 0; i < f.nassets; i++) {
        na = *fa;

        if (!na.dat || na.len == 0) {
            if (ns.delBlankAssets)
                continue;
            
            continue; //uhh.. :3
        }

    }
/*

//faster log functions that are also aligned for certain bases
#define __log_def(align) {  \
    long c = 0;              \
                            \
    while (val >>= align)   \
        c++;                \
                            \
    return c;               \
}

static inline long fast_log2(long val) __log_def(1)

#include <iostream>

int main()
{
    long n = 25;
    long hb = 4;
    
    const long hsz = 1 << hb;
    
    long v,x = 0;
    
    do {
        x++;
        v = fast_log2(n * x * 3);
        v = (v >> 3) + ((v & 7) > 0);
        std::cout << "V: " << v << " | x: " << x << " | " << (n * 3 * x) << " | " << fast_log2(n * x * 3) << std::endl;
    } while(x < v && x < 8);
    
    std::cout << "N Bytes: " << x << std::endl;
    
    return 0;
}

*/

    u64 v,x = 0;
    
    do {
        x++;
        v = fast_log2(f.nassets * x * 3);
        v = (v >> 3) + ((v & 7) > 0);
        //std::cout << "V: " << v << " | x: " << x << " | " << (n * 3 * x) << " | " << fast_log2(n * x * 3) << std::endl;
    } while(x < v && x < 8);

    //complete the whole table sub header thing
    const size_t hls = x;

    s->writeByte(hls);
    s->writeByte(nbll >> 3);
    s->writeByte(hashBits);

    //write the hash table
    s->writeBytes(hashTable, hz);

    

    //write all of the sectors
}

void WriteToFile(std::string opath, nomfile f, nomsettings ns) {
    if (opath.length() == 0 || !f.assets || f.nassets == 0)
        return;

    //stream
    ByteStream s = ByteStream();

    //gen context
    AfileGenContext actx;

    if (ns.chunk_id_len > 8) ns.chunk_id_len = 8; //can techinically be 16 bytes but that would require some extra code i dont feel like writing
    if (ns.chunk_id_len < 1) ns.chunk_id_len = 1;
    actx.ext_inf.cidLen = ns.chunk_id_len;
    actx.stream = &s;
    actx.file = f;
    actx.ns = ns;

    constexpr size_t nOffsetBytes = 8;

    actx.ext_inf.nOffsetBytes = nOffsetBytes;

    //write first half of primary file header
    s.writeBytes((byte*) const_cast<char*>(fih), 3); //fsig
    s.writeByte((byte) AssetTy::Generic);            //subformat
    stream_write_version(&s, nea_ver);
    byte unicorn_byte = ((((byte) ns.endian) & 1) << 7) | ((((byte) ns.defISz) & 3) << 5) | (((nOffsetBytes - 1) & 7) << 1);
    byte cidlb = (ns.chunk_id_len - 1);

    //do some pre-asset analysis
    nomasset na; nomasset *fa;

    i32 i, j;

    //write second half of primary file header
    s.writeByte((0xef + cidlb) & 0xff); //chunk id len

    SubChunkHeader sch;

    constexpr size_t maxCDatHeaderLen = 10; //9 bytes for uncompressed length + 1 byte for compression format

    sch.dat = new byte[maxCDatHeaderLen];

    //write all the assets first
    for (i = 0; i < f.nassets; i++) {
        na = *fa;

        if (!na.dat || na.len == 0)
            continue;

        Chunk chonk = genAssetDataChunk(na);

        switch (chonk.h.ty) {
        case ChunkType::RawDat:
            stream_write_chunk(actx, chonk);
        case ChunkType::CompressedDat:
            //configure the sch
            //uncompressed length
            auto lg2l = (signed) fast_log2(na.len),
                 nbl = (signed) ((lg2l >> 3) + ((lg2l & 7) > 0));
            if (nbl > maxCDatHeaderLen) {
                std::cout << "asset warning: nbl computation is sus" << std::endl;
                nbl = 8;
            }
            sch.len = 2 + nbl;
            byte *sdat = sch.dat;
            *sdat++ = nbl & 0xff;
            j = na.len;
            if (ns.endian == IntFormat_BigEndian) endian_swap(j, nbl);
            do {
                *sdat++ = j & 0xff;
                j >>= 8;
            } while (--nbl > 0);

            //compression format
            *sdat++ = (u8) na._side_info.storage.compression;

            //write the chunk
            //note: data is already compressed in genAssetDataChunk so no need to compress it here
            stream_write_chunk(actx, chonk, sch);
            s.writeUInt32(0); //append the uncompressed checksum at the end
        break;
        default:
            std::cout << "warning: invalid chunk type encountered!" << std::endl;
            break;
        }
    }

    //free the sch and its data
    _safe_free_a(sch.dat);
    sch.len = 0;

    //s.writeByte(unicorn_byte);
    //TODO: add stream functions to restore endians

    //now create the whole directory of le assets


    //write to the file
    FileWrite::writeToBin(opath, s.getBytePtr(), s.size());
    s.free();
}

#include "json.hpp"

nomfile omn::GenNomFileFromJson(std::string jsonPath) {
    nomfile res;

    res.assets = nullptr;
    res.nassets = 0;

    if (jsonPath.length() == 0)
        return res;

    file jf = FileWrite::readFromBin(jsonPath);

    if (!jf.dat || jf.len == 0) {
        if (jf.dat) _safe_free_a(jf.dat);
        return res;
    }

    JStruct fStruct = jparse::parseStr((const char*) const_cast<const byte*>(jf.dat), jf.len);

    for (JToken tok : fStruct.body) {
        if (tok.ty == JERR_INVALID_TOK)
            continue;

        tok.body->
    }

    _safe_free_a(jf.dat);
};