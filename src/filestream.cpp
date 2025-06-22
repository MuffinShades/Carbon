#include "filestream.hpp"

FileStream::FileStream(std::string src) {
    if (src.length() == 0) return;

    stream = std::fstream(src, std::ios::in | std::ios::out | std::ios::binary);

    if (!stream.good()) {
        std::cout << "Failed to load filestream! Make sure the path provided is valid! (" << src << ")" << std::endl;
        return;
    }

    stream.seekg(std::ios::end);
    this->f_size = stream.tellg();
    stream.seekg(std::ios::beg);

    this->ByteStream::enable_manual_mode();
}

size_t FileStream::seek(size_t pos) {
    stream.seekg(pos);
    stream.seekp(pos);
    return ByteStream::seek(pos);
}