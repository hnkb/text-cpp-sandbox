
#include "File.h"
#include "Compression.h"
#include <regex>
#include <sstream>

using namespace std;

#include <iostream>


#if defined(__GNUC__) || defined(__clang__)

#	include <cxxabi.h>
#	include <memory>

string demangle(const char* name)
{
	int status;
	unique_ptr<char, void (*)(void*)>
		res { abi::__cxa_demangle(name, nullptr, nullptr, &status), free };
	return (status == 0) ? res.get() : name;
}

#elif defined(_MSC_VER)

string demangle(const char* name)
{
	regex structRegex(R"(^(struct |class |enum ))");
	return regex_replace(name, structRegex, "");
}

#endif


File::Pack::Pack(const filesystem::path& filename, const char mode, const char signature[6])
	: filename(filename),
	  mode(mode)
{
	// modes to support: r, w, a, x
	// r: read, w: write, a: append, x: write but fail if exists

	if (mode == 'x' && filesystem::exists(filename))
		throw runtime_error("File exists");

	file = make_unique<File>(filename, string(1, mode));
	if (!*file)
		throw runtime_error("Unable to open file");

	if (mode == 'r')  // or if a is  || mode == 'a')
	{
		// TODO: append to empty file should be valid
		const auto size = file->size();

		if (size < sizeof(header))
			throw runtime_error("Invalid file size");
		fread(&header, sizeof(header), 1, *file);

		if (memcmp(header.signature, signature, 6) != 0)
			throw runtime_error("Invalid signature");

		vector<char> descriptors;
		fseek(*file, header.descriptorOffset, SEEK_SET);

		if (header.version == 0)
		{
			descriptors.resize(size - header.descriptorOffset);
			fread(descriptors.data(), 1, descriptors.size(), *file);
		}
		else if (header.version == 1)
		{
			vector<uint8_t> compressed(size - header.descriptorOffset);
			fread(compressed.data(), 1, compressed.size(), *file);
			descriptors = decompress<char>(compressed);
		}
		else
			throw runtime_error("Invalid version");

		auto offset = sizeof(header);

		regex pattern("([^;]*);([^;]*);(\\d+);([^\\n]*)\\s*");
		for (sregex_token_iterator
				 i(descriptors.begin(), descriptors.end(), pattern, { 1, 2, 3, 4 });
			 i != sregex_token_iterator();
			 i++)
		{
			auto& block = blocks.emplace_back();
			block.typeinfo = i->str();
			block.compression = (++i)->str();
			block.offset = offset;
			block.compressedSize = (size_t)stoull((++i)->str());
			block.name = (++i)->str();

			if (find(block.name) != &block)
				throw runtime_error("Block already exists");

			offset += block.compressedSize;
		}
	}

	if (mode == 'w' || mode == 'x')  // mode=='a' and file is empty
	{
		copy_n(signature, 6, header.signature);
		fwrite(&header, sizeof(header), 1, *file);
		currentWritePosition = sizeof(header);
	}
}


File::Pack::Block* File::Pack::find(const string& name)
{
	for (auto& block: blocks)
		if (block.name == name)
			return &block;

	return nullptr;
}

void File::Pack::get(
	const string& name,
	function<uint8_t*()> outputBuffer,
	function<void(size_t)> outputResize)
{
	auto block = find(name);
	if (!block)
		throw runtime_error("Block not found");

	if (block->compression.empty())
	{
		outputResize(block->compressedSize);
		fseek(*file, block->offset, SEEK_SET);
		fread(outputBuffer(), block->compressedSize, 1, *file);
	}
	else
	{
		vector<uint8_t> buffer(block->compressedSize);
		fseek(*file, block->offset, SEEK_SET);
		fread(buffer.data(), buffer.size(), 1, *file);
		decompress(buffer.data(), buffer.size(), outputBuffer, outputResize);
	}
}


template <>
void File::Pack::add(
	const string& name,
	const uint8_t* data,
	size_t size,
	int compression,
	const char* typeinfo)
{
	if (find(name) != nullptr)
		throw runtime_error("Block already exists");

	auto& block = blocks.emplace_back();
	block.name = name;
	block.typeinfo = demangle(typeinfo);
	block.offset = currentWritePosition;

	fseek(*file, block.offset, SEEK_SET);

	if (compression)
	{
		auto compressed = compress(data, size, Compression::Brotli, compression);
		block.compression = "brotli";
		block.compressedSize = compressed.size();
		fwrite(compressed.data(), compressed.size(), 1, *file);
	}
	else
	{
		block.compressedSize = size;
		fwrite(data, size, 1, *file);
	}

	currentWritePosition += block.compressedSize;
	dirty = true;
}


void File::Pack::flush()
{
	if (dirty)
	{
		stringstream descriptors;

		for (auto& block: blocks)
			descriptors
				<< block.typeinfo << ";" << block.compression << ";" << block.compressedSize
				<< ";" << block.name << "\n";

		fseek(*file, currentWritePosition, SEEK_SET);
		if (header.version == 0)
		{
			const auto buffer = descriptors.str();
			fwrite(buffer.c_str(), buffer.size() - 1, 1, *file);  // -1 to exclude the ending
																  // '\n'
		}
		else
		{
			const auto buffer = compress(descriptors.str());
			fwrite(buffer.data(), buffer.size(), 1, *file);
		}

		header.descriptorOffset = currentWritePosition;
		fseek(*file, 0, SEEK_SET);
		fwrite(&header, sizeof(header), 1, *file);

		dirty = false;
	}
}
