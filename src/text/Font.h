#pragma once

#include "../graphics/Mesh.h"
#include <vector>
#include <filesystem>
#include <string>


struct GlyphInfo
{
	uint32_t index;
	float2 pos;
	float advance;

	GlyphInfo(uint32_t index, float2 pos) : index(index), pos(pos) {}
};

std::vector<GlyphInfo>
	shapeWithHarfbuzz(const std::string& text, const std::filesystem::path& fontFilename);

int saveFontUsingFreeTypeAndLibTess(const std::filesystem::path& filename);

std::string* readWOFF2(const std::filesystem::path& filename);
