/*

Collection of hasing functions for:

SHA-1, SHA-224, SHA-256, SHA-384, and SHA512

written by muffinshades 7/22/2026 (only took one day!!! :P)

*/

#pragma once

#include <iostream>
#include <cstdint>
#include <cstring>
#include <memory>
#include <iomanip>
#include "msutil.hpp"

#ifndef MSUTIL
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef uint8_t byte;
#endif

typedef uint8_t size_8;
typedef uint32_t size_32;
typedef uint64_t size_64;

constexpr u32 sha_const_1[4] = {
    0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6
};

constexpr u32 sha_const_256[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

constexpr u64 sha_const_512[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL,
	0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
	0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL,
	0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
	0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
	0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
	0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL, 0x2de92c6f592b0275ULL,
	0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
	0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL,
	0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
	0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL,
	0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
	0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL,
	0x92722c851482353bULL, 0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
	0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
	0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
	0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL,
	0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
	0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL,
	0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
	0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL,
	0xc67178f2e372532bULL, 0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
	0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL,
	0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
	0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
	0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
	0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

u32 h_def_1[5] = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
};

u32 h_def[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

u64 h_def_512[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

//TODO: optimize this more by finding what alignment of n makes the arithmatic fastest
inline u32 quick_rotr_32(u32 v, size_32 n) {
    return (v >> n) | (v << (32 - n));
}

u32 safe_rotr_32(u32 v, size_t n) {
    if (n >= 32)
        return v;
        
    return quick_rotr_32(v, n & 0xFF);
}

#define rotr_32 quick_rotr_32

inline u32 quick_rotl_32(u32 v, size_32 n) {
    return (v << n) | (v >> (32 - n));
}

u32 safe_rotl_32(u32 v, size_t n) {
    if (n >= 32)
        return v;
        
    return quick_rotl_32(v, n & 0xFF);
}

#define rotl_32 quick_rotl_32

inline u64 quick_rotr_64(u64 v, size_64 n) {
    return (v >> n) | (v << (64ULL - n));
}

u64 safe_rotr_64(u64 v, size_t n) {
    if (n >= 64ULL)
        return v;
        
    return quick_rotr_64(v, n & 0xFF);
}

#define rotr_64 quick_rotr_64

inline u32 Ch256(u32 x, u32 y, u32 z) {
    return (x & y) ^ ((~x) & z);
}

inline u32 Maj256(u32 x, u32 y, u32 z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline u32 bsig0_256(u32 x) {
    return rotr_32(x, 2) ^ rotr_32(x, 13) ^ rotr_32(x, 22);
}

inline u32 bsig1_256(u32 x) {
    return rotr_32(x, 6) ^ rotr_32(x, 11) ^ rotr_32(x, 25);
}

inline u32 lsig0_256(u32 x) {
    return rotr_32(x, 7) ^ rotr_32(x, 18) ^ (x >> 3);
}

inline u32 lsig1_256(u32 x) {
    return rotr_32(x, 17) ^ rotr_32(x, 19) ^ (x >> 10);
}

inline u32 Parity32(u32 x, u32 y, u32 z) {
    return x ^ y ^ z;
}

inline u64 Ch512(u64 x, u64 y, u64 z) {
    return (x & y) ^ ((~x) & z);
}

inline u64 Maj512(u64 x, u64 y, u64 z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline u64 bsig0_512(u64 x) {
    return rotr_64(x, 28) ^ rotr_64(x, 34) ^ rotr_64(x, 39);
}

inline u64 bsig1_512(u64 x) {
    return rotr_64(x, 14) ^ rotr_64(x, 18) ^ rotr_64(x, 41);
}

inline u64 lsig0_512(u64 x) {
    return rotr_64(x, 1) ^ rotr_64(x, 8) ^ (x >> 7ULL);
}

inline u64 lsig1_512(u64 x) {
    return rotr_64(x, 19) ^ rotr_64(x, 61) ^ (x >> 6ULL);
}

inline void process_sha_block_1(byte *block, u32 *H, i32 i) {
    if (!block || !H) return;
    
    u32 a,b,c,d,e,j,j0;
    
    u32 W[80] = {0};
    
    for (j = 0; j < 16; j++) {
        j0 = (j << 2) + i;
        W[j] = 
            (block[j0+0] << 24) |
            (block[j0+1] << 16) |
            (block[j0+2] << 8)  |
            (block[j0+3] << 0)  ;
    }
            
    //fill the rest of the block
    for (j = 16; j < 80; j++) {
        W[j] = rotl_32(W[j-3] ^ W[j-8] ^ W[j-14] ^ W[j-16], 1);
    }
            
    a = H[0]; b = H[1];
    c = H[2]; d = H[3];
    e = H[4];
            
    u32 t;
    
    #define _sha1_process_right e = d; d = c; c = rotl_32(b, 30); b = a; a = t;
    
    //Ch, Parity, Maj, Parity
    //0-19, 20-39, 40-59, 60-79
    for (j = 0; j < 20; j++) {t = rotl_32(a, 5) + Ch256(b, c, d) + e + sha_const_1[0] + W[j]; _sha1_process_right}
    for (j = 20; j < 40; j++) {t = rotl_32(a, 5) + Parity32(b, c, d) + e + sha_const_1[1] + W[j]; _sha1_process_right}
    for (j = 40; j < 60; j++) {t = rotl_32(a, 5) + Maj256(b, c, d) + e + sha_const_1[2] + W[j]; _sha1_process_right}
    for (j = 60; j < 80; j++) {t = rotl_32(a, 5) + Parity32(b, c, d) + e + sha_const_1[3] + W[j]; _sha1_process_right}
            
    H[0] += a; H[1] += b;
    H[2] += c; H[3] += d;
    H[4] += e;
}

inline void process_sha_block_256(byte *block, u32 *H, i32 i) {
    if (!block || !H) return;
    
    u32 a,b,c,d,e,f,g,h,j,j0;
    
    u32 W[64] = {0};
    
    for (j = 0; j < 16; j++) {
        j0 = (j << 2) + i;
        W[j] = 
            (block[j0+0] << 24) |
            (block[j0+1] << 16) |
            (block[j0+2] << 8)  |
            (block[j0+3] << 0)  ;
    }
            
    //fill the rest of the block
    for (j = 16; j < 64; j++) {
        W[j] = lsig1_256(W[j-2])  + W[j-7] 
             + lsig0_256(W[j-15]) + W[j-16];
    }
            
    a = H[0]; b = H[1];
    c = H[2]; d = H[3];
    e = H[4]; f = H[5];
    g = H[6]; h = H[7];
            
    u32 t1,t2;
            
    for (j = 0; j < 64; j++) {
        t1 = h + bsig1_256(e) + Ch256(e,f,g) + sha_const_256[j] + W[j];
        t2 = bsig0_256(a) + Maj256(a,b,c);
        h = g; g = f; f = e; e = d + t1; 
        d = c; c = b; b = a; a = t1 + t2;
    }
            
    H[0] += a; H[1] += b;
    H[2] += c; H[3] += d;
    H[4] += e; H[5] += f;
    H[6] += g; H[7] += h;
}

inline void process_sha_block_512(byte *block, u64 *H, i64 i) {
    if (!block || !H) return;
    
    u64 a,b,c,d,e,f,g,h,j,j0;
    
    u64 W[80] = {0};
    
    for (j = 0; j < 16; j++) {
        j0 = (j << 3) + i;
        W[j] = 
            ((u64)block[j0+0] << 56ULL) |
            ((u64)block[j0+1] << 48ULL) |
            ((u64)block[j0+2] << 40ULL)  |
            ((u64)block[j0+3] << 32ULL)  |
            ((u64)block[j0+4] << 24ULL)  |
            ((u64)block[j0+5] << 16ULL)  |
            ((u64)block[j0+6] << 8ULL)  |
            ((u64)block[j0+7] << 0ULL)  ;
    }
            
    //fill the rest of the block
    for (j = 16; j < 80; j++) {
        W[j] = lsig1_512(W[j-2])  + W[j-7] 
             + lsig0_512(W[j-15]) + W[j-16];
    }
    
    //printBlock32(W);
            
    a = H[0]; b = H[1];
    c = H[2]; d = H[3];
    e = H[4]; f = H[5];
    g = H[6]; h = H[7];
    
    
            
    u64 t1,t2;
            
    for (j = 0; j < 80; j++) {
        t1 = h + bsig1_512(e) + Ch512(e,f,g) + sha_const_512[j] + W[j];
        t2 = bsig0_512(a) + Maj512(a,b,c);
        h = g; g = f; f = e; e = d + t1; 
        d = c; c = b; b = a; a = t1 + t2;
    }
            
    H[0] += a; H[1] += b;
    H[2] += c; H[3] += d;
    H[4] += e; H[5] += f;
    H[6] += g; H[7] += h;
}

/*

Word sizes:

sha256 or below: 32bits or u32
all other sha (above sha256): 64bits or u64

*/

struct igroup160_32 {
    u32 dat[5] = {0,0,0,0,0};
};

struct igroup256_32 {
  u32 dat[8] = {0,0,0,0,0,0,0,0};  
};

struct igroup512_64 {
    u64 dat[8] = {0,0,0,0,0,0,0,0};
};

struct igroup224_32 {
  u32 dat[6] = {0,0,0,0,0,0};  
};

struct igroup384_64 {
    u64 dat[6] = {0,0,0,0,0,0};
};

class hash {
public:
    static igroup160_32 sha1(byte *dat, size_t len) {
        igroup160_32 res;
        
        memcpy(res.dat, h_def_1, sizeof(u32) * 5);
        
        if (!dat || len == 0)
            return res;
        
        const size_t lb = len << 3;
        const u32 k = 512 - ((lb + 65) & 511);
        const size_t appendFFill = len & 63;
        
        byte dat_append[64] = {0};
        
        size_t g = 0;
        
        const u32 la = (len - appendFFill);
        
        for (;g < appendFFill;g++) dat_append[g] = dat[la + g];
        
        dat_append[g++] = 0x80;

        g += (k >> 3);
        
        dat_append[g+0] = (lb >> 56) & 0xFF;
        dat_append[g+1] = (lb >> 48) & 0xFF;
        dat_append[g+2] = (lb >> 40) & 0xFF;
        dat_append[g+3] = (lb >> 32) & 0xFF;
        dat_append[g+4] = (lb >> 24) & 0xFF;
        dat_append[g+5] = (lb >> 16) & 0xFF;
        dat_append[g+6] = (lb >> 8) & 0xFF;
        dat_append[g+7] = lb & 0xFF;
        
        const size_t l_proc = len - appendFFill;
        
        i32 i;
        
        for (i = 0; i < l_proc; i += 64) {
            process_sha_block_1(dat+i, res.dat, 0);
        }
        
        //process final block
        process_sha_block_1(dat_append, res.dat, 0);
            
        return res;
    }
    
    static igroup224_32 sha224(byte *dat, size_t len) {
        igroup256_32 s256 = sha256(dat,len);
        igroup224_32 res;
        memcpy(res.dat, s256.dat, sizeof(u32) * 6);
        return res;
    }

    static igroup256_32 sha256(byte *dat, size_t len) {
        igroup256_32 res;
        
        memcpy(res.dat, h_def, sizeof(u32) * 8);
        
        if (!dat || len == 0)
            return res;
        
        const size_t lb = len << 3, lbm512 = lb & 511;
        const u32 k = 512 - ((lb + 65) & 511);
        
        const size_t appendFFill = len & 63;
        
        byte dat_append[64] = {0};
        
        size_t g = 0;
        
        const u32 la = (len - appendFFill);
        
        for (;g < appendFFill;g++) dat_append[g] = dat[la + g];
        
        dat_append[g++] = 0x80;

        g += (k >> 3);
        
        dat_append[g+0] = (lb >> 56) & 0xFF;
        dat_append[g+1] = (lb >> 48) & 0xFF;
        dat_append[g+2] = (lb >> 40) & 0xFF;
        dat_append[g+3] = (lb >> 32) & 0xFF;
        dat_append[g+4] = (lb >> 24) & 0xFF;
        dat_append[g+5] = (lb >> 16) & 0xFF;
        dat_append[g+6] = (lb >> 8) & 0xFF;
        dat_append[g+7] = lb & 0xFF;
        
        const size_t l_proc = len - appendFFill;
        
        i32 i;
        
        for (i = 0; i < l_proc; i += 64) {
            process_sha_block_256(dat+i, res.dat, 0);
        }
        
        //process final block
        process_sha_block_256(dat_append, res.dat, 0);
            
        return res;
    }
    
    static igroup384_64 sha384(byte *dat, size_t len) {
        igroup512_64 s512 = sha512(dat,len);
        igroup384_64 res;
        memcpy(res.dat, s512.dat, sizeof(u64) * 6);
        return res;
    }
    
    static igroup512_64 sha512(byte *dat, size_t len) {
        igroup512_64 res;
        
        memcpy(res.dat, h_def_512, sizeof(u64) * 8);
        
        if (!dat || len == 0)
            return res;
            
        const size_t lb = len << 3;
        const u64 k = 1024 - ((lb + 129) & 1023);
        const size_t appendFFill = len & 127;
        
        byte dat_append[128] = {0};
        
        size_t g = 0;
        
        const u64 la = (len - appendFFill);
        
        for (;g < appendFFill;g++) dat_append[g] = dat[la + g];
        
        dat_append[g++] = 0x80;
        
        g += (k >> 3);
        
        //skip the left 64 bits in the 128bit number of bits
        dat_append[g+8] = (lb >> 56ULL) & 0xFFULL;
        dat_append[g+9] = (lb >> 48ULL) & 0xFFULL;
        dat_append[g+10] = (lb >> 40ULL) & 0xFFULL;
        dat_append[g+11] = (lb >> 32ULL) & 0xFFULL;
        dat_append[g+12] = (lb >> 24ULL) & 0xFFULL;
        dat_append[g+13] = (lb >> 16ULL) & 0xFFULL;
        dat_append[g+14] = (lb >> 8ULL) & 0xFFULL;
        dat_append[g+15] = lb & 0xFFULL;
        
        const size_t l_proc = len - appendFFill;
        
        i32 i;
        
        for (i = 0; i < l_proc; i += 128) {
            process_sha_block_512(dat+i, res.dat, 0);
        }
        
        //process final block
        process_sha_block_512(dat_append, res.dat, 0);
            
        return res;
    }
};