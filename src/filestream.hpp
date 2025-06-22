#include "bytestream.hpp"
#include <fstream>

class FileStream : ByteStream {
private:
    std::fstream stream;
    size_t f_size = 0;
    size_t write_begin = 0;
public:
    FileStream(std::string src);
    size_t seek(size_t pos);
};