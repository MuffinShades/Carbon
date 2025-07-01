#pragma once
#include "types.hpp"
#include "msutil.hpp"

/**
 * 
 * Fast Bytestream for Oxygen and Carvon
 * 
 * Copyright James Weigand 2025 All rights reserved
 * 
 * Version 1.0 [Current]
 * 	basic class
 * Version 1.1 [In Dev]
 * 	adding filestream capabilities
 *  better memory usage
 *  ability to write anywhere in the stream (fast!)
 *  stream settings & read-only streams
 *  safer data pointers
 */

#ifdef MSFL_DLL
#ifdef MSFL_EXPORTS
#define MSFL_EXP __declspec(dllexport)
#else
#define MSFL_EXP __declspec(dllimport)
#endif
#else
#define MSFL_EXP
#endif

#ifdef MSFL_DLL
#ifdef __cplusplus
extern "C" {
#endif
#endif

constexpr size_t MAX_BLOCK_SIZE = 0xffffffff, MAX_BLOCK_SIZE_LOG16 = 8;
constexpr size_t MIN_BLOCK_SIZE = 16;
constexpr size_t BYTESTREAM_DEFAULT_BLOCK_PADDING = 0xf;
constexpr size_t BYTESTREAM_DEFAULT_BLOCK_ALLOC = 0xffff;

struct mem_block {
	//sz -> # of bytes allocated in the block
	//data_len -> # of bytes that are actual data
	size_t sz = 0, pos = 0, padding = 1;
	size_t data_len = 0;
	mem_block* next = nullptr, *prev = nullptr;
	byte* dat = nullptr, *dat_end = nullptr;
	/*
	whether or not the memory block owns its data

	when set to false (recieving data from another source) this block 
	will not! free itself

	when freeed this block will just no longer point to the data

	DO NOT FREE DATA PASSED TO THE STREAM UNLESS YOU FREE THE STREAM!!!
	*/
	bool data_own = true; 
	u64 side_info = 0; //side info sub_classes can use for holding info
};

#define BYTESTREAM_ALIGNED_16

struct StreamExport {
	byte* dat;
	size_t sz;
	u32 crc;
};

struct StreamSettings {
	/*
	
	Whether or not the ByteStream is read-only
	
	*/
	bool readOnly = false;

	/*
	
	Whether or not the stream is protected

	When protected:
		- cannot get a raw byte pointer
		- something else i planned in my head but forgot
	
	*/
	bool protectedStream = false;

	/*
	
	Whether or not when adding data to the stream, liek bytes,
	the stream will copy all the data into either the current
	block or a new block or will create a reference block and
	disable the data_own flag on the new block
	
	*/
	bool copyAllData = false;
	/*
	
	Amount of memory to allocate with each new memory block

	Default is 64kb
	
	*/
	size_t blockAllocSz = BYTESTREAM_DEFAULT_BLOCK_ALLOC;
};

class ByteStream {
private:
	bool sz_aligned = false;
    bool manual_stream = false;
protected:
	enum block_write_status {
		stream_bws_canWriteAtCur = 0,
		stream_bws_canWriteWithSplit = 1,
		stream_bws_cannotWrite = 2
	};

	size_t blockAllocSz = BYTESTREAM_DEFAULT_BLOCK_ALLOC, 
		   blockAllocSzLog16 = 4, 
		   blockAllocSzLog2 = 16;
	#ifdef COMPILER_MODE_64_BIT
	size_t blockAllocSzAlignment = 5;
	#else
	size_t blockAllocSzAlignment = 4;
	#endif	

	mem_block* head_block = nullptr, *cur_block = nullptr, *tail_block = nullptr;
	size_t allocSz = 0, len = 0, pos = 0, nBlocks = 0, blockPos = 0;
	byte* cur = nullptr;
	mem_block* alloc_new_block(size_t blockSz);

	/*

	block_append important info
	
	nCalls is there since block repair calls this function and it calls block repair
	so infinite recurrsion could occur. It should be impossible but this is here as
	a backup since if this is running on some important system this recurrsion could
	REALLY screw stuff up.

	never set a value to nCalls, NEVER. just let the functions do their work

	*/
	void block_append(mem_block* block, const size_t nCalls = 0);
	void add_new_block(const size_t blockSz);
	void add_new_block(byte* dat, const size_t blockSz);
	/*

	don't set nCalls; it is simply there as a recurrsion check
	
	*/
	void block_repair(size_t nCalls = 0);
	void set_stream_data(byte *dat, size_t sz);
	bool block_adv(bool pos_adv = false, bool write = false);
	virtual void pos_adv();
	virtual void pos_adv(const size_t sz);

	void block_end();
	void set_cur_block(mem_block *block);

	virtual void len_inc();
	virtual void len_inc(const size_t sz);

	//more block helper functions
	bool atCurBlockEnd();
	block_write_status canWriteToCurBlock();

	mem_block *get_t_block(size_t pos);
	u32 mod_block_sz(const u64 val);
public:
	IntFormat int_mode = IntFormat_LittleEndian;

	MSFL_EXP ByteStream(byte* dat, size_t len);
	MSFL_EXP ByteStream(size_t allocSz);
	MSFL_EXP ByteStream();

	//write functions
	MSFL_EXP virtual void writeBytes(byte* dat, size_t sz);
	MSFL_EXP virtual void writeInt(i64 val, size_t nBytes = 4);
	MSFL_EXP virtual void writeUInt(u64 val, size_t nBytes = 4);
	MSFL_EXP virtual void writeByte(byte b);
	MSFL_EXP virtual void writeInt16(i16 val);
	MSFL_EXP virtual void writeUInt16(u16 val);
	MSFL_EXP virtual void writeInt24(i24 val);
	MSFL_EXP virtual void writeUInt24(u24 val);
	MSFL_EXP virtual void writeInt32(i32 val);
	MSFL_EXP virtual void writeUInt32(u32 val);
	MSFL_EXP virtual void writeInt48(i48 val);
	MSFL_EXP virtual void writeUInt48(u48 val);
	MSFL_EXP virtual void writeInt64(i64 val);
	MSFL_EXP virtual void writeUInt64(u64 val);
	MSFL_EXP virtual void multiWrite(u64 val, size_t valSz, size_t nCopy);

	//read functions
	MSFL_EXP virtual byte* readBytes(size_t sz);
	MSFL_EXP virtual i64 readInt(size_t nBytes);
	MSFL_EXP virtual u64 readUInt(size_t nBytes);
	MSFL_EXP virtual byte readByte();
	MSFL_EXP virtual i16 readInt16();
	MSFL_EXP virtual u16 readUInt16();
	MSFL_EXP virtual i24 readInt24();
	MSFL_EXP virtual u24 readUInt24();
	MSFL_EXP virtual i32 readInt32();
	MSFL_EXP virtual u32 readUInt32();
	MSFL_EXP virtual i48 readInt48();
	MSFL_EXP virtual u48 readUInt48();
	MSFL_EXP virtual i64 readInt64();
	MSFL_EXP virtual u64 readUInt64();

	//next functions
	MSFL_EXP virtual byte nextByte();
	
	MSFL_EXP void clear();
	MSFL_EXP void free();

	MSFL_EXP size_t home();
	MSFL_EXP size_t end();
	MSFL_EXP size_t seek(size_t pos);
	MSFL_EXP size_t tell();
	MSFL_EXP size_t size();
	MSFL_EXP void skip(size_t nBytes);
	MSFL_EXP void stepBack(size_t nBytes);

	MSFL_EXP void resize(size_t sz);
	MSFL_EXP void clip();
	MSFL_EXP void setMode(IntFormat mode);
	MSFL_EXP std::string readStr(size_t len);
	MSFL_EXP ByteStream* extract_substream();
	MSFL_EXP void __printDebugInfo();

	//safety functions
	MSFL_EXP void Repair();
	MSFL_EXP void EnableBinDumpOnFail();
	MSFL_EXP void ProtectStream();
	MSFL_EXP virtual bool canAdv();

	//export and import features
	MSFL_EXP void copyTo(byte* buffer, size_t copyStart, size_t copySz);
	MSFL_EXP void pack(); //packs all data into 1 chunk
	MSFL_EXP byte* getBytePtr(); //calls pack then returns pointer to base block

	MSFL_EXP StreamExport exportStream();
	MSFL_EXP StreamExport zlibExport();
	MSFL_EXP StreamExport errCorrectExport();
	
	MSFL_EXP bool importStream(StreamExport exp);
	MSFL_EXP bool importZStream(StreamExport exp);
	MSFL_EXP bool importECStream(StreamExport exp);

	//operators
	MSFL_EXP byte operator[](size_t i);

    //modes
    MSFL_EXP void enable_manual_mode();

	//settings
#ifdef BYTESTREAM_ALIGNED_16
	MSFL_EXP void setBlockAllocSz(const size_t log16Sz);
#else
	MSFL_EXP void setBlockAllocSz(const size_t sz);
#endif
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif