#include "filestream.hpp"

//copied from bytestream.cpp
void free_block(mem_block* block) {
	if (!block) return;

	if (block->dat)
		_safe_free_a(block->dat);

	ZeroMem(block);

	_safe_free_b(block);
}
//------------------------

FileByteStream::FileByteStream(std::string src) {
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

size_t FileByteStream::seek(size_t pos) {
    this->f_stream_seek(pos);
    return this->ByteStream::seek(pos);
}

void FileByteStream::f_stream_seek(size_t p) {
    stream.seekg(p);
    stream.seekp(p);
}

void FileByteStream::flush_delete_block(mem_block *block) {
    if (!block) 
        return;

    this->f_stream_seek(block->side_info);
    this->stream.write((const char*)block->dat, block->sz);

    free_block(block);
}

void FileByteStream::flush_blocks_before(mem_block *block) {
    if (!block)
        return;

    mem_block *c_flush_block = block->prev, *_prev;

    while (c_flush_block) {
        _prev = c_flush_block->prev;
        this->flush_delete_block(c_flush_block);
        c_flush_block = _prev;
    }

    block->prev = nullptr;
}

void FileByteStream::load_new_block(size_t sz) {
    const size_t fileBytesLeft = this->f_size - this->pos;
    const u64 b_side_info = this->pos;

    if (fileBytesLeft < sz) {
        
    }
}

bool FileByteStream::block_adv(bool pos_adv, bool write) {
    if (this->cur_block && this->cur_block->next) {
		this->cur_block = this->cur_block->next;
		this->blockPos = 0;
		this->cur = this->cur_block->dat;
		if (pos_adv)
			this->pos = this->cur_block->pos;
		return true;
	} else if (write) {
		if (!this->cur_block)
			this->block_repair();
		this->read_new_block(this->blockAllocSz);
		return this->block_adv(pos_adv, false);
	}

	return false;
}