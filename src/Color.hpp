#pragma once
#include "msutil.hpp"

/**
 * 
 * Color.hpp
 * 
 * Programmed by muffinshades 2025
 * 
 */

//TODO: verify all these macros are correct
#define MAKECOLOR(r,g,b,a) (((r) & 0xff) | (((g) & 0xff) << 8) | (((b) & 0xff) << 16) | (((a) & 0xff) << 24))
#define GET_COLOR_CHANNEL(col, ch) (((col) >> ((mu_min(ch, 3)) << 3)) & 0xff)
#define MODIFY_COLOR_CHANNEL(col, ch, val) ((col) ^ ((col) & (0xff << (mu_min(ch, 3) << 3)))) | (((val) & 0xff) << (mu_min(ch, 3) << 3))

enum ColorMode {
	DEFAULT_COLOR,
	AUTO_COLOR
};

enum ColorFormat {
	ColorFormat_RGB,
	ColorFormat_RGBA,
	ColorFormat_HSV,
	ColorFormat_HSL,
	ColorFormat_YCrCb,
	ColorFormat_GrayScale
};

extern const size_t GetColorFormatSize(ColorFormat fm);

class Color {
private:
	u8 r = 0, g = 0, b = 0, a = 255;
	u32 i = 0;
	ColorMode _mode = DEFAULT_COLOR;
	void __i_compute();
	void __ch_compute();
public:
	Color(u8 r = 0, u8 g = 0, u8 b = 0, u8 a = 255);
	Color(u32 i);
	Color(ColorMode mode);
	u32 rgb();
	u32 rgba();
	u8 red();
	u8 green();
	u8 blue();
	u8 alpha();
	u8 luma();
	u8 chromaR();
	u8 chromaB();
	u32 yCrCb();
	u16 hue();
	u8 saturation();
	u8 lightness();
	u8 vibrance();
	ColorMode mode();
	bool autoColor();
	void setR(u8 r);
	void setG(u8 g);
	void setB(u8 b);
	void setA(u8 a);
	void setLuma(u8 y);
	void setChromaR(u8 cr);
	void setChromaB(u8 cb);
	void setHue(u16 h);
	void setSaturation(u8 s);
	void setLightness(u8 l);
	void setVibrance(u8 v);
};

struct GradColorStop {
	Point p;
	Color c;
};

class Gradient {
private:
	std::vector<GradColorStop> stops;
public:
	Gradient(GradColorStop s[]);
	Gradient(std::vector<GradColorStop> s[]);
};