#pragma once
#include <iostream>

#define APPLE_SILICON

#ifndef INLINE
#ifdef __GNUC__
#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define INLINE __inline__ __attribute__((always_inline))
#else
#define INLINE __inline__
#endif
#elif defined(_MSC_VER)
#define INLINE __forceinline
#elif (defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE 
#endif
#endif

extern void* in_memcpy(void *dest, const void* src, size_t len);
extern void* in_minicpy512(void *dest, const void* src, size_t len);
extern void* in_minicpy256(void *dest, const void *src, size_t len);