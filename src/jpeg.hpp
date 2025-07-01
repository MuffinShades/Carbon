#pragma once
#include <iostream>
#include "msutil.hpp"
#include "peg.hpp"

/**
 * 
 * jpeg.hpp
 * 
 * Encoder and decoders for .jpg, .jpeg, .jfif files
 * 
 * Written by muffinshades June 28th 2025
 * 
 * Copyright muffinshades 2025-Present
 * 
 */

struct jpeg_image {
	byte* data = nullptr;
	size_t sz = 0;
	size_t width = 0;
	size_t height = 0;
};

class JpegParse {
public:
    static jpeg_image Decode(const std::string src);
	static jpeg_image DecodeBytes(byte* bytes, size_t sz);
	static bool Encode(std::string src, jpeg_image p);
};