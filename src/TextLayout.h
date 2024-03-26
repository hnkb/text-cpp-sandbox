#pragma once

#include "vectors.h"
#include <vector>
#include <filesystem>
#include <string>


struct GlyphInfo
{
	uint32_t index;
	float2 pos;

	GlyphInfo(uint32_t index, float2 pos) : index(index), pos(pos) {}
};

std::vector<GlyphInfo>
	shapeWithHarfbuzz(const std::string& text, const std::filesystem::path& fontFilename);
