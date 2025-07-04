#include "color.hpp"

void Color::__i_compute() {
	this->i = MAKECOLOR(this->r, this->g, this->b, this->a);
}

void Color::__ch_compute() {
	this->r = GET_COLOR_CHANNEL(this->i, 0);
	this->g = GET_COLOR_CHANNEL(this->i, 1);
	this->b = GET_COLOR_CHANNEL(this->i, 2);
	this->a = GET_COLOR_CHANNEL(this->i, 3);
}

Color::Color(ColorMode mode) {
	this->_mode = mode;
}

Color::Color(u8 r, u8 g, u8 b, u8 a) {
	this->i = MAKECOLOR(r,g,b,a);
}

Color::Color(u32 i) {
	this->i = i;
}

u32 Color::rgb() {
	return this->i & 0xffffff;
}

u32 Color::rgba() {
	return this->i;
}

u8 Color::red() {
	return GET_COLOR_CHANNEL(this->i, 0);
}

u8 Color::green() {
	return GET_COLOR_CHANNEL(this->i, 1);
}

u8 Color::blue() {
	return GET_COLOR_CHANNEL(this->i, 2);
}

u8 Color::alpha() {	
	return GET_COLOR_CHANNEL(this->i, 3);
}

void Color::setR(u8 r) {
	this->i = MODIFY_COLOR_CHANNEL(this->i, 0, r);
}

void Color::setG(u8 g) {
	this->i = MODIFY_COLOR_CHANNEL(this->i, 1, g);
}

void Color::setB(u8 b) {
	this->i = MODIFY_COLOR_CHANNEL(this->i, 2, b);
}

void Color::setA(u8 a) {
	this->i = MODIFY_COLOR_CHANNEL(this->i, 3, a);
}

u8 Color::luma() {
	return 0;
}

u8 Color::chromaR() {
	return 0;
}

u8 Color::chromaB() {
	return 0;
}

u32 Color::yCrCb() {
	return 0;
}

u16 Color::hue() {
	return 0;
}

u8 Color::saturation() {
	return 0;
}

u8 Color::lightness() {
	return 0;
}

u8 Color::vibrance() {
	return 0;
}

ColorMode Color::mode() {
	return this->_mode;
}

bool Color::autoColor() {
	return this->_mode == AUTO_COLOR;
}

const size_t GetColorFormatSize(ColorFormat fm) {
	switch (fm) {
	case ColorFormat_RGB:
		return 3;
	case ColorFormat_RGBA:
		return 4;
	case ColorFormat_HSV:
		return 3;
	case ColorFormat_YCrCb:
		return 3;
	case ColorFormat_GrayScale:
		return 1;
	}

	return 0;
}