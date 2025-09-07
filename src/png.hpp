#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include "balloon.hpp"
#include "bytestream.hpp"
#include "content.hpp"

/**
 * 
 * png.hpp
 * 
 * Encoder and decoders for .png image files
 * 
 * Written by muffinshades 2024-2025
 * 
 * Copyright muffinshades 2025-Present
 * 
 */

enum Png_ColorSpace {
	Png_Color_GrayScale = 0,
	Png_Color_RGB = 2,
	Png_Color_Indexed = 3,
	Png_Color_GrayScale_Alpha = 4,
	Png_Color_RGBA = 6
};

struct _IHDR {
	size_t w, h;
	byte bitDepth;
	Png_ColorSpace colorSpace;
	byte compressionMethod = 0;
	byte filterType = 0;
	bool interlaced;
	size_t bytesPerPixel, nChannels, bpp;
	bool from_src = false;
};

struct png_header {
	size_t sz = 0;
	size_t width = 0;
	size_t height = 0;
	i32 channels = 0;
	Png_ColorSpace colorMode;
	i32 bitDepth;
	_IHDR tech_header = {
		.from_src = false
	};
};

struct png_image {
	byte* data = nullptr;
	size_t sz = 0;
	png_header inf;
};

class PngParse {
public:
	static png_image Decode(ContentSrc src);
	static png_header ExtractHeader(ContentSrc src);
	static bool Encode(std::string src, png_image p);
};