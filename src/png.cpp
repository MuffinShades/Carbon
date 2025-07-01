#include "msutil.hpp"
#include "png.hpp"
#include "filewrite.hpp"
#include "bitmap.hpp"
#include "bitstream.hpp"
#include "logger.hpp"

static Logger l;

constexpr u64 sig[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
constexpr size_t sigSz = 8;

enum chunk_type {
	NULL_CHUNK = 0x0,
	IHDR = 0x49484452,
	IDAT = 0x49444154,
	IEND = 0x49454e44,
	sRGB = 0x73524742,
	gAMA = 0x67414d41,
	PLTE = 0x504c5445
};

struct png_chunk {
	size_t len;
	chunk_type type;
	byte* dat;
	u32 checksum;
	bool good = false;
};

struct _IDAT {
	byte* dat;
	size_t sz;
};

struct _IHDR {
	size_t w, h;
	byte bitDepth;
	Png_ColorSpace colorSpace;
	byte compressionMethod = 0;
	byte filterType = 0;
	bool interlaced;
	size_t bytesPerPixel, nChannels, bpp;
};

chunk_type int_to_cType(int ty) {
	switch (ty) {
	case IHDR:
		return IHDR;
	case IDAT:
		return IDAT;
	case IEND: 
		return IEND;
	case sRGB: 
		return sRGB;
	case PLTE:
		return PLTE;
	case gAMA:
		return gAMA;
	}

	return NULL_CHUNK;
}

//read chunk
png_chunk readNextChunk(ByteStream* stream) {
	png_chunk res = {
		.len = stream->readUInt32()
	};
	i32 iTy = stream->readUInt32();
	res.type = int_to_cType(iTy);

	std::cout << "Chunk Read: " << res.type << " " << res.len << std::endl;

	if (res.len > 0)
		res.dat = new byte[res.len];
	else {
		res.dat = nullptr;
		return res;
	}

	ZeroMem(res.dat, res.len);
	
	for (size_t i = 0; i < res.len; i++)
		res.dat[i] = stream->readByte();

	res.checksum = stream->readUInt32();
	res.good = true;
	return res;
};

_IDAT ProcessIDAT(png_chunk c) {
	//inflate everything
	//BinDump::dump(c.dat, c.len);
	balloon_result decompressed_dat = Balloon::Inflate(c.dat, c.len);
	
	return {
		.dat = decompressed_dat.data,
		.sz = decompressed_dat.sz
	};
}

size_t GetNChannelsFromColorSpace(Png_ColorSpace color) {
	switch (color) {
	case Png_Color_GrayScale:
		return 1;
	case Png_Color_GrayScale_Alpha:
		return 2;
	case Png_Color_RGB:
		return 3;
	case Png_Color_RGBA:
		return 4;
	case Png_Color_Indexed:
		return 1;
	default:
		return 0;
	}
}

_IHDR ProcessIDHR(png_chunk hChunk) {
	if (!hChunk.good || !hChunk.dat || hChunk.len <= 0)
		return {0};

	ByteStream h_stream = ByteStream(hChunk.dat, hChunk.len);

	h_stream.int_mode = IntFormat_BigEndian;

	l.LogHex(h_stream.getBytePtr(), h_stream.size());

	_IHDR header = {
		.w = h_stream.readUInt32(),
		.h = h_stream.readUInt32(),
		.bitDepth = h_stream.readByte(),
		.colorSpace = (Png_ColorSpace) h_stream.readByte(),
		.compressionMethod = h_stream.readByte(),
		.filterType	= h_stream.readByte(),
		.interlaced = (bool) (h_stream.readByte() & 1)
	};

	if (header.compressionMethod != 0 || header.filterType != 0)
		return { 0 };

	//set values of bytesPerPixel, bpp, and nChannels
	header.bytesPerPixel = 
		mu_max(1,(header.bpp = (header.bitDepth * 
		(header.nChannels = GetNChannelsFromColorSpace(header.colorSpace)))) >> 3);

	return header;
}

//defilter instance used for defiltering step
struct DefilterContext {
	size_t scanY = 0, scanX = 0, p = 0;
	const size_t bytesPerScan;
	const _IHDR *hdr = nullptr;
	byte* stream = nullptr, *oStream = nullptr, *pStream = nullptr, *end = nullptr, *o_end = nullptr;
};

//helper function(s) for defilter inst
static byte defilter_context_next(DefilterContext* inst) {
	if (inst->stream + inst->p >= inst->end)
		return 0;
	return inst->stream[inst->p++];
}

static byte defilter_context_cur(DefilterContext* inst) {
	if (inst->stream + inst->p >= inst->end)
		return 0;
	return inst->stream[inst->p];
}

//gets a or value to left
static byte defilter_getLeft(DefilterContext* inst) {
	if (!inst || inst->scanX < inst->hdr->bytesPerPixel || !inst->pStream) return 0;
	return *(inst->pStream + (inst->p - inst->hdr->bytesPerPixel - (inst->scanY+1))); //subtract scanY+1 to account for the 1 byte offset the output buffer length is from the raw data buffer each line (from filter type)
}

//gets b or value top
static byte defilter_getTop(DefilterContext* inst) {
	if (!inst || inst->scanY <= 0 || !inst->pStream) return 0;
	return *(inst->pStream + (inst->p - inst->bytesPerScan - (inst->scanY+1)));
}

//gets c or value top left
static byte defilter_getTopLeft(DefilterContext* inst) {
	if (!inst || inst->scanX < inst->hdr->bytesPerPixel || inst->scanY <= 0  || !inst->pStream) return 0;
	return *(inst->pStream + (inst->p - inst->bytesPerScan - inst->hdr->bytesPerPixel - (inst->scanY+1)));
}

#include <cmath>

//paeth preditionct
byte paeth_predict(DefilterContext* inst) {
	if (!inst) return 0;
	const i16 a = defilter_getLeft(inst), b = defilter_getTop(inst), c = defilter_getTopLeft(inst);
	const i16
		p = a + b - c,
		pa = abs(p - a),
		pb = abs(p - b),
		pc = abs(p - c),
		pMin = mu_min(pa, mu_min(pb,pc));

	if (pMin == pa) return a;
	if (pMin == pb) return b;
	if (pMin == pc) return c;
}

//defilter methods
class DefilterMethod {
public: 
	//sub defilter
	static void sub(DefilterContext* inst) {
		if (!inst || inst->oStream >= inst->o_end)
			return;
		
		*inst->oStream = (defilter_context_cur(inst) + defilter_getLeft(inst)) & 0xff;
	};
	//up defilter
	static void up(DefilterContext* inst) {
		if (!inst || inst->oStream >= inst->o_end)
			return;
		*inst->oStream = (defilter_context_cur(inst) + defilter_getTop(inst)) & 0xff;
	};
	//avg defilter
	static void avg(DefilterContext* inst) {
		if (!inst || inst->oStream >= inst->o_end)
			return;
		const i32 avg = (defilter_getLeft(inst) + defilter_getTop(inst)) >> 1;
		*inst->oStream = (defilter_context_cur(inst) + avg) & 0xff;
	};
	//paeth defilter, he
	static void paeth(DefilterContext* inst) {
		if (!inst || inst->oStream >= inst->o_end)
			return;
		*inst->oStream = (defilter_context_cur(inst) + paeth_predict(inst)) & 0xff;
	};
};

//converts the bgr colors to rgb since png encodes them as bgr for some reason
void bgr_fix(byte* dat, size_t sz, size_t nChannels) {
	byte* pix = dat, *end = dat + sz;

	do {
		std::swap(*pix, *(pix + 2)); //swap b and r
	} while ((pix += nChannels) < end);
}

//fixes flip and bgr issue
byte* defilterCleanUp(byte *dat, size_t sz, _IHDR* hdr) {
	if (!dat || sz <= 0 || !hdr)
		return nullptr;

	//first fix bgr -> rgb
	if (hdr->colorSpace == Png_Color_RGB || hdr->colorSpace == Png_Color_RGBA)
		switch (hdr->colorSpace) {
		case Png_Color_RGB:
			bgr_fix(dat, sz, 3);
			break;
		case Png_Color_RGBA:
			bgr_fix(dat, sz, 4);
			break;
		}

	//next flip image
	byte* flipped = new byte[sz];
	ZeroMem(flipped, sz);

	const size_t scanSz = hdr->w * hdr->bytesPerPixel;
	for (size_t scan = 0, p = 0; scan < hdr->h; scan++, p += scanSz)
		memcpy(flipped + p, dat + (sz - (p + scanSz)), scanSz);

	_safe_free_a(dat);

	//
	return flipped;
}

//defilter the png data
byte* defilterDat(byte* i_dat, const size_t datSz, _IHDR *hdr) {
	//BinDump::dump(i_dat, datSz);
	if (!hdr || !i_dat || datSz <= 0) return nullptr;

	if (hdr->colorSpace == Png_Color_Indexed) {
		std::cout << "Error, png color indexing not supported!" << std::endl;
		return nullptr;
	}

	if (hdr->bytesPerPixel <= 0) {
		std::cout << "Error, too little bytes per pixel!" << std::endl;
		return nullptr;
	}

	const size_t scanline = hdr->w * hdr->bytesPerPixel;
	const size_t defilterSz = (scanline * hdr->h);

	byte* out = new byte[defilterSz];
	ZeroMem(out, defilterSz);

	//create inst
	DefilterContext df_inst = {
		.bytesPerScan = scanline,
		.hdr = hdr,
		.stream = i_dat,
		.oStream = out,
		.pStream = out,
		.end = i_dat + datSz,
		.o_end = out + defilterSz
	};

	df_inst.scanY = 0;

	//y scan
	std::cout << "W: " << hdr->w << " | H: " << hdr->h << " | Mem Allocated: " << defilterSz << std::endl;
	for (; df_inst.scanY < hdr->h; df_inst.scanY++) {
		const byte method = defilter_context_next(&df_inst); //get defilter method

		//x scan
		for (df_inst.scanX = 0; df_inst.scanX < scanline; df_inst.scanX++) {
			switch (method) {

			//none
			case 0:
				*df_inst.oStream = defilter_context_cur(&df_inst);
				break;
			//sub
			case 1: 
				DefilterMethod::sub(&df_inst);
				break;
			//up
			case 2:
				DefilterMethod::up(&df_inst);
				break;
			//avg
			case 3:
				DefilterMethod::avg(&df_inst);
				break;
			//paeth
			case 4:
				DefilterMethod::paeth(&df_inst);
				break;
			default:
				l.Error("Failed to defilter png!");
				*df_inst.oStream = defilter_context_cur(&df_inst);
				//_safe_free_a(out);
				//return nullptr;
				break;
			}
			df_inst.p++;
			df_inst.oStream++;
		}
	}

	_safe_free_a(i_dat);
	//return defilterCleanUp(out, osz, hdr);
	return out;
};

//decode a file
png_image PngParse::Decode(std::string src) {
	png_image rs;
	if (src == "" || src.length() <= 0)
		return rs;
	file fDat = FileWrite::readFromBin(src);

	//error check
	if (!fDat.dat) {
		std::cout << "Failed to read png..." << std::endl;
		return rs;
	}
	rs = PngParse::DecodeBytes(fDat.dat, fDat.len);
	delete[] fDat.dat;
	return rs;
}

//to free png chunk
void free_png_chunk(png_chunk* p) {
	if (p && p->dat) {
		delete[] p->dat;
		p->dat = nullptr;
		p->len = 0;
		p->checksum = 0;
		p->type = NULL_CHUNK;
	}
};

#define MSFL_PNG_DEBUG

png_image PngParse::DecodeBytes(byte* bytes, size_t sz) {

	ByteStream stream = ByteStream(bytes, sz);
	stream.int_mode = IntFormat_BigEndian; //set stream mode to big endian since that's what pngs use

	png_image rpng;

	//get for signature
	for (auto sig_byte : sig)
		if (stream.readByte() != sig_byte) {
			std::cout << "[png error] invalid png sig!" << std::endl;
			return rpng;
		}

	//read in header chunk
	png_chunk headChunk = readNextChunk(&stream);

	if (headChunk.type != IHDR) {
		std::cout << "[png error] first chunk was not a header chunk!" << std::endl;
		return rpng;
	}

	//parse header chunk
	_IHDR png_header = ProcessIDHR(headChunk);

#ifdef MSFL_PNG_DEBUG
	std::cout << "Png Header: \n\tWidth: " << png_header.w << "\n\tHeight: " << png_header.h << "\n\tBit Depth: " << png_header.bitDepth << "\n\tColor Space" << png_header.colorSpace << "\n\tFilter Method: " << png_header.filterType << "\n\tInterlacing: " << png_header.interlaced << std::endl;
#endif

	free_png_chunk(&headChunk);

	//parse other chunks till first IDat chunk
	bool idat_found = false;
	png_chunk Idat1;
	for (;;) {
		png_chunk dat = readNextChunk(&stream);

		switch (dat.type) {
		case IHDR: break;
		case sRGB: { //eh idk whats in this chunk anyway
			break;
		}
		case IDAT: {
			idat_found = true;
			Idat1 = dat;
			break;
		}
		default: {
			if (dat.len > 0)
				free_png_chunk(&dat);
			break;
		}
		}

		if (idat_found)
			break;

		if (dat.type != NULL_CHUNK)
			free_png_chunk(&dat);

		if (stream.tell() >= stream.size() || dat.type == IEND) break;
	}

	//collect all the idat chunks
	size_t totalIDATSz = Idat1.len;

	std::vector<png_chunk> iData = { Idat1 };

	png_chunk curIdata = iData[0];

	bool extraChunks = true;

	for (;;) {
		curIdata = readNextChunk(&stream);

		switch (curIdata.type) {
		case IDAT:
			iData.push_back(curIdata);
			totalIDATSz += curIdata.len;
			std::cout << "I data sz: " << curIdata.len << std::endl;
			break;
		case IEND:
			extraChunks = false;
			break;
		}

		if (curIdata.type == IEND || curIdata.type != IDAT) {
			free_png_chunk(&curIdata);
			break;
		}
	}

	//allocate and stitch togethera all iData chunks
	byte *compressedIdata = new byte[totalIDATSz];
	size_t compressedIdataSz = 0;
	ZeroMem(compressedIdata, totalIDATSz);
	size_t curCopy = 0;

	i32 _Dbg_Count = 0;

	for (const png_chunk& idatChunk : iData) {
		size_t chunkLen;

		memcpy(compressedIdata + curCopy, idatChunk.dat, chunkLen = idatChunk.len);
		compressedIdataSz += idatChunk.len;
		free_png_chunk(const_cast<png_chunk*>(&idatChunk));

		curCopy += chunkLen;

		if (curCopy >= totalIDATSz)
			break;

		//if (++_Dbg_Count == 3)
		//	break;
	}

	iData.clear();
	
	//decompress all the idata chunks
	const size_t expectedDataSz = png_header.w * png_header.h * png_header.bytesPerPixel + png_header.h; // add png_header.h for the defilter addition stuff
	byte* imgDat;
	size_t iDPos = 0;

	l.Log("Compressed Data: ");
	l.LogHex(compressedIdata, mu_min(256, compressedIdataSz));

	//FileWrite::writeToBin("idatDumpCompressed.bin", compressedIdata, compressedIdataSz);
	
	balloon_result rawImgData = Balloon::Inflate(compressedIdata, compressedIdataSz);

	//FileWrite::writeToBin("idatDumpDecompressed.bin", rawImgData.data, rawImgData.sz);

	_safe_free_a(compressedIdata);

	imgDat = rawImgData.data;
	size_t decodeDatSz = rawImgData.sz;

	l.Log("Decompressed Data: ");
	l.LogHex(imgDat, mu_min(256, decodeDatSz));

	std::cout << "Expected Size: " << expectedDataSz << " | Actual Size: " << decodeDatSz << std::endl;

	//TODO: decode additional (non required) chunks
	//these are not IDAT chunks btw
	//just random ancillary chunks
	if (extraChunks) {

	}

	//defilter the data
	imgDat = defilterDat(imgDat, decodeDatSz, &png_header);

	const size_t defilterSize = png_header.w * png_header.h * png_header.bytesPerPixel;

	//for testing just write data to a bitmap
	Bitmap testOut;
	testOut.header.w = png_header.w;
	testOut.header.h = png_header.h;
	testOut.header.fSz = decodeDatSz - png_header.h;
	testOut.header.bitsPerPixel = png_header.bpp;
	testOut.data = imgDat;

	l.Log("bpp: "+std::to_string(png_header.bpp));

	l.DrawBitMapClip(70,70,testOut);

	BitmapParse::WriteToFile("testpngread.bmp", &testOut);

	return {
		.data = imgDat,
		.sz = decodeDatSz,
		.width = png_header.w,
		.height = png_header.h,
		.channels = (i32) png_header.nChannels,
		.colorMode = png_header.colorSpace,
		.bitDepth = png_header.bitDepth
	};
}

bool PngParse::Encode(std::string src, png_image p) {
	if (src.length() <= 0 || src == "" || p.sz <= 0 || !p.data)
		return false;

	if (p.colorMode == Png_Color_Indexed) {
		std::cout << "Error! Png color indexing not supported yet!" << std::endl;
		return false;
	}

	/*_IHDR hdr = {
		.w = p.width,
		.h = p.height,
		.bitDepth = p.bitDepth,
		.colorSpace = p.colorMode,
		.compressionMethod = h_stream.readByte(),
		.filterType = h_stream.readByte(),
		.interlaced = (bool)(h_stream.readByte() & 1)
	};*/
}