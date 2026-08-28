#include "nom.hpp"
#include "muvec.hpp"
#include <type_traits>
#include <assert.h>

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

constexpr size_t nOffsetBytes = 8;

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
    size_t len = 0, writeLen = 0;
    u32 checksum = 0;

    bool sepLen = false;
};

struct Chunk {
    ChunkHeader h;
    byte *dat;
};

struct SubChunkHeader {
    byte *dat = nullptr;
    size_t len = 0;
};

struct PostChunkData {
    byte *dat = nullptr;
    size_t len = 0;
};

i64 stream_write_chunk(AfileGenContext ctx, Chunk c, SubChunkHeader sch = {}, PostChunkData pcd = {}, bool computeChecksum = false) {
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
    if (c.h.sepLen) {
        const size_t lg2len = fast_log2(c.h.writeLen),
                 nbToRepCL = (lg2len >> 3) + ((lg2len & 7) > 0);

        if (nbToRepCL > 8) {
            std::cout << "Very odd error: cannot represent chunk length with more than 8 bytes!" << std::endl;
            c.h.writeLen &= 0xffffffffffffffffULL;
            c.h.len &= 0xffffffffffffffffULL;
            stream->writeUInt(c.h.writeLen, 8);
        } else {
            stream->writeUInt(c.h.writeLen, nbToRepCL);
        }
    } else {
        const size_t lg2len = fast_log2(c.h.len),
                 nbToRepCL = (lg2len >> 3) + ((lg2len & 7) > 0);

        if (nbToRepCL > 8) {
            std::cout << "Very odd error: cannot represent chunk length with more than 8 bytes!" << std::endl;
            c.h.len &= 0xffffffffffffffffULL;
            stream->writeUInt(c.h.len, 8);
        } else {
            stream->writeUInt(c.h.len, nbToRepCL);
        }
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

    if (pcd.dat && pcd.len > 0)
        stream->writeBytes(pcd.dat, pcd.len);

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

            std::cout << "Compressed len: " << cres.sz << " | " << res.h.len << std::endl;

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

struct dirEntry {
    asset_id id;
    size_t off;
    u32 hash = 0;
    u64 hash64 = 0;
    i64 woffAt = -1;
    i32 link = -1;
};

struct dirInf {
    size_t nbll = 0;
    size_t nUnqLens = 0;
};

struct directoryGenContext1 {
    dirEntry *entries = nullptr;
    size_t nEntries = 0;
    nomsettings ns;
    dirInf dInf;
    ByteStream *s;
    size_t pSelect = 0;
    struct {
        void **dat = nullptr;
        size_t ndat = 0;
    } junk; //all junk will be freed when context is deleted
    //this will just include ever entry's id data since it is spliced and what not and junk will be generated
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

i32 __de_comp(dirEntry &a, dirEntry &b) {
    asset_id aid = a.id,
             bid = b.id;

    if (a.hash != b.hash)
        return a.hash - b.hash;

    if (aid.idp_lens[0] != bid.idp_lens[0])
        return aid.idp_lens[0] - bid.idp_lens[0];

    i32 i = 0;

    while (aid.id_dat[i] == bid.id_dat[i])
        i++;

    return ((i32)aid.id_dat[i]) - ((i32) bid.id_dat[i]);
}

void computeAssetPathHashes(dirEntry *e, size_t n) {
    i32 i,j;

    dirEntry *fa = e;
    asset_id fid;
    char *idat;
    size_t ilen;

    if (!fa || n == 0)
        return;

    for (i = 0; i < n; i++) {
        fid = fa->id;
        
        if (!fid.id_dat || !fid.idp_lens || fid.nParts == 0 || fid.p_hash)
            continue;

        fa->id.p_hash = new u32[fid.nParts];
        idat = fid.id_dat;

        for (j = 0; j < fid.nParts; j++) {
            ilen = fid.idp_lens[j];
            fa->id.p_hash[j] = compute_basic_hash_32_inline(32, idat, ilen);
            idat += ilen;
        }

        fa++;
    }
}

#define _NOM_MAKE_FMT_BYTE(osign, fmt) (((osign) << 7) | (((byte)(fmt)) & 127))

//will write the primary directory and all sub directory onto the end of the given stream
//will return the offset of the directory in the stream
//TODODODODOTOTOTODTODOTODTODOTDOTDOTODTODOTTODO: process the ids of the assets backwards instead of forwards or something since we need to properly note the offsets (or add something where the stream can just jump and write the offset in the entry ex-post-facto)
//TODO: also make sure the recurssion doesn't exceed the stack size
//TODO: only sort for the first iteration since the whole reshuffling thing for each consecutive call shouldn't scramble the entries; they should remained properly sorted
//TODO: compute sha hashes to ensure that no entry shares the same id
//      --> this can be done in the parent function that calls this recursion algorithm
//TODO: add a function to properly add junk to ctx.junk within the entries and ids that would need to be freed at the end
i64 _addDirectoryFmt1(directoryGenContext1 ctx, size_t maxFdatOff = 0, i64 writeOffAt = -1) {
    static_assert(__nea_max_hash <= 32, "NEA hash hard max (__nea_max_hash) exceeds 32bits!");

    ByteStream *s = ctx.s;

    if (!s || ctx.nEntries == 0 || !ctx.entries)
        return -1;

    const i64 rePos = (signed) s->tell() + 1; //position that'll be returned

    //write the offset real quick in the prev dir
    if (writeOffAt > 0) {
        std::cout << "writing woff at " << writeOffAt << " witha  value of " << rePos << std::endl;
        if (s->size() > writeOffAt) {
            s->seek(writeOffAt);
            s->writeUInt(rePos, nOffsetBytes);
            s->seek(rePos);
        } else {
            std::cout << "failed to write offset for position: " << writeOffAt << std::endl;
        }
    }

    //write dictionary format
    constexpr bit dictionary_offset_sign = 1; //positive hash table offsets
    s->writeByte(_NOM_MAKE_FMT_BYTE(dictionary_offset_sign, dictionary_fmt_1));

    std::cout << "FMT Byte: " << _NOM_MAKE_FMT_BYTE(dictionary_offset_sign, dictionary_fmt_1) << std::endl;

    //compute hash sizing
    size_t hashBits = mu_max(1, fast_log2(ctx.nEntries));
    //hashBits = ((hashBits >> 3) + ((hashBits & 7) > 0)) << 3;

    if (ctx.ns.maxHashBits > __nea_max_hash) ctx.ns.maxHashBits = __nea_max_hash;

    if (hashBits > ctx.ns.maxHashBits)
        hashBits = ctx.ns.maxHashBits;

    //do some sizing calculations
    dirEntry *fa = ctx.entries;

    computeAssetPathHashes(ctx.entries, ctx.nEntries);
    mu_qsort<dirEntry>(fa, &__de_comp, ctx.nEntries);

    i32 i;
    dirEntry na;

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

    size_t maxLen = 0, n_unqLens = 1, uLen = fa[0].id.idp_lens[0], 
           maxSuSz = 0; //max entries in a sub sector

    size_t l,m=0;
    size_t nNextE = 0;

    for (i = 0; i < ctx.nEntries; i++) {
        na = *fa;

        l = na.id.idp_lens[0];

        nbll = mu_max(nbll, fast_log2(l));
        if ((nbll & 7) != 0) nbll += 8;

        maxLen = mu_max(maxLen, l);

        m++;
        if (l != uLen) {
            maxSuSz = mu_max(maxSuSz, m); m = 0;
            uLen = l;
            n_unqLens++;
        }

        //compute the hash for each entry
        fa->hash = compute_basic_hash_32(hashBits, fa->id.id_dat, fa->id.idp_lens[0]);
        fa->hash64 = compute_basic_hash_64(63, fa->id.id_dat, fa->id.idp_lens[0]);

        //get a count of number of entries that'll pass on
        if (na.id.nParts > 1)
            nNextE++;

        fa++;
    }

    //compute hls
    u64 v,x = 0;
    do {
        x++;
        v = fast_log2(ctx.nEntries * x * 3);
        v = (v >> 3) + ((v & 7) > 0);
    } while(x < v && x < 8);
    size_t hls = x;

    std::cout << "HLS: " << hls << " | NBLL: " << nbll << " | HBITS: " << hashBits << std::endl;
    
    //fmt1 header
    s->writeByte(hls);
    s->writeByte(nbll >> 3);
    s->writeByte(hashBits);

    //write the hash table
    //create da hash table
    const size_t hashSz = 1 << hashBits;
    const size_t hz = hls * hashSz;

    std::cout << "Skipping: " << hz << " bytes for hash..." << std::endl;

    byte *hashTable = new byte[hz];
    ZeroMem(hashTable, hz);
    const size_t hashFPos = s->tell() + 1; //seek to here and then write the proper hash table once all sectors are written and calculated
    s->skip(hz); //reserve area to hash table we will write here later
    const size_t hashEPos = s->tell() + 1; //end pos of hash which will be the reference point for the offsets of the sectors

    std::cout << "Hash poses: " << hashFPos << " ==> " << hashEPos << std::endl;

    //write all of the sectors
    if (maxSuSz == 0) {
        if (m > 0) {
            maxSuSz = m;
        } else {
            std::cout << "nom error: max extries in a sub-sector was 0" << std::endl;
            return -1;
        }
    }

    struct su_inf {
        i64 off;
        size_t len;
    };

    su_inf *us_off = new su_inf[n_unqLens];
    i64 *off_stack = new i64[maxSuSz], *ofc = off_stack;
    i32 u = 0, ngu = u;

    size_t ofo = 0; //off stack offset

    uLen = mu_i_infinity_64;

    i32 j, freeLink = 0;

    auto _off_stack_push = [&](i64 v) -> void {
        if (++ofo > maxSuSz) {
            std::cout << "nom error: failed to push to off stack!" << std::endl;
            return;
        }
        *ofc++ = v;
    };

    #define _off_stack_ptr_pop() (*--ofc); ofo--
    #define _off_stack_reset() (ofc = off_stack); ofo=0
    
    //create the sectors and sub-sectors
    //write sub-sectors
    /*
    
    start --> index of starting entry
    nes --> number of total entries in the whole sector / subsector group
    
    */
    auto writeSubsectorGroup = [&](i32 start, size_t nes) -> void {
        _off_stack_reset();
        fa = ctx.entries + start;
        uLen = fa->id.idp_lens[0]; //start at 0

        const size_t end = nes + start;

        dirEntry ent, lent, la;

        i32 li = start; //last i that begins the whole sub sector

        std::cout << "Writing subsectors for: " << start << " >>to>> " << end << std::endl;

        bool at_last;

        for (i = start; i < end; i++) {
            na = *fa;
            if (i > start) la = *(fa-1);
            l = na.id.idp_lens[0];
            at_last = i == end-1;

            if (l != uLen || at_last) {
                if (at_last) _off_stack_push(s->tell() + 1);
                const auto sl = uLen;
                uLen = l;

                //u-check
                if (u >= n_unqLens || u < 0) {
                    std::cout << "nom error: large u value: " << u << std::endl;
                    u = n_unqLens;
                    break;
                }

                //write the sub-sector
                std::cout << "upush: " << s->tell() << " \\ " << sl << " @ U = " << u << std::endl;

                us_off[u].off = s->tell() + 1;
                us_off[u].len = sl; //note the offset of the subsector in order to write it in the sector

                u++;

                //note: value of u may be diff after this point

                s->writeByte(0); //flags
                s->writeUInt(ofo, hls); //n entries (hls byte uint)

                i64 off;

                const auto sie = ofo + li;

                for (j = li; j < sie; j++) {
                    off = _off_stack_ptr_pop();

                    if (off < 0) {
                        std::cout << "nom warning: offset value was < 0" << std::endl;
                        continue;
                    }

                    //write the entry
                    ent = ctx.entries[j];

                    //skip duplicate entry but still link it
                    if (j > li) {
                        lent = ctx.entries[j-1];

                        if (lent.hash64 == ent.hash64 && mu_safe_streq(lent.id.id_dat, ent.id.id_dat, lent.id.idp_lens[0], ent.id.idp_lens[0])) {
                            ctx.entries[j].link = lent.link;
                            ctx.entries[j].woffAt = -1; //set the offset to -1 in order to not have multiple modifications
                            continue;
                        }
                    }

                    //write entry and create a new link
                    ctx.entries[j].link = freeLink++;
                    s->writeBytes(reinterpret_cast<byte*>(ent.id.id_dat), ent.id.idp_lens[0]);
                    ctx.entries[j].woffAt = s->tell() + 1;

                    std::cout << sl << " | ";
                    mu_strPrint(ent.id.id_dat, sl);

                    std::cout << "offffffffff: " << ent.off << std::endl;
                    s->writeUInt(ent.id.nParts <= 1 ? ent.off : 0xffffffffffffffffULL, nOffsetBytes); //write a 0 as a place holder so the write off at can modify later
                }
            
                _off_stack_reset();
                li = i;

                if (at_last) continue;
            }

            //if (u < 0) {
            //    std::cout << "nom warning: weird u value: " << u << std::endl;
            //    u = 0;
            //}

            //////No code accessing p_sector above here///////
            _off_stack_push(s->tell() + 1);

            //last good u for debug / handling debug reasons
            ngu = u;
        }
    };

    //TODO: ACTUALLY WRITE THE HASH TABLE!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //nah you already did this dip shit

    ///////////////////////////////
    u32 lHash = ctx.entries[0].hash;
    dirEntry ce;

    size_t nEForHash = 1;
    i32 secStart = 0;

    //write main sectors
    for (i = 0; i < ctx.nEntries; i++) {
        ce = ctx.entries[i];

        if (lHash != ce.hash || i == ctx.nEntries - 1) {
            writeSubsectorGroup(secStart, nEForHash); //write the subsectors

            //number of subsectors is stored in u
            const i32 nSubSec = u;

            if (nSubSec < 0) {
                std::cout << "failed to write format 1 sector: invalid number of sub-sectors" << std::endl;
                continue;
            }

            //sector location check
            size_t secPos = s->tell() + 1;

            if (secPos < hashEPos) { //TODO: if what byte the end of hash means changes, then this must change too (to <= instead of <)
                s->end();
                secPos = s->tell() + 1;

                if (secPos < hashEPos) {
                    std::cout << "error hash is poorly positied" << std::endl;
                    continue;
                }
            }

            //write into the hash table
            if (lHash < hashSz) {
                const size_t ret = s->seek(hashFPos + hls * lHash);
                s->writeUInt(secPos, hls);
                s->seek(ret);
            } else {
                std::cout << "error: sector hash is invalid | hash: " << lHash << std::endl;
                continue;
            }

            //ensure that hls is accurate
            {
                u32 chls = hls;
                if ((chls = (fast_log2(secPos - hashEPos) >> 3)) > hls) {
                    hls = chls;
                    std::cout << "nom info/minor warning: hls was adjusted to: " << hls << std::endl;
                }
            }

            //write the sector
            s->writeByte(0); //write 0 for the offset sign of -1
            s->writeUInt(nSubSec, hls);

            std::cout << "writing " << nSubSec << " subsectors..." << std::endl;

            for (j = 0; j < nSubSec; j++) { //write the length-offset pairs
                s->writeUInt(us_off[j].len, nbll);

                const i64 uoff = us_off[j].off;

                std::cout << "UACCESS: " << j << " | uval" << uoff << " \\ " << us_off[j].len << std::endl;

                if (uoff < 0 || uoff > mu_ui_infinity_32) {
                    std::cout << "failed to write sub sector offset: " << uoff << " is not a valid offset" << std::endl;
                    s->writeUInt32(0);
                } else {
                    s->writeUInt32((u32) uoff);
                }

                //s->writeByte(0xee);s->writeByte(0xee);s->writeByte(0xee);s->writeByte(0xee);
            }

            std::cout << "hash write..." << std::endl;

            if (lHash >= hashSz || !hashTable) {
                std::cout << "failed to log offset of sector in hash table! hash was: " << lHash << std::endl;
            } else {
                const size_t hoff = lHash * hls;
                byte *htBase = hashTable + hoff;
                for (j = ((hls-1) << 3); j >= 0; j -= 8)
                    *htBase++ = ((secPos - hashEPos) >> j) & 0xff;
            }

            std::cout << "hash write done" << std::endl;

            //prepare values for next sector write thingy
            lHash = ce.hash;
            nEForHash = 1;
            secStart = i;
        } else 
            nEForHash++;
    }

    std::cout << "freeing stuff" << std::endl;

    //todo: free memory
    _safe_free_a(us_off);
    _safe_free_a(off_stack);

    std::cout << "freed stuff" << std::endl;

    //modify context and call this function again to write other tables
    const size_t onEntries = ctx.nEntries;

    if (nNextE == 0 || onEntries == 0)
        return rePos;

    j = 0; //j will be the insert position of the entry

    dirEntry la;

    ctx.nEntries = 0;

    i64 lastWoff = ctx.entries[0].woffAt;

    std::cout << "preparing next write" << std::endl;

    for (i = 0; i < onEntries; i++) {
        na = ctx.entries[i];

        if (i > 0 && na.link != la.link) {
            //if the entries are not linked then start the process
            std::cout << " WOAH: " << la.woffAt << std::endl;
            _addDirectoryFmt1(ctx, maxFdatOff, lastWoff); //repeat le process

            //effectivley an entry stack clear cmd
            j = 0; //reset position of entries for next section
            ctx.nEntries = 0;
            lastWoff = na.woffAt;
        }

        cmod_proc:

        if (na.id.nParts <= 1)
            continue; //skip since nothing left to pass on
        
        //effectivly a push to the entry stack
        na.id.nParts--; //decrement number of parts
        na.id.id_dat += *na.id.idp_lens; //go to beginning of the next id
        na.id.idp_lens++; //go to the next length
        na.id.p_hash++; //go to next phash
        na.link = -1; //reset the link
        ctx.nEntries++;

        //insert the entry properly (this whole thing should work since i should always trail ahead of j, and thus, j will never overwrite a non-processed entry)
        ctx.entries[j++] = na;
        la = ctx.entries[i];
    }

    _addDirectoryFmt1(ctx, maxFdatOff, lastWoff); //flush out whatever is left if anything

    return rePos;
}

i64 WriteDirectoryFormat1(ByteStream *stream, nomfile f, nomsettings ns) {
    if (!f.assets || f.nassets == 0) {
        std::cout << "nom error: invalid file for writing directory fmt1" << std::endl;
        return -1;
    }

    directoryGenContext1 ctx;

    ctx.ns = ns;
    ctx.nEntries = f.nassets;
    ctx.s = stream;

    //covert assets into entries
    ctx.entries = new dirEntry[ctx.nEntries];

    if (!ctx.entries) {
        std::cout << "nom error: failed to alloc entries!" << std::endl;
        return -1;
    }

    i32 i, j = 0; dirEntry *fa;

    const size_t ne = ctx.nEntries;

    for (i = 0; i < ne; i++) {
        fa = ctx.entries + j;

        asset_id id = f.assets[i]._side_info.id;

        if (!id.id_dat || !id.idp_lens || id.nParts == 0) {
            std::cout << "nom warning: skipped dir conversion for an asset" << std::endl;
            ctx.nEntries--; //one less entry
            std::cout << "nne: " << ctx.nEntries << std::endl;
            continue;
        }

        std::cout << "successfully coverted an asset!" << std::endl;

        in_memcpy(&fa->id, &id, sizeof(asset_id)); //copy over the id
        std::cout << "off copy: " << f.assets[i]._side_info.storage.offset << " for " << f.assets[i]._side_info.origin.f_path << std::endl;
        fa->off = f.assets[i]._side_info.storage.offset;
        j++;
    }

    std::cout << "NEntries: " << ctx.nEntries << std::endl;
    return _addDirectoryFmt1(ctx);
}

void omn::WriteToFile(std::string opath, nomfile f, nomsettings ns) {
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
    actx.ext_inf.nOffsetBytes = nOffsetBytes;

    //write first half of primary file header
    s.writeBytes((byte*) const_cast<char*>(fih), 3); //fsig
    s.writeByte((byte) AssetTy::Generic);            //subformat
    stream_write_version(&s, nea_ver);
    byte unicorn_byte = ((((byte) ns.endian) & 1) << 7) | ((((byte) ns.defISz) & 3) << 5) | (((nOffsetBytes - 1) & 7) << 1);
    byte cidlb = (ns.chunk_id_len - 1);

    //do some pre-asset analysis
    nomasset na; nomasset *fa = f.assets;

    i32 i, j;

    s.writeByte(unicorn_byte);

    //write second half of primary file header
    s.writeByte((0xef + cidlb) & 0xff); //chunk id len
    

    const size_t reservationsHeadWriteOff = s.tell() + 1;
    s.writeUInt(((1ULL << ((u64)nOffsetBytes << 3ULL)) - 1ULL), nOffsetBytes);
    const size_t rootDirHeadWriteOff = s.tell() + 1;
    s.writeUInt(((1ULL << ((u64)nOffsetBytes << 3ULL)) - 1ULL), nOffsetBytes);
    const size_t issueLogHeadWriteOff = s.tell() + 1;
    s.writeUInt(((1ULL << ((u64)nOffsetBytes << 3ULL)) - 1ULL), nOffsetBytes);

    SubChunkHeader sch;

    constexpr size_t maxCDatHeaderLen = 10; //9 bytes for uncompressed length + 1 byte for compression format

    sch.dat = new byte[maxCDatHeaderLen];

    //write all the assets first
    for (i = 0; i < f.nassets; i++, fa++) {
        na = *fa;

        std::cout << "NA DAT: " << na.len << std::endl;

        if (!na.dat || na.len == 0)
            continue;

        Chunk chonk = genAssetDataChunk(na);

        //compressed dat vars
        byte *sdat = sch.dat;
        i64 wpos = -1;
        auto lg2l = (signed) fast_log2(na.len),
                 nbl = (signed) ((lg2l >> 3) + ((lg2l & 7) > 0));

        switch (chonk.h.ty) {
        case ChunkType::RawDat:
            stream_write_chunk(actx, chonk);
            break;
        case ChunkType::CompressedDat:
            //configure the sch
            //uncompressed length
            if (nbl > maxCDatHeaderLen) {
                std::cout << "asset warning: nbl computation is sus" << std::endl;
                nbl = 8;
            }
            sch.len = 2 + nbl;
            *sdat++ = nbl & 0xff;
            j = na.len;
            if (ns.endian == IntFormat_BigEndian) endian_swap(j, nbl);
            do {
                *sdat++ = j & 0xff;
                j >>= 8;
            } while (--nbl > 0);

            //compression format
            *sdat++ = (u8) na._side_info.storage.compression;

            //some chonk config
            chonk.h.sepLen = true;
            chonk.h.writeLen = chonk.h.len + 4; //add for 4 bytes for checksum at end

            std::cout << "chunk len: " << chonk.h.writeLen << std::endl;

            //write the chunk
            //note: data is already compressed in genAssetDataChunk so no need to compress it here
            wpos = stream_write_chunk(actx, chonk, sch);
            //wpos = 9999;

            std::cout << "Asset Pos: " << wpos << " on " << fa->_side_info.origin.f_path << std::endl;

            if (wpos < 0) {
                std::cout << "error failed to write asset: " << na._side_info.origin.f_path << std::endl;
                continue;
            }

            s.writeUInt32(0); //append the uncompressed checksum at the end

            fa->_side_info.storage.offset = wpos;
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

    /*s.writeByte(0xaa);
    s.writeByte(0xbb);
    s.writeByte(0xcc);
    s.writeByte(0xdd);
    s.writeByte(0xee);
    s.writeByte(0xff);*/

    //now create the whole directory of le assets
    const i64 prim_dir_pos = WriteDirectoryFormat1(&s, f, ns);
    const size_t ret = s.seek(rootDirHeadWriteOff) + 1;
    if (prim_dir_pos > 0) s.writeUInt(prim_dir_pos, nOffsetBytes);
    s.seek(ret);

    //write to the file
    FileWrite::writeToBin(opath, s.getBytePtr(), s.size());
    s.free();
}

#include "json.hpp"

nomfile omn::GenNomFileFromJson(std::string jsonPath) {
    std::cout << "parsing path: " << jsonPath << std::endl;

    nomfile res;

    res.assets = nullptr;
    res.nassets = 0;

    if (jsonPath.length() == 0)
        return res;

    file jf = FileWrite::readFromBin(jsonPath);

    std::cout << "file len: " << jf.len << std::endl;

    if (!jf.dat || jf.len == 0) {
        if (jf.dat) _safe_free_a(jf.dat);
        return res;
    }

    JStruct fStruct = jparse::parseStr((const char*) const_cast<const byte*>(jf.dat), jf.len);

    size_t lStackSz = 256;
    const size_t avgLabelLen = 16, lStackInc = 256;

    #define _COMPUTE_STACK_SPACE_LEFT(len) (sizeof(u16) * (len))
    #define _COMPUTE_STACK_SPACE_RIGHT(len) (sizeof(char) * (len) * avgLabelLen)
    #define _COMPUTE_STACK_SPACE(len) (_COMPUTE_STACK_SPACE_LEFT(len) + _COMPUTE_STACK_SPACE_RIGHT(len))

    size_t nsElem = lStackSz, sLen = _COMPUTE_STACK_SPACE(nsElem);

    byte *labelStack = new byte[sLen], *sEnd = labelStack + sLen;
    char *datStack = (char*) (labelStack + nsElem * sizeof(u16)), *dsCur = datStack;
    u16 *lenStack = (u16*) labelStack, *lsCur = lenStack;

    size_t stackOcu = 0;

    struct protoAsset {
        std::string src;
        asset_id id;
    };

    auto freeProtoAsset = [&](protoAsset &pa) -> void {
        pa.src = "";
        if (pa.id.id_dat) _safe_free_a(pa.id.id_dat);
        if (pa.id.idp_lens) _safe_free_a(pa.id.idp_lens);
        if (pa.id.p_hash) _safe_free_a(pa.id.p_hash);
        pa.id.nParts = 0;
    };

    //if return false then push failed and should exit function
    auto lStackDatPush = [&](char *dat, size_t len) -> bool {
        if (len > mu_ui_infinity_16) len = mu_ui_infinity_16; //length cap

        std::cout << "stack push: ";
        mu_strPrint(dat,len,true);
        if ((uintptr_t) dsCur >= (uintptr_t) (sEnd - len)) {
            std::cout << "stack realloc..." << std::endl;

            //need to allocate more of le stack
            nsElem += lStackInc;
            const size_t snLen = _COMPUTE_STACK_SPACE(nsElem), prevNElem = nsElem - lStackInc;
            byte *nStack = new byte[snLen];

            if (!nStack) {
                std::cout << "Asset gen failed: bad alloc" << std::endl;
                return false;
            }

            ZeroMem(nStack, snLen);
            in_memcpy(nStack, labelStack, _COMPUTE_STACK_SPACE_LEFT(prevNElem)); //copy lengths
            in_memcpy(
                nStack     + _COMPUTE_STACK_SPACE_LEFT(nsElem), 
                labelStack + _COMPUTE_STACK_SPACE_LEFT(prevNElem), 
                _COMPUTE_STACK_SPACE_RIGHT(prevNElem)
            ); //copy strings
            sLen = snLen;

            _safe_free_a(labelStack);
            labelStack = nStack;

            //adjust current pointers
            const size_t lOff = ((uintptr_t) lsCur - (uintptr_t) lenStack) / sizeof(u16),
                         dOff = ((uintptr_t) dsCur - (uintptr_t) datStack) / sizeof(char);

            lenStack = (u16*) labelStack;
            lsCur = lenStack + lOff;

            datStack = (char*) (labelStack + nsElem * sizeof(u16));
            dsCur = datStack + dOff;
        }

        //push le data
        std::cout << "ls push: " << (u16) len << std::endl;
        *lsCur++ = (u16) len;
        std::cout << " | " << *(lsCur) << std::endl;
        in_memcpy(dsCur, dat, len * sizeof(char));
        dsCur += len;
        stackOcu++;

        return true;
    };

    auto lStackDatIdPop = [&](bool noAsset = false) -> asset_id {
        asset_id id = {
            .id_dat = nullptr,
            .nParts = 0
        };

        if ((uintptr_t) dsCur > (uintptr_t) datStack && (uintptr_t) lsCur > (uintptr_t) lenStack) {
            const size_t np = stackOcu;
            const size_t nc = ((uintptr_t) dsCur - (uintptr_t) datStack) / sizeof(char);

            if (np == 0 || nc == 0 || noAsset)
                goto pop_fin;

            std::cout << "NP: " << np << " NC: " << nc << std::endl;
            mu_strPrint(datStack, nc);

            id.nParts = np;
            id.idp_lens = new u16[np];
            id.id_dat = new char[nc];

            for (i32 ll = 0; ll < np; ll++) {
                std::cout << "lcop: " << lenStack[ll] << std::endl;
            }

            in_memcpy(id.idp_lens, lenStack, sizeof(u16) * np);
            in_memcpy(id.id_dat, datStack, sizeof(char) * nc);

            id.use_16bit_part_lens = true;
            id.p_hash = nullptr; //have this be autocalculated
        }

        pop_fin:

        if (stackOcu > 0) {
            dsCur -= *(--lsCur);
            stackOcu--;
        } else {
            lsCur = lenStack;
            std::cout << "uhh" << std::endl;
        }

        return id;
    };

    mu_vec<protoAsset> pAssets = mu_vec<protoAsset>();

    //generate proto assets from le json
    auto processJStruct = [&](JStruct *js, auto&& pjs) -> void {
        std::cout << "g: " << js->body.size() << std::endl;

        for (JToken tok : js->body) {
            std::cout << "tok: " << tok.rawValue << " " << tok.rawValue.length() << " | " << (u32) tok.ty << std::endl;
            std::cout << "mo tok: " << tok.label << std::endl;

            if (tok.label.length() == 0)
                continue;

            if (!lStackDatPush((char*) tok.label.c_str(), tok.label.length())) {
                std::cout << "asset gen warning: container label \"" << tok.rawValue << "\" failed to parse... skipping label" << std::endl; 
                continue;
            }

            if (!tok.body) {
                asset_id aId = lStackDatIdPop();

                if (!aId.id_dat || aId.nParts == 0) {
                    std::cout << "asset gen warning: asset label \"" << tok.rawValue << "\" failed to parse... skipping label" << std::endl;
                    continue;
                }

                protoAsset pa = {
                    .src = tok.rawValue,
                    .id = aId
                };

                pAssets.push(pa);
            } else {
                pjs(tok.body, pjs);
                lStackDatIdPop(true); //pop with no asset
            }
        }
    };

    processJStruct(&fStruct, processJStruct);

    //convert proto assets into actual assets
    res.nassets = pAssets.len();

    if (res.nassets > 0)
        res.assets = new nomasset[res.nassets];
    else
        res.assets = nullptr;

    i32 i;
    //nomasset *ta;
    protoAsset pa;

    //WARNING: CANNOT INTERATE OR INTERACT WITH PASSETS AFTER THIS LOOP!!!!
    //THIS LOOP CAN ALSO ONLY GO IN 1 DIRECTION (DO NOT MODIFY i WITHIN THE LOOP!!!!)
    for (i = 0; i < res.nassets; i++) {
        nomasset &ta = res.assets[i]; pa = pAssets[i];

        //ZeroMem(ta, 1);

        std::cout << ta.len << std::endl;
        std::cout << "PROTO: " << pa.id.nParts << " | " << (pa.id.nParts > 0 ? pa.id.idp_lens[0] : 9999) << " || " << std::endl;

        ta._side_info.id = pa.id;
        ta._side_info.storage.compression = CompressionMode::Zlib;
        ta._side_info.origin.f_path = pa.src;
        ta._side_info.good = true;
        ta._side_info.origin.oty = _nomasset_origin::File;
        
        //read file dat
        file f = FileWrite::readFromBin(pa.src);

        std::cout << "attempted read from file: " << pa.src << std::endl;
        std::cout << "results: " << f.len << std::endl;

        if (!f.dat || f.len == 0) {
            std::cout << "asset gen error: failed to read from file \"" << pa.src << "\"" << std::endl;
            freeProtoAsset(pa);
            if (f.dat) _safe_free_a(f.dat);
            ZeroMem(&ta, 1);
            ta.dat = nullptr;
            ta.len = 0;
            continue;
        }

        //NOTE: asset data must be freed through freeing nomassets since it will not be freed by freeProtoAsset
        //since id is passed to the asset
        ZeroMem(&pa.id, 1); //zero out the id so freeing does nothing

        std::cout << "asset sucess: \"" << pa.src << "\"" << std::endl;

        ta.dat = f.dat;
        ta.len = f.len;

        //do not free f.dat since the data is just given to the asset
        f.dat = nullptr; f.len = 0; //just incase i accidentally free it this will simply stop the free from working

        freeProtoAsset(pa);
    }

    pAssets.free();

    //mem management
    _safe_free_a(labelStack);
    _safe_free_a(jf.dat);


    for (i = 0; i < res.nassets; i++) {
        std::cout << ":asset: " << res.assets[i].len << " \\ " << res.assets[i]._side_info.origin.f_path << std::endl;
    }

    //
    return res;
};