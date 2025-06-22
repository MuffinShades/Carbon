#include "bitstream.hpp"
#include "msutil.hpp"

void BitStream::pos_adv() {
	this->subPos = 0;
	this->ByteStream::pos_adv();
}

void BitStream::byte_adv_nc(size_t nBytes) {
	this->cur += nBytes;
	this->blockPos += nBytes;
	this->pos += nBytes;
	if (this->pos >= this->len) {
		this->len = this->pos + 1;
	}
}

void BitStream::block_adv_check() {
	size_t itr = 0;

	while (this->blockPos >= this->cur_block->sz) {
		const size_t pSave = this->pos;

		if (!this->block_adv(1,1)) {
			std::cout << "Block adv failed!" << std::endl;
		}

		const size_t pDiff = pSave - this->pos;

		//re-adjust positions
		this->byte_adv_nc(pDiff);

		if (++itr >= 10) {
			std::cout << "BitStream error! Blocksize too small!" << std::endl;
			break;
		}
	}
}

void BitStream::bit_adv(size_t nBits) {
	const size_t nbRead = nBits >> 3, bOverflowRead = nBits & 7;

	this->subPos += bOverflowRead; //sub pos change

	const size_t jb = this->subPos >= 8;
	this->subPos -= 8 * jb;
	this->byte_adv_nc(jb + nbRead);
	this->block_adv_check();
}

void BitStream::byte_adv(size_t nBytes) {
	this->byte_adv_nc(nBytes);
	this->block_adv_check();
}

void BitStream::writeBit(bit b) {
	if (!this->cur) return;

	if (this->pos >= this->len)
		this->len_inc(this->pos - this->len + 1);

	*this->cur |= (b & 1) << this->subPos;
	this->bit_adv(1);

	//this->writeBits(b,1);
}


void BitStream::writeByte(byte b) {
	/*if (!this->cur) return;
	size_t lsz = (7 - this->subPos) + 1;
	*this->cur |= (b & MAKE_MASK(lsz)) >> this->subPos; //left hand side
	this->pos = this->len;
	this->len_inc();
	*this->cur = b & this->subPos;*/

	this->writeBits(b, 8); //I feel like write bits is just faster tbh
						   //but I guess there could be some optmizations?
}

struct LR_Split {
	size_t req;
	size_t nLBits, nRBits;
	size_t lMask, rMask;
};

inline size_t __computeReq(const size_t nBits, const size_t subPos) {
	return (nBits >> 3) + ((subPos + (nBits & 7)) > 7);
}

inline LR_Split __getBitSplit(const size_t nBits, const size_t subPos) {
	// Compute number of left hand side bits and make a mask
	//
	//   Sub Pos            Left Hand Copy
	// +---------+ +.......................... (...)+
	// 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0 | 0 0  ... -> 64
	// +--------------------------------------------+
	const size_t nLBits = mu_nb_min(64 - subPos, nBits);
	//right hand
	const i16 ext = (signed) (nBits - nLBits);
	const size_t nRBits = ext * (ext >> 15);

	return {
		.req = __computeReq(nBits, subPos),
		.nLBits = nLBits,
		.nRBits = nRBits,
		.lMask = MAKE_MASK_64(nLBits),
		.rMask = MAKE_MASK_64(nRBits)
	};
}

#define __context_bitStream_rawWrite(nBits, type, val, buf) \
	type* buf = (type*) this->cur;					   		\
	(*buf) |= (val << this->subPos);				   		\
	this->bit_adv(nBits)

//writes the bits of a val to the stream
//this functions works except for when splitting on block ends ;-;
//some sort of misalignment is happening
void BitStream::writeBits(u64 val, size_t nBits) {
	if (nBits <= 0 || nBits > 64) return;

	LR_Split split = __getBitSplit(nBits, this->subPos);

	if (__getOSEndian() == IntFormat_BigEndian) {
		val = endian_swap(val, 8);
		val >>= (64 - nBits);
	}

	u64 R_VAL = 0, L_VAL = 0;

	//split needed, do what is done with writeInt in ByteStream
	if (!this->cur_block || this->blockPos + split.req >= this->cur_block->sz) {
		if (!this->cur_block) {
			this->add_new_block(mu_max(this->blockAllocSz, 8));
		} else {
			split.nLBits = 
				((this->cur_block->sz - (this->blockPos + 1)) << 3) + //bytes left in block * 8
				((7 - this->subPos) + 1); 						      //bits left in sub-byte
			split.lMask = MAKE_MASK_64(split.nLBits);
			split.nRBits = nBits - split.nLBits;
			split.rMask = MAKE_MASK_64(split.nRBits);
		}

		//std::cout << "Split At: " << this->len << " | Split Inf: L: " << split.nLBits << " R: " << split.nRBits << " | " << this->subPos << std::endl;

		L_VAL = (val & split.lMask);
		R_VAL =  ((val >> split.nLBits) & split.rMask);
	} else {
		L_VAL = ((val >> split.nRBits) & split.lMask);
		R_VAL =  (val & split.rMask);
	}

	__context_bitStream_rawWrite(split.nLBits, u64, L_VAL, lwb); //left copy
	__context_bitStream_rawWrite(split.nRBits, u64, R_VAL, rwb); //right copy
}

void BitStream::writeBytes(byte* dat, size_t sz) {
	if (!dat || sz <= 0) return;
}

void BitStream::writeInt(i64 val, size_t nBytes) {
	this->writeBits(val, nBytes << 3);
}

void BitStream::writeUInt(u64 val, size_t nBytes) {
	this->writeBits(val, nBytes << 3);
}

void BitStream::adv_extra_check() {
	const size_t jb = this->subPos >= 8;
	this->subPos -= jb << 3;
	this->byte_adv(jb);
}

//read functionality
bit BitStream::readBit() {
	bit b = (*this->cur >> this->subPos) & 1;
	this->subPos++;

	//jump thingy
	this->adv_extra_check();

	//return :O (no way!)
	return b;
}

#define __context_bitStream_rawRead(nBits, type, out_var, mask)  \
	type out_var = ((*((type*)this->cur)) >> this->subPos) & mask; \
	this->bit_adv(nBits)

//TODO: swap endians and stuff if machine is not big endian
//this function needs to read u64* as big endian and if machine
//is little endian then some switch or something needs to happen
//use multiplication to make it branchless
u64 BitStream::readBits(size_t nBits) {
	if (nBits == 0 || nBits > 64 || !this->cur_block) return 0;

	LR_Split split = __getBitSplit(nBits, this->subPos);

	//should be able to adjust l and r in split if this is applicable
	if (split.req + this->blockPos >= this->cur_block->sz) {
		split.nLBits = 
			((this->cur_block->sz - (this->blockPos + 1)) << 3) + //bytes left in block * 8
			((7 - this->subPos) + 1); 						      //bits left in sub-byte
		split.lMask = MAKE_MASK_64(split.nLBits);
		split.nRBits = nBits - split.nLBits;
		split.rMask = MAKE_MASK_64(split.nRBits);

		const __context_bitStream_rawRead(split.nLBits, u64, lVal, split.lMask); //left
		const __context_bitStream_rawRead(split.nRBits, u64, rVal, split.rMask); //right

		//combine
		return (u64) ((rVal << split.nLBits) | (u64) lVal);
	} else {
		const __context_bitStream_rawRead(split.nLBits, u64, lVal, split.lMask); //left
		const __context_bitStream_rawRead(split.nRBits, u64, rVal, split.rMask); //right

		//combine
		return (u64) ((lVal << split.nRBits) | (u64) rVal);
	}
}

//TODO: this
byte* BitStream::readBytes(size_t sz) {
	return nullptr;
}

i64 BitStream::readInt(size_t nBytes) {
	return (signed) this->readBits(nBytes << 3);
}

u64 BitStream::readUInt(size_t nBytes) {
	return (unsigned) this->readBits(nBytes << 3);
}

byte BitStream::readByte() {
	return (byte) this->readBits(8);
}

//TODO: to implement simply do the same as BitStream::readBits but dont call adv
//right side may be a bit more difficult
//could create a temp version of cur
u64 BitStream::nextBits(size_t nBits) {
	if (nBits == 0 || nBits > 64 || !this->cur) return 0;

	LR_Split split = __getBitSplit(nBits, this->subPos);

	auto* l_buf = this->cur, *r_buf = this->cur + __computeReq(split.nLBits, this->subPos);

	//should be able to adjust l and r in split if this is applicable
	if (split.req + this->blockPos >= this->cur_block->sz) {
		split.nLBits = 
			((this->cur_block->sz - (this->blockPos + 1)) << 3) + //bytes left in block * 8
			((7 - this->subPos) + 1); 						      //bits left in sub-byte
		split.lMask = MAKE_MASK_64(split.nLBits);
		split.nRBits = nBits - split.nLBits;
		split.rMask = MAKE_MASK_64(split.nRBits);
		
		if (this->cur_block->next)
			r_buf = this->cur_block->next->dat;
		else
			return ((*((u64*) l_buf)) >> this->subPos) & split.lMask;
	}

	const u64 lVal = ((*((u64*) l_buf)) >> this->subPos) & split.lMask; //left
	const u64 rVal = ((*((u64*) r_buf)) >> this->subPos) & split.rMask; //right

	//combine
	return (u64) ((lVal << split.nRBits) | (u64) rVal);

}

void BitStream::skipBytes(size_t nBytes) {
	if (nBytes == 0) return;
	const size_t rj = this->subPos, lj = 7 - this->subPos;
	this->bit_adv(lj);
	this->byte_adv(nBytes - 1);
	this->bit_adv(rj);
}

//TODO: this function
void BitStream::skipBits(size_t nBits) {
	const size_t nByteSkip = nBits >> 3, nBitSkip = nBits & 7;
}

void BitStream::bitSeek(size_t bit) {
	this->seek(bit >> 3);
	this->subPos = bit & 7;
}

void BitStream::free() {
	this->subPos = 0;
	this->ByteStream::free();
}

void BitStream::__int_dbg() {
	for (int i = 0; i < 3; i++) {
		std::cout << (int)(*(this->cur_block->dat + i)) << " | " << (uintptr_t)this->cur_block->dat << ", " << (uintptr_t)(this->cur_block->dat+i) << ", " << (uintptr_t)(this->cur) << std::endl;
 	}
}

size_t BitStream::bitTell() {
	return (this->tell() << 3) + this->subPos;
}

size_t BitStream::subBitTell() {
	return this->subPos;
}

bool BitStream::canAdv() {
	return this->subPos < 7 || this->ByteStream::canAdv();
}