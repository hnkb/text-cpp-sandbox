#pragma once

#include <filesystem>
#include <functional>
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



	class Pack
	{
	public:
		Pack(const std::filesystem::path& filename, const char mode, const char signature[6]);
		~Pack() { flush(); }

		const char mode;
		const std::filesystem::path filename;


		template <typename Type>
		void add(
			const std::string& name,
			const Type* data,
			size_t size,
			int compression = 5,
			const char* typeinfo = nullptr)
		{
			add(name, (uint8_t*)data, size * sizeof(Type), compression, typeid(Type).name());
		}

		template <class Type, size_t size>
		void add(const std::string& name, const Type (&array)[size], int compression = 5)
		{
			add(name, array, size, compression);
		}

		// template <class Type, template <class, class...> class Container, class... Rest>
		// void add(const std::string& name, const Container<Type, Rest...>& container)
		template <class Container>
		void add(const std::string& name, const Container& container, int compression = 5)
		{
			add(name, container.data(), container.size(), compression);
		}


		template <class Type>
		std::vector<Type> get(const std::string& name)
		{
			std::vector<Type> outputBuffer;
			get(
				name,
				[&]() { return (uint8_t*)outputBuffer.data(); },                // data
				[&](size_t size) { outputBuffer.resize(size / sizeof(Type)); }  // resize
			);
			return outputBuffer;
		}

	private:
		struct Block
		{
			std::string name;
			std::string typeinfo;
			std::string compression;
			size_t offset;
			size_t compressedSize;
		};

		void flush();

		Block* find(const std::string& name);
		void get(
			const std::string& name,
			std::function<uint8_t*()> outputBuffer,
			std::function<void(size_t)> outputResize);


		std::vector<Block> blocks;

		std::unique_ptr<File> file;
		size_t currentWritePosition = 0;
		bool dirty = false;

		struct
		{
			char signature[6];
			uint16_t version = 1;
			uint64_t descriptorOffset = 0;
		} header;
	};
};


template <>
void File::Pack::add(const std::string&, const uint8_t*, size_t, int, const char*);
