#pragma once

#include "../utils/Math.h"
#include <vector>
#include <filesystem>
#include <string>


struct ShapedGlyph
{
	uint32_t index;
	float2 pos;
	ShapedGlyph(uint32_t index, float2 pos) : index(index), pos(pos) {}
};

std::vector<ShapedGlyph>
	shapeWithHarfbuzz(const std::string& text, const std::filesystem::path& fontFilename);
