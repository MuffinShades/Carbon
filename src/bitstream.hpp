#pragma once
#include "bytestream.hpp"

/**
 *
 * Fast BitStream for Oxygen
 *
 * Copyright James Weigand 2025 All rights reserved
 *
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

class BitStream : public ByteStream {
	private:
		size_t subPos = 0;
		MSFL_EXP void adv_extra_check();
	protected:
		using ByteStream::skip;
		MSFL_EXP void pos_adv() override;
		MSFL_EXP void bit_adv(size_t nBits);
		MSFL_EXP void byte_adv(size_t nBytes);
		MSFL_EXP void byte_adv_nc(size_t nBytes);
		MSFL_EXP void block_adv_check();
	public:
		BitStream() : ByteStream() {};
		BitStream(size_t sz) : ByteStream(sz) {};
		BitStream(byte* dat, size_t sz) : ByteStream(dat, sz) {};
		MSFL_EXP void writeBytes(byte* dat, size_t sz) override;
		MSFL_EXP void writeInt(i64 val, size_t nBytes = 8) override;
		MSFL_EXP void writeUInt(u64 val, size_t nBytes = 8) override;
		MSFL_EXP void writeByte(byte b) override;

		MSFL_EXP void writeBit(bit b);
		MSFL_EXP void writeBits(u64 val, size_t nBits);

		MSFL_EXP byte* readBytes(size_t sz) override;
		MSFL_EXP i64 readInt(size_t nBytes) override;
		MSFL_EXP u64 readUInt(size_t nBytes) override;
		MSFL_EXP byte readByte() override;

		MSFL_EXP bit readBit();
		MSFL_EXP u64 readBits(size_t nBits);
		MSFL_EXP u64 nextBits(size_t nBits);

		MSFL_EXP void skipBytes(size_t nBytes);
		MSFL_EXP void skipBits(size_t nBits);

		MSFL_EXP void bitSeek(size_t bit);
		MSFL_EXP size_t subBitTell();
		MSFL_EXP size_t bitTell();
		MSFL_EXP void free();
		MSFL_EXP bool canAdv();
		void __int_dbg();
	};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif