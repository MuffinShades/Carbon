#pragma once
#include "Color.hpp"

/**
 * 
 * Picture for le video compression schemes
 * 
 * Copyright muffinsahdes 2025 All rights reserved
 * 
 */

struct pic_header {
	size_t w = 0, h = 0;
	ColorFormat cFormat;
};

struct Picture {
	pic_header header = {};
	byte* data = nullptr;
};

extern Picture MakeBlankPicture(pic_header h);
extern const size_t GetPictureDataSize(pic_header h);