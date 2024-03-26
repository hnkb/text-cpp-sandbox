#include "TextLayout.h"
#include <hb.h>
#include <sstream>

using namespace std;

vector<GlyphInfo> shapeWithHarfbuzz(const string& text, const filesystem::path& fontFilename)
{
	auto blob = hb_blob_create_from_file(fontFilename.c_str());
	auto face = hb_face_create(blob, 0);
	auto font = hb_font_create(face);

	// if not set, we'll use design units
	int pixel_size = 32;  // The desired size of the font in pixels
	hb_font_set_scale(font, pixel_size << 6, pixel_size << 6);

	hb_font_extents_t extents;
	float line_height = pixel_size * 1.25f;
	if (hb_font_get_h_extents(font, &extents))
		line_height = (extents.ascender - extents.descender + extents.line_gap) / 64.f;

	auto buffer = hb_buffer_create();

	float2 cursor = { 0, 0 };


	vector<GlyphInfo> output;
	output.reserve(text.size());


	istringstream stream(text);
	string line;
	while (getline(stream, line))
	{
		hb_buffer_reset(buffer);
		hb_buffer_add_utf8(buffer, line.c_str(), -1, 0, -1);
		hb_buffer_guess_segment_properties(buffer);

		hb_shape(font, buffer, NULL, 0);

		unsigned int glyph_count;
		auto glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_count);
		auto glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_count);

		for (unsigned int i = 0; i < glyph_count; i++)
		{
			auto pos =
				cursor + float2 { glyph_pos[i].x_offset / 64.f, glyph_pos[i].y_offset / 64.f };
			cursor.x += glyph_pos[i].x_advance / 64.f;
			cursor.y += glyph_pos[i].y_advance / 64.f;
			output.emplace_back(glyph_info[i].codepoint, pos);
		}

		cursor.x = 0;
		cursor.y += line_height;
	}

	hb_buffer_destroy(buffer);
	hb_font_destroy(font);
	hb_face_destroy(face);
	hb_blob_destroy(blob);

	return output;
}
