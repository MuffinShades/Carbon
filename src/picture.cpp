#include "picture.hpp"

const size_t GetPictureDataSize(pic_header h) {
	return h.w * h.h * GetColorFormatSize(h.cFormat);
}

Picture MakeBlankPicture(pic_header h) {
	const size_t bbp = GetColorFormatSize(h.cFormat);

	if (bbp == 0)
		return {
			.header = {0},
			.data = nullptr
		};

	const size_t dataSz = GetPictureDataSize(h);

	Picture r_pic = {
		.header = h,
		.data = new byte[dataSz]
	};

	ZeroMem(r_pic.data, dataSz);

	return r_pic;
}