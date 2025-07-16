#include "filewrite.hpp"
#include <fstream>
#include "msutil.hpp"

bool FileWrite::writeToBin(std::string src, byte* dat, size_t sz) {
	//well you screwed up step one bruh
	if (src.length() <= 0 || !dat || sz <= 0) return false;

	std::ofstream ws(src, std::ios::out | std::ios::binary);

	if (!ws.good())
		return false; //breh

	ws.write(const_cast<const char*>(reinterpret_cast<char*>(dat)), sz); //write
	ws.close();
}

/**
 * 
 * 
 */
file FileWrite::readFromBin(std::string src) {
	if (src.length() <= 0) return {};

	std::ifstream is(src, std::ios::in | std::ios::binary);

	if (!is.good())
		return {};


	//get file length
	is.seekg(0, std::ios::end);
	size_t f_len = is.tellg();
	is.seekg(0, std::ios::beg);

	//read into buffer
	byte* buf = new byte[f_len];
	ZeroMem(buf, f_len);

	is.read(reinterpret_cast<char*>(buf), f_len);
	is.close();

	return {
		.len = f_len,
		.dat = buf
	};
}

static text_file readFromText(std::string src) {
if (src.length() <= 0) return {};

	std::ifstream is(src, std::ios::in | std::ios::binary);

	if (!is.good())
		return {};

	//get file length
	is.seekg(0, std::ios::end);
	size_t f_len = is.tellg(), b_len = f_len + 1;
	is.seekg(0, std::ios::beg);

	//read into buffer
	char* buf = new char[b_len];
	ZeroMem(buf, b_len);

	is.read(buf, f_len);
	is.close();

	return {
		.len = f_len,
		.buf_sz = b_len,
		.dat = buf
	};
}

static bool writeToText(std::string src, char* text, i64 len) {
	//well you again screwed up step one bruh
	if (src.length() <= 0 || !text) return false;

	//auto compute len by finding first NUL byte
	if (len < 0) {
		len = 0;

		char* c = text;

		while (*c++ != 0)
			++len;
	}

	std::ofstream ws(src, std::ios::out | std::ios::binary);

	if (!ws.good())
		return false; //breh

	ws.write(text, len); //write
	ws.close();	
}