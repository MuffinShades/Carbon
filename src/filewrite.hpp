#pragma once
#include <iostream>
#include "types.hpp"

struct file {
	size_t len = 0;
	byte* dat = nullptr;
};

struct text_file {
	size_t len = 0;
	size_t buf_sz = 0;
	char* dat = nullptr;
};

class FileWrite {
public:
	static bool writeToBin(std::string src, byte* dat, size_t sz);
	static file readFromBin(std::string src);
	static text_file readFromText(std::string src);
	static bool writeToText(std::string src, char* text, i64 len = -1);
};