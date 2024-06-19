#pragma once

#include <functional>
#include <vector>
#include <stdint.h>


enum class Compression {
	Auto,
	// Zstd,
	// Deflate,
	// zlib,
	// gzip,
	Brotli,
	// LZMA2,
	// Snappy,
	// zmolly,
	// ZPAQ,
};

template <class Type>
std::vector<uint8_t> compress(
	const Type* data,
	size_t size,
	Compression method = Compression::Auto,
	int level = 3)
{
	return compress(reinterpret_cast<uint8_t*>(data), sizeof(Type) * size, method, level);
}

template <>
std::vector<uint8_t> compress(const uint8_t* data, size_t size, Compression method, int level);

template <class Type, template <class, class...> class Container, class... Rest>
inline std::vector<uint8_t> compress(
	const Container<Type, Rest...>& container,
	const Compression method = Compression::Auto,
	const int level = 3)
{
	return compress((uint8_t*)container.data(), container.size() * sizeof(Type), method, level);
}



void decompress(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	std::function<uint8_t*()> outputBuffer,
	std::function<void(size_t)> outputResize,
	Compression method = Compression::Auto);

template <class OutputType = uint8_t, template <class, class...> class Container, class... Rest>
std::vector<OutputType> decompress(
	const Container<uint8_t, Rest...>& compressedBuffer,
	Compression method = Compression::Auto)
{
	std::vector<OutputType> outputBuffer;
	decompress(
		compressedBuffer.data(),
		compressedBuffer.size(),
		[&]() { return (uint8_t*)outputBuffer.data(); },                       // data
		[&](size_t size) { outputBuffer.resize(size / sizeof(OutputType)); },  // resize
		method);
	return outputBuffer;
}

template <class OutputType = uint8_t>
std::vector<OutputType> decompress(
	const uint8_t* inputBuffer,
	const size_t inputSize,
	Compression method = Compression::Auto)
{
	std::vector<OutputType> outputBuffer;
	decompress(
		inputBuffer,
		inputSize,
		[&]() { return (uint8_t*)outputBuffer.data(); },                       // data
		[&](size_t size) { outputBuffer.resize(size / sizeof(OutputType)); },  // resize
		method);
	return outputBuffer;
}
