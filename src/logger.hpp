#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>
#include "bitmap.hpp"
#include "Color.hpp"
//#ifdef WIN32
//#include <windows.h>
//#endif

/*enum ConsoleColor {
	CCBlack = 30,
	CCDarkRed = 31,
	CCDarkGreen = 32,
	CCGold = 33,
	CCDarkBlue = 34,
	CCDarkMagenta = 35,
	CCDarkCyan = 36,
	CCLightGray = 37,
	CCDarkGray = 90,
	CCRed = 91,
	CCGreen = 92,
	CCYellow = 93,
	CCBlue = 94,
	CCMagenta = 95,
	CCCyan = 96,
	CCWhite = 97
};*/

/*struct Color {
	unsigned char r, g, b;
	bool _auto = false;
};*/

struct LogDescriptor {
	Color logColor = { 255, 255, 255 },
		warnColor = { 255, 255, 0 },
		errColor = { 255, 0, 0 },
		infoColor = { 0, 128, 255 };

	Color seperatorColor = { 255, 255, 255 };
	size_t separatorSize = 25;
};

class ConsoleStateHandler {
public:
	static Color _ccfg;
	static Color _ccbg;
};

struct HexSettings {
	size_t bytesPerRow = 16;
	bool displayChars = true;
};

class Logger {
private:
	LogDescriptor _Desc;
	std::string sep;
	static void EnableConsoleColoring();
public:
	Logger() {
		this->SetSeparatorSize(this->_Desc.separatorSize);
		EnableConsoleColoring();
	}
	void Log(std::string dat);
	void Warn(std::string dat);
	void Error(std::string dat);
	template<class _ErrTy> _ErrTy ThrowableError(std::string dat) {
		this->Error(dat);
		return _ErrTy(dat);
	};
	void Inform(std::string dat);
	void PrintColor(std::string dat, Color fb, Color bg = {});
	void PrintSeparator(bool adjustColor = true);
	void SetSeparatorSize(size_t sz);
	void LogHex(unsigned char* dat, size_t nBytes, HexSettings disp_settings = {});
	std::string LogProgress(float p, size_t w, bool log = false);
	void ClearLastLine();
	void NextLine();
	void SetSpectrumOutputColor(Color fg, Color bg = {});
	void SetSpectrumOutputColor(std::stringstream& stream, Color fg, Color bg = {});
	void SetLogColor(Color c);
	void SetLogBgColor(Color c);
	void DrawBitMapClip(size_t renderW, size_t renderH, Bitmap bmp);
	template<class _ObjTy> inline void LogObjHex(_ObjTy obj) {
		this->LogHex(reinterpret_cast<unsigned char*>(&obj), sizeof(_ObjTy));
	}
	template<class _T> void ArrPrint(_T* dat, size_t sz);
};