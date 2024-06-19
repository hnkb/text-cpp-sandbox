#pragma once

#include <filesystem>
#include <string>
#include <vector>


class File
{
public:
	File(const std::filesystem::path& filename, const std::string& mode)
	{
		// make sure path exists so we can actually create file
		if (mode.find('w') != std::string::npos)
			std::filesystem::create_directories(filename.parent_path());
		handle = fopen(filename.c_str(), mode.c_str());
	}
	~File() { fclose(handle); }

	size_t size()
	{
		auto current = ftell(handle);
		fseek(handle, 0, SEEK_END);
		auto length = ftell(handle);
		fseek(handle, current, SEEK_SET);
		return length;
	}

	operator FILE*() { return handle; }

private:
	FILE* handle = nullptr;



public:
	template <class Type = uint8_t>
	static std::vector<Type> readAll(const std::filesystem::path& filename)
	{
		File file(filename, "rb");
		std::vector<Type> buffer(file.size() / sizeof(Type));
		fread(buffer.data(), sizeof(Type), buffer.size(), file);
		return buffer;
	}

	template <class Type>
	static void writeAll(const Type* data, size_t size, const std::filesystem::path& filename)
	{
		std::filesystem::create_directories(filename.parent_path());
		File file(filename, "wb");
		fwrite(data, sizeof(Type), size, file);
	}

	template <class Type, size_t size>
	static void writeAll(const Type (&array)[size], const std::filesystem::path& filename)
	{
		return writeAll(array, size, filename);
	}

	template <class Type>
	static void writeAll(const Type& data, const std::filesystem::path& filename)
	{
		return writeAll(data.data(), data.size(), filename);
	}
};
