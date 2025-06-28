#include "logger.hpp"

#define _COLOR_COMPARE(c1, c2) ((c1).r == (c2).r && (c1).g == (c2).g && (c1).b == (c2).b)

//static values for the ConsoleStateHandler
Color ConsoleStateHandler::_ccfg = { 255, 0, 0 };
Color ConsoleStateHandler::_ccbg = { 0, 0, 0 };

//changes console color
void Logger::SetSpectrumOutputColor(Color fg, Color bg) {

	//make sure we dont update color twice
	if (!_COLOR_COMPARE(fg, ConsoleStateHandler::_ccfg) && !fg._auto){
		std::cout << "\x1B[38;2;" << (int)fg.r << ";" << (int)fg.g << ";" << (int)fg.b << "m";
		ConsoleStateHandler::_ccfg = fg;
	}
	if (!_COLOR_COMPARE(bg, ConsoleStateHandler::_ccbg) && !bg._auto) {
		std::cout << "\x1B[48;2;" << (int)bg.r << ";" << (int)bg.g << ";" << (int)bg.b << "m";
		ConsoleStateHandler::_ccbg = bg;
	}
}

void Logger::SetSpectrumOutputColor(std::stringstream& stream, Color fg, Color bg) {

	//make sure we dont update color twice
	if (!_COLOR_COMPARE(fg, ConsoleStateHandler::_ccfg) && !fg._auto) {
		stream << "\x1B[38;2;" << (int)fg.r << ";" << (int)fg.g << ";" << (int)fg.b << "m";
		ConsoleStateHandler::_ccfg = fg;
	}
	if (!_COLOR_COMPARE(bg, ConsoleStateHandler::_ccbg) && !bg._auto) {
		stream << "\x1B[48;2;" << (int)bg.r << ";" << (int)bg.g << ";" << (int)bg.b << "m";
		ConsoleStateHandler::_ccbg = bg;
	}
}

void Logger::PrintColor(std::string dat, Color fg, Color bg) {
	if (dat.length() <= 0) return;
	
	this->SetSpectrumOutputColor(
		fg,
		bg
	);

	std::cout << dat << std::endl;
}

void Logger::Log(std::string dat, Color c) {
	if (!c._auto) {
		this->PrintColor(
			"[Log] "+dat,
			c,
			{ ._auto = true }
		);
		this->SetSpectrumOutputColor(this->_Desc.logColor);
	} else 
		this->PrintColor(
			"[Log] "+dat,
			this->_Desc.logColor,
			{ ._auto = true }
		);
}

void Logger::Warn(std::string dat) {
	this->PrintColor(
		"[Warning] "+dat,
		this->_Desc.warnColor,
		{ ._auto = true }
	);
}

void Logger::Error(std::string dat) {
	this->PrintColor(
		"[Error] "+dat,
		this->_Desc.errColor,
		{ ._auto = true }
	);
}

void Logger::Inform(std::string dat) {
	this->PrintColor(
		"[Info (i)] " + dat,
		this->_Desc.infoColor,
		{ ._auto = true }
	);
}

void Logger::PrintSeparator(bool adjustColor) {
	if (adjustColor)
		this->SetSpectrumOutputColor(this->_Desc.seperatorColor, { ._auto = true });

	std::cout << this->sep << std::endl;
}

void Logger::SetSeparatorSize(size_t sz) {
	if (sz <= 0) return;

	std::string s = "";

	for (size_t i = 0; i < sz; i++)
		s += '-';

	this->sep = s;
}

int iLog16(int v) {
	int r = 0;
	while (v >>= 4) r++;
	return r;
}

std::string GenSpaceString(size_t sz) {
	std::string res;
	while (sz--) res += ' ';
	return res;
}

std::string pHex(int v, size_t t) {
	std::stringstream _st;
	_st << std::hex << std::setfill('0') << std::setw(t) << v;
	return _st.str();
}


void Logger::LogHex(unsigned char* dat, size_t nBytes, HexSettings disp_settings) {
	const size_t nRows = floor(nBytes / disp_settings.bytesPerRow);

	size_t rowDispLen = iLog16(nRows) + 1;

	std::string tlSpace = GenSpaceString(rowDispLen);

	//first display byte thing at top row
	this->SetSpectrumOutputColor({ 128,128,128 }, { ._auto = true });
	std::cout << tlSpace << " ";
	for (int i = 0; i < disp_settings.bytesPerRow; i++)
		std::cout << pHex(i, 2) << " ";
	std::cout << std::endl;

	//display data
	size_t x = 0, y = 0, cb = 0;

	const int bpr = disp_settings.bytesPerRow;

	while (cb < nBytes) {
		this->SetSpectrumOutputColor({ 128,128,128 }, { ._auto = true });
		std::cout << pHex(y, rowDispLen) << " ";
		this->SetSpectrumOutputColor({ 255,255,255 }, { ._auto = true });
		x = bpr;
		while (x--)
			if (cb < nBytes)
				std::cout << pHex(dat[cb++], 2) << " ";
			else
				break;

		//TODO prevent overflow and display properly on shorter lines
		if (disp_settings.displayChars) {
			std::cout << std::dec << "   ";

			//note the value in x is the number of bytes left over
			int overflow = x + 1;
			const std::string s = GenSpaceString(2);
			while (overflow-- > 0) std::cout << s << " ";

			//TODO: fix this logging since the loop breaks when the row ends before the
			// diaply end thingy

			for (int i = bpr - x; i > 0; i--) {
				const size_t p = cb - i + 1;
				if (p >= nBytes) continue;
				char v = (char) dat[p];
				if (v >= ' ')
					std::cout << v;
				else
					std::cout << ".";
			}
		}

		y++;
		std::cout << std::endl;
	}
}

std::string Logger::LogProgress(float p, size_t w, bool log) {
	int fw = p * w;

	if (fw < 0) fw = 0;

	int dw = w - fw;

	if (dw < 0) dw = 0;

	std::stringstream tStream;

	this->SetSpectrumOutputColor(tStream, { 0,255,0 }, { ._auto = true });
	tStream << "[";
	while (fw--) tStream << "#";
	while (dw--) tStream << ".";
	tStream << "]";

	std::string s = tStream.str();

	if (log)
		std::cout << s << std::endl;

	return s;
}

void Logger::ClearLastLine() {
	std::cout << "\x1b[1A" << "\x1b[2K" << "\r";
}

void Logger::NextLine() {
	std::cout << std::endl;
}

template<class _T> void Logger::ArrPrint(_T* dat, size_t sz) {
	if (dat == nullptr || sz <= 0) return;

	
}

void Logger::SetLogBgColor(Color c) {
	//TODO: add this function
}

void Logger::SetLogColor(Color c) {
	this->_Desc.logColor = c;
}

void Logger::DrawBitMapClip(size_t renderW, size_t renderH, Bitmap bmp) {
	renderH >>= 1;
	const f32 dx = (f32) bmp.header.w / (f32) renderW;
	const f32 dy = (f32) bmp.header.h / (f32) renderH;

	u32 x, y;
	f32 ix = 0, iy = 0;

	const byte* bmp_data = bmp.data;
	const size_t by_pp = bmp.header.bitsPerPixel >> 3;

	if (by_pp < 3) {
		this->Warn("Cannot render bitmap! Color space must be either rgb or rgba!");
		return;
	}

	const size_t rowLen = bmp.header.w;

	bool stripe_ln = 1;

	for (y = 0; y < renderH; y++) {
		this->SetSpectrumOutputColor({._auto = true}, {
				0,0,0
		});
		std::cout << "\n";

		for (x = 0; x < renderW; x++) {
			const size_t iPos = (floor(ix) + floor(iy) * bmp.header.w) * by_pp;

			this->SetSpectrumOutputColor({._auto = true}, {
				bmp_data[iPos + 0],
				bmp_data[iPos + 1],
				bmp_data[iPos + 2]
			});

			std::cout << " ";
			ix += dx;
		}

		this->SetSpectrumOutputColor({._auto = true}, {
				(byte)((128*(i32)stripe_ln)+127), (byte)((128*(i32)stripe_ln)+127), (byte)((128*(i32)stripe_ln)+127)
		});

		stripe_ln = !stripe_ln;
		std::cout << " ";

		ix = 0;
		iy += dy;
	}

	this->SetSpectrumOutputColor({._auto = true}, {0,0,0});
	std::cout << " ";

	std::cout << std::endl;
}