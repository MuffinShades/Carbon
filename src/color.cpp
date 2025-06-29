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

}

u8 Color::chromaR() {

}

u8 Color::chromaB() {

}

u32 Color::yCrCb() {

}

u16 Color::hue() {

}

u8 Color::saturation() {

}

u8 Color::lightness() {

}

u8 Color::vibrance() {
	
}