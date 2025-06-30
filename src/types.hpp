#pragma once
#include <iostream>

//h264 types
typedef bool flag;

//binary types
typedef bool bit;
typedef unsigned char byte;

//generic types
typedef uint8_t u8;
typedef int8_t i8;

typedef uint16_t u16;
typedef int16_t i16;

typedef uint32_t u24;
typedef int32_t i24;

typedef uint32_t u32;
typedef int32_t i32;

typedef uint64_t u48;
typedef int64_t i48;

typedef uint64_t u64;
typedef int64_t i64;

typedef float f32;
typedef double f64;

//words
#ifdef WORD
#undef WORD
#endif

#ifndef WIN32
typedef int16_t WORD;
#endif

#ifdef DWORD
#undef DWORD
#endif

#ifndef WIN32
typedef int32_t DWORD;
#endif

#ifdef QWORD
#undef QWORD
#endif

typedef int64_t QWORD;