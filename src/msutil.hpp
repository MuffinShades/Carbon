#pragma once
#include <iostream>
#include <cstring>
#include <vector>
#include "types.hpp"
#include "memcpy.hpp"
#define MSUTIL

template<class _Ty> inline static void ZeroMem(_Ty* dat, size_t sz = 1) {
    if (!dat) return;
    memset(dat, 0, sizeof(_Ty) * sz);
}

inline static void ZeroMem(void* dat, size_t sz) {
    if (dat == nullptr) return;
    memset(dat, 0, sz);
}

inline static size_t GetNumSz(const unsigned long long num) {
    unsigned long long compare = 1;
    size_t nBytes = 0;
    const size_t mxSz = sizeof(unsigned long long);

    while (num >= compare) {
        nBytes++;
        compare <<= 8;
    }

    return nBytes;
}

static inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

struct Point {
    float x, y;
};

#define GMask63(l) (u64) ((1ULL << ((unsigned long long)l)) - 1ULL)
#define GMask64(l) (((((1ULL << (l - 1ULL)) - 1ULL) << 1ULL) | 1ULL) * (l != 0ULL))
#define GMask(l) ((1 << (l)) - 1)
#define MAKE_MASK GMask
#define MAKE_MASK_64 GMask64

static inline std::vector<std::string> SplitString(std::string str, const char delim) {
    std::vector<std::string> res;
    const size_t len = str.length();
    std::string collector = "";
    const char* cStr = str.c_str();

    for (size_t i = 0; i < len; i++) {
        if (cStr[i] == delim) {
            res.push_back(collector);
            collector = "";
        }
        else
            collector += cStr[i];
    }

    res.push_back(collector);

    return res;
}

#define MAKE_LONG_BE(b0, b1, b2, b3, b4, b5, b6, b7) \
    ((b0) << 56) | \
    ((b1) << 48) | \
    ((b2) << 40) | \
    ((b3) << 32) | \
    ((b4) << 24) | \
    ((b5) << 16) | \
    ((b6) <<  8) | \
    ((b7) <<  0)

#define MAKE_LONG_LE(b0, b1, b2, b3, b4, b5, b6, b7) \
    ((b7) << 56) | \
    ((b6) << 48) | \
    ((b5) << 40) | \
    ((b4) << 32) | \
    ((b3) << 24) | \
    ((b2) << 16) | \
    ((b1) <<  8) | \
    ((b0) <<  0)

#define MAKE_INT_BE(b0, b1, b2, b3) \
    ((b0) << 24) | \
    ((b1) << 16) | \
    ((b2) <<  8) | \
    ((b3) <<  0)

#define MAKE_INT_LE(b0, b1, b2, b3) \
    ((b3) << 24) | \
    ((b2) << 16) | \
    ((b1) <<  8) | \
    ((b0) <<  0)

#define MAKE_SHORT_BE(b0, b1) \
    ((b0) <<  8) | \
    ((b1) <<  0)

#define MAKE_SHORT_LE(b0, b1) \
    ((b1) <<  8) | \
    ((b0) <<  0)

static inline u64 modifyByte(u64 val, size_t b_select, byte v) {
    b_select <<= 3;
    const u64 mask = MAKE_MASK_64(64) ^ (0xff << b_select);
    return (val & mask) | (v << b_select);
}

static bool _strCompare(std::string s1, std::string s2, bool lcmp = true, size_t slen = 0) {
    //length comparision
    if (lcmp)
        if (s1.length() != s2.length())
            return false;

    //compare chars
    const char* sc1 = s1.c_str(), * sc2 = s2.c_str();
    size_t sl = slen > 0 ? slen : s1.length();

    while (sl--)
        if (*sc1++ != *sc2++)
            return false;

    return true;
}

template<typename _Ty> static inline void _safe_free_a(_Ty*& m) {
    #ifdef MSFL_UTIL_MEM_DEBUG
    std::cout << "Freeing mem (A): " << (uintptr_t) m << std::endl;
    #endif
    if (!m) return;
    else {
        try {
            delete[] m;
            m = nullptr;
        }
        catch (std::exception e) {
            //oof
            m = nullptr;
            return;
        }
    }
}

template<typename _Ty> static inline void _safe_free_b(_Ty*& m) {
    #ifdef MSFL_UTIL_MEM_DEBUG
    std::cout << "Freeing mem (B): " << (uintptr_t) m << std::endl;
    #endif
    if (!m) return;
    else {
        try {
            delete m;
            m = nullptr;
        }
        catch (std::exception e) {
            //oof
            m = nullptr;
            return;
        }
    }
}

template<typename _Ty> static _Ty ArrMax(_Ty* arr, size_t len) {
    _Ty max = 0;

    for (size_t i = 0; i < len; i++)
        if (arr[i] > max) max = arr[i];

    return max;
}

template<typename _Ty> static void memfill(void* ptr, _Ty val, size_t nCopy) {
    if (nCopy < 0 || !ptr)
        return;

    _Ty* t_ptr = (_Ty*)ptr;

    for (size_t i = 0; i < nCopy; i++)
        *t_ptr++ = val;
}

static u64 NumReverse(u64 v, size_t bSz) {
    u64 r = 0;

    for (i32 i = 0; i < bSz; i++, r <<= 8, v >>= 8)
        r |= (v & 0xff);

    return r >> 8;
}

#ifndef mu_max
#define mu_max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef mu_min
#define mu_min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef mu_clamp
#define mu_clamp(a, max, min) (((a) > (max)) ? (max) : (((a) < (min)) : (min) : (a)));
#endif

//branchless min from: https://stackoverflow.com/questions/24529504/find-out-max-min-of-two-number-without-using-if-else
#define mu_nb_min(a, b) (a) ^ (((a) ^ (b)) & -((a) > (b)))
#define mu_nb_max(a, b) (a) ^ (((a) ^ (b)) & -((a) < (b)))
//pososible improvements on https://www.geeksforgeeks.org/compute-the-minimum-or-maximum-max-of-two-integers-without-branching/

/**
 *
 * Void buffer macros to make void buffers
 * less annoying
 *
 * VOID_BUF_ADD -> adds a value to the buffer
 * VOID_BUF_GET -> gets current value in a buffer
 * VOID_BUF_INC -> goes to next element in buffer
 * VOID_BUF_SET -> set current value in a void buffer
 * VOID_BUF_SI -> basically a void buf set followed by a void buf inc
 *
 */
#define VOID_BUF_ADD(buf, val) (buf) = (char*)(buf) + 1; \
                                *((char*)(buf)) = (val)
#define VOID_BUF_GET(buf) (*((char*)(buf)))
#define VOID_BUF_INC(buf) (buf) = (char*)(buf) + 1
#define VOID_BUF_SET(buf, val) *((char*)(buf)) = (val)
#define VOID_BUF_SI(buf, val) *((char*)(buf)) = (val); \
                               (buf) = (char*)(buf) + 1


#define foreach_ptr(type, cur, ptr, sz) \
    for (\
        type *cur=(ptr),*__end=((ptr)+(sz)); \
        cur<__end;cur++\
    )

 //same as foreach_ptr but m is for manual as in cur ins't incremented each iteration, it's manual
#define foreach_ptr_m(type, cur, ptr, sz) \
    for (\
        type *cur=(ptr),*__end=((ptr)+(sz)); \
        cur<__end;\
    )

#define forrange(n) \
    for (i64 i = 0; i < (n); i++)

/*#ifndef clamp
#define clamp(v, n, x) max((n), min((v), (x)))
#endif*/

#ifndef inRange
#define inRange(v, n, x) ((v) > (n)) && ((v) < (x))
#endif

template<class _Ty> static bool _bufCmp(_Ty *buf1, _Ty* buf2, size_t bSz) {
	if (!buf1 || !buf2) return false;

	for (size_t i = 0; i < bSz; i++)
		if (*buf1++ != *buf2++)
			return false;

	return true;
}

//fast version of modulo for certain bases
#define fast_modBase2(val, shift) ((val) & MAKE_MASK_64(shift))
#define fast_mod2(val) (val) & 1
#define fast_mod4(val) (val) & 3
#define fast_mod8(val) (val) & 7
#define fast_mod16(val) (val) & 15
#define fast_mod32(val) (val) & 31
#define fast_mod64(val) (val) & 63
#define fast_mod128(val) (val) & 127
#define fast_mod256(val) (val) & 255
#define fast_mod512(val) (val) & 511

static size_t computeMaxMod(u64 val) {
    u64 p = 1, n = 0;
    
    while (!(val & p)) {
        p <<= 1;
        n++;
    }
    
    return n;
}

//faster log functions that are also aligned for certain bases
#define __log_def(align) {  \
    size_t c = 0;              \
                            \
    while (val >>= align)   \
        c++;                \
                            \
    return c;               \
}

static inline size_t fast_log2(u64 val) __log_def(1)
static inline size_t fast_log4(u64 val) __log_def(2)
static inline size_t fast_log8(u64 val) __log_def(3)
static inline size_t fast_log16(u64 val) __log_def(4)
static inline size_t fast_log32(u64 val) __log_def(5)
static inline size_t fast_log64(u64 val) __log_def(6)
static inline size_t fast_log128(u64 val) __log_def(7)
static inline size_t fast_log256(u64 val) __log_def(8)

//may be able to make consteval / constexpr
static inline u64 endian_swap(u64 val, const size_t nBytes) {
    switch (nBytes) {
    case 2:
        return ((val & 0xff00) >> 8) | ((val & 0x00ff) << 8);
    case 3:
        return ((val & 0xff0000) >> 16) | ((val & 0x0000ff) << 16) | (val & 0x00ff00);
    case 4:
        return ((val & 0x000000ff) << 24) |
               ((val & 0x0000ff00) << 8) |
               ((val & 0x00ff0000) >> 8) |
               ((val & 0xff000000) >> 24);
    case 5:
        return ((val & 0x00000000ffULL) << 32ULL) |
               ((val & 0x000000ff00ULL) << 16ULL) |
                (val & 0x0000ff0000ULL) |
               ((val & 0x00ff000000ULL) >> 16ULL) |
               ((val & 0xff00000000ULL) >> 32ULL);
    case 6:
        return ((val & 0x0000000000ffULL) << 40ULL) |
               ((val & 0x00000000ff00ULL) << 24ULL) |
               ((val & 0x000000ff0000ULL) << 8ULL) |
               ((val & 0x0000ff000000ULL) >> 8ULL) |
               ((val & 0x00ff00000000ULL) >> 24ULL) |
               ((val & 0xff0000000000ULL) >> 40ULL);
    case 7:
        return ((val & 0x000000000000ffULL) << 48ULL) |
               ((val & 0x0000000000ff00ULL) << 32ULL) |
               ((val & 0x00000000ff0000ULL) << 16ULL) |
                (val & 0x000000ff000000ULL) |
               ((val & 0x0000ff00000000ULL) >> 16ULL) |
               ((val & 0x00ff0000000000ULL) >> 32ULL) |
               ((val & 0xff000000000000ULL) >> 48ULL);
    case 8:
        return
            ((val & 0x00000000000000ffULL) << 56ULL) | ((val & 0xff00000000000000ULL) >> 56ULL) |
            ((val & 0x000000000000ff00ULL) << 40ULL) | ((val & 0x00ff000000000000ULL) >> 40ULL) |
            ((val & 0x0000000000ff0000ULL) << 24ULL) | ((val & 0x0000ff0000000000ULL) >> 24ULL) |
            ((val & 0x00000000ff000000ULL) << 8ULL)  | ((val & 0x000000ff00000000ULL) >> 8ULL);
    default:
        return val;
    }
}

enum IntFormat {
    IntFormat_BigEndian = 0,
    IntFormat_LittleEndian = 1
};

static std::string escapeStrBackslashes(std::string str) {
    std::string res = "";
    for (char c : str) {
        if (c == '\\')
            res += "\\\\";
        else
            res += c;
    }
    return res;
}

constexpr inline static IntFormat __getOSEndian() {
    constexpr byte v = 1;
    return ((*(&v)) == 1) ? IntFormat_LittleEndian : IntFormat_BigEndian;
}

static void jumpOffABridge() {
    //volatile int* C4 = 0x0000000;
    //*C4 = NULL; //crash program :3
}

#define EXTRACT_BYTE_FLAG(flag, select) (((flag) >> (select)) & 1ULL)

static char* CovertBytesToString(byte* dat, size_t len, bool free_dat = false) {
    char *r = new char[len + 1];
    in_memcpy(r, dat, len);
    r[len] = 0;
    if (free_dat) _safe_free_a(dat);
    return r;
}

static f64 mu_sign(f64 val) {
    //return val / abs(val);
    return (val < 0.0f) ? -1.0f : 1.0f;
}

static i64 mu_sign(i64 val) {
    return ((val >> 63) & 1) ? -1 : 1;
    //return (val == 0) ? -1 : 1;
}

constexpr f64 mu_pi = 3.1415926;
constexpr f64 mu_rtd_const = mu_pi / 180.0;
constexpr f64 mu_dtr_const = 180.0 / mu_pi;

static inline f64 mu_rad(f64 theta) {
    return theta * mu_rtd_const;
}

static inline f64 mu_deg(f64 theta) {
    return theta * mu_dtr_const;
}

const u64 mu_ui_infinity_64 = 0xffffffffffffffffULL;
const u32 mu_ui_infinity_32 = 0xffffffffU;
const u16 mu_ui_infinity_16 = 0xffffU;
const u8  mu_ui_infinity_8 = 0xffU;

const i64 mu_i_infinity_64 = 0xffffffffffffffffLL;
const i32 mu_i_infinity_32 = 0xffffffff;
const i16 mu_i_infinity_16 = 0xffff;
const i8  mu_i_infinity_8 = 0xff;

#define __MSFL_HASH_32 {                        \
    if (!dat || nBits == 0 || len == 0) return 0;\
    if (nBits > 32) nBits = 32;                 \
                                                \
    const char *c_dat = (const char*) dat;      \
    const u32 mask = (1 << nBits) - 1;          \
                                                \
    u32 hash = 0, g = 0xff;                     \
                                                \
    i32 i;                                      \
                                                \
    for (i = 0; i < len; i++) {                 \
        hash += (c_dat[i] * g);                 \
                                                \
        g <<= 8;                                \
        g |= 0xff;                              \
        g = (g >> 31) * 0xff + g * !(g >> 31);  \
    }                                           \
                                                \
    return hash & mask;                         \
}

#define __MSDF_HASH_32_ALIGN_4 { \
    if (!dat || nBits == 0 || len == 0) return 0; \
                                                  \
    if (nBits > 32) nBits = 32;                   \
    const u32 *c_dat = (const u32*) dat;          \
    const u32 mask = (1 << nBits) - 1;            \
    u32 hash = 0, g = 0xff;                       \
    const size_t len_4 = len >> 2;                \
                                                  \
    i32 i;                                        \
                                                  \
    for (i = 0; i < len_4; i++) {                 \
        hash += (c_dat[i] * g);                   \
                                                  \
        g <<= 8;                                  \
        g |= 0xff;                                \
        g = (g >> 31) * 0xff + g * !(g >> 31);    \
    }                                             \
                                                  \
    return hash & mask;                           \
}

#define __MSFL_HASH_64 {                        \
    if (!dat || nBits == 0 || len == 0) return 0;\
    if (nBits > 64) nBits = 64;                 \
                                                \
    const char *c_dat = (const char*) dat;      \
    const u64 mask = (1ULL << nBits) - 1ULL;          \
                                                \
    u64 hash = 0, g = 0xffULL;                     \
                                                \
    i32 i;                                      \
                                                \
    for (i = 0; i < len; i++) {                 \
        hash += (c_dat[i] * g);                 \
                                                \
        g <<= 8ULL;                                \
        g |= 0xffULL;                              \
        g = (g >> 31ULL) * 0xffULL + g * !(g >> 31ULL);  \
    }                                           \
                                                \
    return hash & mask;                         \
}

#define __MSDF_HASH_64_ALIGN_8 { \
    if (!dat || nBits == 0 || len == 0) return 0; \
                                                  \
    if (nBits > 64) nBits = 64;                   \
    const u64 *c_dat = (const u64*) dat;          \
    const u64 mask = (1ULL << nBits) - 1ULL;            \
    u64 hash = 0, g = 0xffULL;                       \
    const size_t len_8 = len >> 3ULL;                \
                                                  \
    i32 i;                                        \
                                                  \
    for (i = 0; i < len_8; i++) {                 \
        hash += (c_dat[i] * g);                   \
                                                  \
        g <<= 8ULL;                                  \
        g |= 0xffULL;                                \
        g = (g >> 63ULL) * 0xffULL + g * !(g >> 63ULL);    \
    }                                             \
                                                  \
    return hash & mask;                           \
}

static const u32 compute_basic_hash_32(size_t nBits, void *dat, size_t len) __MSFL_HASH_32
static inline const u32 compute_basic_hash_32_inline(size_t nBits, void *dat, size_t len) __MSFL_HASH_32
static const u32 compute_basic_hash_32_align_4(size_t nBits, void *dat, size_t len) __MSDF_HASH_32_ALIGN_4
static inline const u32 compute_basic_hash_32_align_4_inline(size_t nBits, void *dat, size_t len) __MSDF_HASH_32_ALIGN_4
static const u64 compute_basic_hash_64(size_t nBits, void *dat, size_t len) __MSFL_HASH_64
static inline const u64 compute_basic_hash_64_inline(size_t nBits, void *dat, size_t len) __MSFL_HASH_64
static const u64 compute_basic_hash_64_align_8(size_t nBits, void *dat, size_t len) __MSDF_HASH_64_ALIGN_8
static inline const u64 compute_basic_hash_64_align_8_inline(size_t nBits, void *dat, size_t len) __MSDF_HASH_64_ALIGN_8

#define COMMA ,

const i32 greedy_look_256[] = {
    0x0,0xF,0x4E,0x16,0x8D,0x24D,0x55,0x1D,0xCC,0x28C,0x124C,0x44C,
    0x94,0x254,0x5C,0x24,0x10B,0x2CB,0x128B,0x48B,0x224B,0x924B,0x144B,
    0x64B,0xD3,0x293,0x1253,0x453,0x9B,0x25B,0x63,0x2B,0x14A,0x30A,
    0x12CA,0x4CA,0x228A,0x928A,0x148A,0x68A,0x324A,0xA24A,0x4924A,0x1124A,
    0x244A,0x944A,0x164A,0x84A,0x112,0x2D2,0x1292,0x492,0x2252,0x9252,
    0x1452,0x652,0xDA,0x29A,0x125A,0x45A,0xA2,0x262,0x6A,0x32,0x189,0x349,
    0x1309,0x509,0x22C9,0x92C9,0x14C9,0x6C9,0x3289,0xA289,0x49289,0x11289,
    0x2489,0x9489,0x1689,0x889,0x4249,0xB249,0x4A249,0x12249,0x89249,
    0x249249,0x51249,0x19249,0x3449,0xA449,0x49449,0x11449,0x2649,0x9649,
    0x1849,0xA49,0x151,0x311,0x12D1,0x4D1,0x2291,0x9291,0x1491,0x691,0x3251,
    0xA251,0x49251,0x11251,0x2451,0x9451,0x1651,0x851,0x119,0x2D9,0x1299,
    0x499,0x2259,0x9259,0x1459,0x659,0xE1,0x2A1,0x1261,0x461,0xA9,0x269,
    0x71,0x39,0x39,0x71,0x269,0xA9,0x461,0x1261,0x2A1,0xE1,0x659,0x1459,0x9259,
    0x2259,0x499,0x1299,0x2D9,0x119,0x851,0x1651,0x9451,0x2451,0x11251,0x49251,
    0xA251,0x3251,0x691,0x1491,0x9291,0x2291,0x4D1,0x12D1,0x311,0x151,0xA49,
    0x1849,0x9649,0x2649,0x11449,0x49449,0xA449,0x3449,0x19249,0x51249,
    0x249249,0x89249,0x12249,0x4A249,0xB249,0x4249,0x889,0x1689,0x9489,0x2489,
    0x11289,0x49289,0xA289,0x3289,0x6C9,0x14C9,0x92C9,0x22C9,0x509,0x1309,0x349,
    0x189,0x32,0x6A,0x262,0xA2,0x45A,0x125A,0x29A,0xDA,0x652,0x1452,0x9252,
    0x2252,0x492,0x1292,0x2D2,0x112,0x84A,0x164A,0x944A,0x244A,0x1124A,0x4924A,
    0xA24A,0x324A,0x68A,0x148A,0x928A,0x228A,0x4CA,0x12CA,0x30A,0x14A,0x2B,0x63,
    0x25B,0x9B,0x453,0x1253,0x293,0xD3,0x64B,0x144B,0x924B,0x224B,0x48B,0x128B,
    0x2CB,0x10B,0x24,0x5C,0x254,0x94,0x44C,0x124C,0x28C,0xCC,0x1D,0x55,0x24D,
    0x8D,0x16,0x4E,0xF,0x0
};

static inline void mu_strPrint(char *str, size_t len, bool endl = true) {
    if (!str || len == 0) return;

    i32 i;

    for (i = 0; i < len; i++) {
        std::cout << str[i];
    }

    if (endl)
        std::cout << std::endl;
}