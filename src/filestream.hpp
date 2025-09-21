#include "bytestream.hpp"
#include "bitstream.hpp"
#include <fstream>

class FileByteStream : ByteStream {
private:
    std::fstream stream;
    size_t f_size = 0;
    size_t write_begin = 0;

    void f_stream_seek(size_t p);
    void load_new_block(size_t sz);
    void flush_delete_block(mem_block *mem_block);
    void flush_blocks_before(mem_block *block);
    bool block_adv(bool pos_adv = false, bool write = false) override;
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