#pragma once
/**
 *
 * BALLOON - C++ lightweight zlib implementation
 *
 * Version 0.0 written May 2024 [Old]
 * 		poorly made
 * Version 1.0 written September - December 2024 [Old]
 *      made better
 * Version 1.1 written June 2025 [New]
 * 	    slight optimizations with bugs fixes
 * Version 1.2  written June 18th 2025 [In Developement]
 * 		optimizations
 *
 * Program written by muffinshades
 *
 * Copyright (c) 2024 muffinshades
 *
 * You can do what ever you want with the software but you must
 * credit the author (muffinshades) with the original creation
 * of the software. Idk what else to put here lmao.
 *
 * Balloon Notes:
 *
 * This library is a implementation of the zlib or Inflate / Deflate
 * compression algorithm.
 *
 * Right now compression speeds are around 4-6mb/s and decompression
 * speeds are much faster. This isn't the worlds fastest implementation,
 * but its decently fast and lightweight. One day I will improve the LZ77
 * hash functions, but for now it's gonna stay at around 5mb/s.
 *
 * This program should be able to function with any other inflate / deflate
 * implementation apart from the whole compression level calculations being
 * different since I didn't entirley implement lazy and good matches into
 * the lz77 functions. I also didn't add a whole fast version for everything
 * since this is a relativley light weight library. One day I do plan on adding
 * these functions and making a even better implementation of zlib.
 *
 */

#include <iostream>
#include <cstring>
#include "msutil.hpp"

#ifdef CARBON_DLL
#ifdef CARBON_EXPORTS
#define CARBON_EXP __declspec(dllexport)
#else
#define CARBON_EXP __declspec(dllimport)
#endif
#else
#define CARBON_EXP
#endif

#ifdef CARBON_DLL
#ifdef __cplusplus
extern "C" {
#endif
#endif

struct balloon_result {
	byte* data;
	size_t sz;
	u32 checksum;
	byte compressionMethod;
};

#define BALLOON_MAX_THREADS_AUTO -1

class Balloon {
public:
	CARBON_EXP static balloon_result Deflate(byte* data, size_t sz, u32 compressionLevel = 2, const size_t winBits = 0xf);
	CARBON_EXP static balloon_result Inflate(byte* data, size_t sz);
	static bool DeflateFileToFile(std::string in_src, std::string out_src, u32 compressionLevel = 2, const size_t winBits = 0xf);
	static bool InflateFileToFile(std::string in_src, std::string out_src);
	static balloon_result MultiThreadDeflate(byte* data, size_t sz, i32 maxThreads = BALLOON_MAX_THREADS_AUTO, u32 compressionLevel = 2, const size_t winBits = 0xf);
	static balloon_result MultiThreadDeflate(byte* data, size_t sz, i32 maxThreads = BALLOON_MAX_THREADS_AUTO);
	static bool MultiThreadDeflateFileToFile(std::string in_src, std::string out_src, i32 maxThreads = BALLOON_MAX_THREADS_AUTO, u32 compressionLevel = 2, const size_t winBits = 0xf);
	static bool MultiThreadInflateFileToFile(std::string in_src, std::string out_src, i32 maxThreads = BALLOON_MAX_THREADS_AUTO);
	CARBON_EXP static void Free(balloon_result* res);
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif