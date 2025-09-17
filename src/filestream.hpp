#include "bytestream.hpp"
#include "bitstream.hpp"
#include <fstream>

class FileByteStream : ByteStream {
private:
    std::fstream stream;
    size_t f_size = 0;
    size_t write_begin = 0;
public:
    FileByteStream(std::string src);
    size_t seek(size_t pos);
};

class FileBitStream : BitStream {
private:
    std::fstream stream;
    size_t f_size = 0;
    size_t write_begin = 0;
public:
    FileBitStream(std::string src);
    size_t seek(size_t pos);
};