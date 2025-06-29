#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include "balloon.hpp"
#include "bytestream.hpp"

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

struct png_image {
	byte* data = nullptr;
	size_t sz = 0;
	size_t width = 0;
	size_t height = 0;
	i32 channels = 0;
	Png_ColorSpace colorMode;
	i32 bitDepth;
};

class PngParse {
public:
	static png_image Decode(std::string src);
	static png_image DecodeBytes(byte* bytes, size_t sz);
	static bool Encode(std::string src, png_image p);
};