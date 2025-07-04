#pragma once
#include <iostream>
#include "msutil.hpp"

struct mp3_stream {

};

class Mp3Parse {
public:
	mp3_stream Decode(std::string src);
	mp3_stream DecodeBytes(byte* data, size_t sz);
};