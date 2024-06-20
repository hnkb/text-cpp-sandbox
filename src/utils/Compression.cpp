
// #define ENABLE_ZSTD
// #define ENABLE_LIBDEFLATE
#define ENABLE_BROTLI
// #define ENABLE_LZMA2
// #define ENABLE_SNAPPY
// #define ENABLE_ZMOLLY
// #define ENABLE_ZPAQ

#include "Compression.h"

#if defined(ENABLE_ZSTD)
#	include <zstd.h>
#endif
#if defined(ENABLE_LIBDEFLATE)
#	include <libdeflate.h>
#endif
#if defined(ENABLE_BROTLI)
#	include <brotli/encode.h>
#	include <brotli/decode.h>
#endif
#if defined(ENABLE_LZMA2)
#	include <lzma.h>
#endif
#if defined(ENABLE_SNAPPY)
#	include <snappy.h>
#endif

#include <stdexcept>

using namespace std;


inline Compression detectCompression(const uint8_t* data, size_t size)
{
#if defined(ENABLE_ZSTD)
	if (size > 6 && *(uint32_t*)data == 0xFD2F'B528)
		return Compression::Zstd;
#endif

#if defined(ENABLE_LZMA2)
	if (size > 6 && *(uint32_t*)data == 0x587a'37fd && *(uint16_t*)(data + 4) == 0x005a)
		return Compression::LZMA2;
#endif

#if defined(ENABLE_SNAPPY)
	if (size > 6 && string((char*)data, 6) == "sNaPpY")
		return Compression::Snappy;
#endif

#if defined(ENABLE_ZPAQ)
	if (size > 13 && *(uint64_t*)data == 0xd383'31a0'7453'6b37
		&& *(uint32_t*)(data + 8) == 0xb028'b28c && *(data + 12) == 0xd3)
		return Compression::ZPAQ;
#endif

#if defined(ENABLE_LIBDEFLATE)
	if (size > 6 && (data[0] & 0x8f) == 0x08 && ((int)data[0] * 256 + data[1]) % 31 == 0)
		return Compression::zlib;

	if (size > 18 && data[0] == 0x1f && data[1] == 0x8b && data[2] == 0x08)
		return Compression::gzip;

	// no pattern for raw deflate (except maybe the first two bits -frame type- are not 11)
	// as we haven't detected anything else, let's try defalte
	return Compression::Deflate;
#endif

#if defined(ENABLE_BROTLI)
	// no pattern for brotli (see https://stackoverflow.com/a/39032023)
	// but if we are here, let's assume it is brotli and try to decompress
	return Compression::Brotli;
#endif
}


#if defined(ENABLE_ZSTD)
inline void
	compressZstd(vector<uint8_t>& compressed, const uint8_t* data, size_t size, int level)
{
	// compressed.resize(ZSTD_compressBound(size));

	// const auto ret = ZSTD_compress(compressed.data(), compressed.size(), data, size, level);
	// if (ZSTD_isError(ret))
	// 	throw runtime_error("ZSTD_compress failed");
	// compressed.resize(ret);

	compressed.resize(ZSTD_compressBound(size));

	auto ctx = ZSTD_createCCtx();
	ZSTD_CCtx_setParameter(ctx, ZSTD_c_compressionLevel, level);
	ZSTD_CCtx_setParameter(ctx, ZSTD_c_nbWorkers, 12);

	const auto ret = ZSTD_compress2(ctx, compressed.data(), compressed.size(), data, size);

	ZSTD_freeCCtx(ctx);

	if (ZSTD_isError(ret))
		throw runtime_error("ZSTD_compress failed");
	compressed.resize(ret);
}

inline void decompressZstd(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize)
{
	const auto contentSize = ZSTD_getFrameContentSize(inputBuffer, inputSize);
	if (contentSize == ZSTD_CONTENTSIZE_ERROR || contentSize == ZSTD_CONTENTSIZE_UNKNOWN)
		throw runtime_error("ZSTD_getFrameContentSize failed");

	outputResize(contentSize);
	const auto actualSize =
		ZSTD_decompress(outputBuffer(), contentSize, inputBuffer, inputSize);
	if (ZSTD_isError(actualSize))
		throw runtime_error("ZSTD_compress failed.");
	outputResize(actualSize);
}
#endif


#if defined(ENABLE_LIBDEFLATE)
inline void compressDeflate(
	vector<uint8_t>& compressed,
	const uint8_t* data,
	size_t size,
	Compression method,
	int level)
{
	auto ld = libdeflate_alloc_compressor(level);

	if (method == Compression::gzip)
	{
		compressed.resize(libdeflate_gzip_compress_bound(ld, size));
		compressed.resize(
			libdeflate_gzip_compress(ld, data, size, compressed.data(), compressed.size()));
	}
	else if (method == Compression::zlib)
	{
		compressed.resize(libdeflate_zlib_compress_bound(ld, size));
		compressed.resize(
			libdeflate_zlib_compress(ld, data, size, compressed.data(), compressed.size()));
	}
	else
	{
		compressed.resize(libdeflate_deflate_compress_bound(ld, size));
		compressed.resize(
			libdeflate_deflate_compress(ld, data, size, compressed.data(), compressed.size()));
	}

	libdeflate_free_compressor(ld);
}

inline void decompressDeflate(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize)
{
	auto ld = libdeflate_alloc_decompressor();

	// uncompressed size is unknown, let's make a guess
	auto size = inputSize;

	if (method == Compression::gzip)
	{
		// gzip stores 32-bits of uncompressed size in the last four bytes of frame
		// see https://tools.ietf.org/html/rfc1952

		auto isize = *(int*)(inputBuffer + inputSize - 4);
		isize = (isize + 3) / 4;  // because we multiply by four inside next loop, divide by
								  // four here to compensate

		// As gzip frame only stores 32-bits of size, it is possible that the stored isize
		// is already smaller than the compressed size (for blocks of data exceeding 4GB).
		// In that case, do not use the invalid isize.
		if (isize > size)
			size = isize;
	}

	while (true)  // TODO: provide a limit to the number of loop iterations in case of error
	{
		auto bufferSize = size *= 4;
		outputResize(bufferSize);

		size_t actualSize = 0;
		auto result = LIBDEFLATE_BAD_DATA;

		if (method == Compression::gzip)
			result = libdeflate_gzip_decompress(
				ld,
				inputBuffer,
				inputSize,
				outputBuffer(),
				bufferSize,
				&actualSize);
		else if (method == Compression::zlib)
			result = libdeflate_zlib_decompress(
				ld,
				inputBuffer,
				inputSize,
				outputBuffer(),
				bufferSize,
				&actualSize);
		else
			result = libdeflate_deflate_decompress(
				ld,
				inputBuffer,
				inputSize,
				outputBuffer(),
				bufferSize,
				&actualSize);

		if (result == LIBDEFLATE_SUCCESS)
		{
			outputResize(actualSize);
			break;
		}

		if (result != LIBDEFLATE_INSUFFICIENT_SPACE)
			throw invalid_argument("libdeflate_deflate_decompress failed");
	}

	libdeflate_free_decompressor(ld);
}
#endif


#if defined(ENABLE_BROTLI)
inline void
	compressBrotli(vector<uint8_t>& compressed, const uint8_t* data, size_t size, int level)
{
	compressed.resize(BrotliEncoderMaxCompressedSize(size));
	size_t actual = compressed.size();
	if (!BrotliEncoderCompress(
			level,
			BROTLI_DEFAULT_WINDOW,
			BROTLI_DEFAULT_MODE,
			size,
			data,
			&actual,
			compressed.data()))
		throw runtime_error("BrotliEncoderCompress failed");
	compressed.resize(actual);
}

inline void decompressBrotli(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize)
{
	size_t actualSize = inputSize * 20;
	outputResize(actualSize);
	if (!BrotliDecoderDecompress(inputSize, inputBuffer, &actualSize, outputBuffer()))
		throw runtime_error(" failed");
	outputResize(actualSize);
}
#endif


#if defined(ENABLE_LZMA2)
inline void
	compressLZMA2(vector<uint8_t>& compressed, const uint8_t* data, size_t size, int level)
{
	compressed.resize(lzma_stream_buffer_bound(size));
	size_t actual = 0;
	if (lzma_easy_buffer_encode(
			level,
			LZMA_CHECK_CRC32,
			NULL,
			data,
			size,
			compressed.data(),
			&actual,
			compressed.size())
		!= LZMA_OK)
		throw runtime_error("lzma_easy_buffer_encode() failed");
	compressed.resize(actual);
}

inline void decompressLZMA2(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize)
{
	lzma_stream strm = LZMA_STREAM_INIT;

	if (lzma_stream_decoder(&strm, UINT64_MAX, 0) != LZMA_OK)
		throw runtime_error("failed to initialize LZMA decoder");

	vector<uint8_t> buffer(128 * 1024);
	strm.next_in = inputBuffer;
	strm.avail_in = inputSize;

	size_t outputSize = 0;

	do
	{
		strm.next_out = buffer.data();
		strm.avail_out = buffer.size();

		auto ret = lzma_code(&strm, LZMA_FINISH);

		if (ret != LZMA_OK && ret != LZMA_STREAM_END)
		{
			lzma_end(&strm);
			throw runtime_error("failed to decompress LZMA stream");
		}

		auto writeSize = buffer.size() - strm.avail_out;
		outputResize(outputSize += writeSize);
		copy_n(buffer.data(), writeSize, outputBuffer() + writeBegin);
		// TODO: this won't work when Type is larger than byte and stream boundary is in the
		// middle of one Type

		if (ret == LZMA_STREAM_END)
			break;
	}
	while (strm.avail_out == 0);

	lzma_end(&strm);
}
#endif


#if defined(ENABLE_SNAPPY)
inline void compressSnappy(vector<uint8_t>& compressed, const uint8_t* data, size_t size)
{
	compressed.resize(snappy::MaxCompressedLength(size));
	size_t actual = compressed.size();
	snappy::RawCompress((char*)data, size, (char*)compressed.data(), &actual);
	compressed.resize(actual);
}

inline void decompressSnappy(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize)
{
	string uc;
	if (!snappy::Uncompress((char*)inputBuffer, inputSize, &uc))
		throw invalid_argument("snappy decompression failed");
	outputResize(uc.size());
	copy(uc.begin(), uc.end(), outputBuffer());
}
#endif


#if defined(ENABLE_ZMOLLY)
#	include <istream>
#	include <sstream>
#	include <streambuf>

void zmolly_encode(istream& orig, ostream& comp);
void zmolly_decode(istream& comp, ostream& orig);

inline void compressZmolly(vector<uint8_t>& compressed, const uint8_t* data, size_t size)
{
	struct membuf : streambuf
	{
		membuf(char* begin, char* end) { this->setg(begin, begin, end); }
	};
	membuf buf((char*)data, (char*)data + size);
	istream is(&buf);
	ostringstream os;
	zmolly_encode(is, os);
	auto str = os.str();
	compressed.resize(str.size());
	copy(str.begin(), str.end(), compressed.begin());
}
#endif


#if defined(ENABLE_ZPAQ)
#	include <libzpaq.h>

void libzpaq::error(const char* msg)
{
	throw runtime_error(msg);
}

class zpaqin : public libzpaq::Reader
{
public:
	zpaqin(char* data, size_t size) : pos(0), size(size), data(data) {}

	int get() override  // should return 0..255, or -1 at EOF
	{
		if (pos >= size)
			return -1;
		return data[pos++];
	}
	int read(char* buf, int n) override
	{
		auto remaining = size - pos;
		auto actual = min((size_t)n, remaining);
		copy_n(data + pos, actual, buf);
		pos += actual;
		return actual;
	}

private:
	size_t pos;
	size_t size;
	char* data;
};

class zpaqout : public libzpaq::Writer
{
public:
	zpaqout(function<uint8_t*()> outputBuffer, function<void(size_t)> outputResize)
		: outputBuffer(outputBuffer),
		  outputResize(outputResize)
	{}

	zpaqout(vector<uint8_t>& buffer)
		: outputBuffer([&]() { return (uint8_t*)buffer.data(); }),
		  outputResize([&](size_t size) { buffer.resize(size); })
	{}

	void put(int c) override
	{
		// should output low 8 bits of c
		// buffer.push_back((uint8_t)c);
		outputResize(++pos);
		outputBuffer()[pos] = (uint8_t)c;
	}
	void write(const char* buf, int n) override
	{
		outputResize(pos += n);
		copy_n(buf, n, outputBuffer() + pos);
	}

private:
	function<uint8_t*()> outputBuffer;
	function<void(size_t)> outputResize;
	size_t pos = 0;
};

inline void
	compressZPAQ(vector<uint8_t>& compressed, const uint8_t* data, size_t size, int level)
{
	zpaqin zin((char*)data, size);
	zpaqout zout(compressed);
	auto method = to_string(min(5, max(0, level)));
	libzpaq::compress(&zin, &zout, method.c_str());
}

inline void decompressZPAQ(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize)
{
	zpaqin zin((char*)inputBuffer, inputSize);
	zpaqout zout(outputBuffer, outputResize);
	libzpaq::decompress(&zin, &zout);
}
#endif


template <>
vector<uint8_t> compress(const uint8_t* data, size_t size, Compression method, int level)
{
	vector<uint8_t> compressed;

	switch (method)
	{
		case Compression::Auto:
			// pick the first enabled method below

#if defined(ENABLE_ZSTD)
		case Compression::Zstd:
			compressZstd(compressed, data, size, level);
			break;
#endif

#if defined(ENABLE_LIBDEFLATE)
		case Compression::Deflate:
		case Compression::zlib:
		case Compression::gzip:
			compressDeflate(compressed, data, size, method, level);
			break;
#endif

#if defined(ENABLE_BROTLI)
		case Compression::Brotli:
			compressBrotli(compressed, data, size, level);
			break;
#endif

#if defined(ENABLE_LZMA2)
		case Compression::LZMA2:
			compressLZMA2(compressed, data, size, level);
			break;
#endif

#if defined(ENABLE_SNAPPY)
		case Compression::Snappy:
			compressSnappy(compressed, data, size);
			break;
#endif

#if defined(ENABLE_ZMOLLY)
		case Compression::zmolly:
			compressZmolly(compressed, data, size);
			break;
#endif

#if defined(ENABLE_ZPAQ)
		case Compression::ZPAQ:
			compressZPAQ(compressed, data, size, level);
			break;
#endif

		default:
			throw invalid_argument("compression method not supported");
	}
	return compressed;
}


void decompress(
	const uint8_t* data,
	const size_t size,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize,
	Compression method /*= Compression::Auto*/)
{
	if (method == Compression::Auto)
		method = detectCompression(data, size);

	switch (method)
	{
#if defined(ENABLE_ZSTD)
		case Compression::Zstd:
			return decompressZstd(data, size, outputBuffer, outputResize);
#endif

#if defined(ENABLE_LIBDEFLATE)
		case Compression::Deflate:
		case Compression::zlib:
		case Compression::gzip:
			return decompressDeflate(data, size, outputBuffer, outputResize);
#endif

#if defined(ENABLE_BROTLI)
		case Compression::Brotli:
			return decompressBrotli(data, size, outputBuffer, outputResize);
#endif

#if defined(ENABLE_LZMA2)
		case Compression::LZMA2:
			return decompressLZMA2(data, size, outputBuffer, outputResize);
#endif

#if defined(ENABLE_SNAPPY)
		case Compression::Snappy:
			return decompressSnappy(data, size, outputBuffer, outputResize);
#endif

#if defined(ENABLE_ZPAQ)
		case Compression::ZPAQ:
			return decompressZPAQ(data, size, outputBuffer, outputResize);
#endif

		default:
			throw invalid_argument("compression method not supported");
	}
}
