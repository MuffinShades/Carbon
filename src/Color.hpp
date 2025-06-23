#pragma once
#include "util.hpp"

//TODO: verify all these macros are correct
#define WIN52_min(a,b) ((a) < (b) ? (a) : (b))
#define WIN52_MAKECOLOR(r,g,b,a) (((r) & 0xff) | (((g) & 0xff) << 8) | (((b) & 0xff) << 16) | (((a) & 0xff) << 24))
#define WIN52_GET_COLOR_CHANNEL(col, ch) (((col) >> (WIN52_min(ch, 3)) << 3) & 0xff)
#define WIN52_MODIFY_COLOR_CHANNEL(col, ch, val) ((col) ^ ((col) & (0xff << (WIN52_min(ch, 3) << 3)))) | (((val) & 0xff) << (WIN52_min(ch, 3) << 3))

namespace Win52 {
	class Color {
	private:
		unsigned char r = 0, g = 0, b = 0, a = 255;
		u32 i = 0;
		void __i_compute() {
			this->i = WIN52_MAKECOLOR(this->r, this->g, this->b, this->a);
		}
		void __ch_compute() {
			this->r = WIN52_GET_COLOR_CHANNEL(this->i, 0);
			this->g = WIN52_GET_COLOR_CHANNEL(this->i, 1);
			this->b = WIN52_GET_COLOR_CHANNEL(this->i, 2);
			this->a = WIN52_GET_COLOR_CHANNEL(this->i, 3);
		}
	public:
		Color(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0, unsigned char a = 255) {
			this->i = WIN52_MAKECOLOR(r,g,b,a);
		}
		Color(u32 i) {
			this->i = i;
		}
		u32 rgb() {
			return this->i & 0xffffff;
		}
		u32 rgba() {
			return this->i;
		}
		COLORREF cref() {
			return this->rgb();
		}
		unsigned char red() {
			return WIN52_GET_COLOR_CHANNEL(this->i, 0);
		}
		unsigned char green() {
			return WIN52_GET_COLOR_CHANNEL(this->i, 1);
		}
		unsigned char blue() {
			return WIN52_GET_COLOR_CHANNEL(this->i, 2);
		}
		unsigned char alpha() {
			return WIN52_GET_COLOR_CHANNEL(this->i, 3);
		}
		void setR(unsigned char r) {
			this->i = WIN52_MODIFY_COLOR_CHANNEL(this->i, 0, r);
		}
		void setG(unsigned char g) {
			this->i = WIN52_MODIFY_COLOR_CHANNEL(this->i, 1, g);
		}
		void setB(unsigned char b) {
			this->i = WIN52_MODIFY_COLOR_CHANNEL(this->i, 2, b);
		}
		void setA(unsigned char a) {
			this->i = WIN52_MODIFY_COLOR_CHANNEL(this->i, 3, a);
		}
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
}