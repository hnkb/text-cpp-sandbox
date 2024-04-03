#include "TextLayout.h"
#include <iostream>

#include <ttf2mesh.h>
#include <hb.h>
using namespace std;

extern hb_font_extents_t extents;

int main()
{
	// const auto text = "grape of\nmother";
	const auto text = "اگر آن ترک شیرازی دستش را به دل ما برد";

	const filesystem::path fontFF =
		"/Users/hani/dev/hani/render-sandbox/client/cpp/assets/fonts/"
		"NotoSansArabic-Regular.ttf";

	auto glyphs = shapeWithHarfbuzz(text, fontFF);
	// for (const auto& glyph: glyphs)
	// {
	// 	cout << "Glyph ID: " << glyph.index << ", ";
	// 	cout << "Pos: " << glyph.pos.x << ", " << glyph.pos.y << endl;
	// }


	ttf_t* font = nullptr;
	ttf_load_from_file(fontFF.u8string().c_str(), &font, false);
	if (!font)
		throw std::runtime_error("Unable to load font");
	// // temporary buffers to store CPU-side data

	// std::vector<float2> vert;
	// std::vector<GLushort> index;
	// std::map<wchar_t, void*> vertOffsets;

	// vert.reserve(font->nglyphs * 125);
	// index.reserve(font->nglyphs * 125);

	// cout << font->hhea.ascender << endl;
	// cout << extents.ascender << endl;
	// cout << font->hhea.ascender / extents.ascender << endl;
	const auto scale = font->hhea.ascender / extents.ascender;
	// const auto lineHeight = (extents.ascender - extents.descender + extents.line_gap) / 64.f;
	// const auto lineHeight2 =
	// 	(font->hhea.ascender - font->hhea.descender + font->hhea.lineGap) / scale / 64.f;
	// cout << lineHeight << " == " << lineHeight2 << endl;

	// int numErrors = 0;
	for (const auto& glyphInfo: glyphs)
	// for (int glyphIdx = 0; glyphIdx < font->nglyphs; glyphIdx++)
	{
		try
		{
			auto glyphIdx = glyphInfo.index;

			auto glyph = &font->glyphs[glyphIdx];

			ttf_mesh_t* mesh = nullptr;
			if (glyphIdx==3 || glyphIdx==972)
				printf("Glyph %d\n", glyphIdx);

			const auto err = ttf_glyph2mesh(glyph, &mesh, TTF_QUALITY_HIGH, TTF_FEATURES_DFLT);
			if (err != TTF_DONE)
			{
				// throw std::runtime_error("unable to load glyph mesh");
				printf("Failed to create mesh for %d error %d\n", glyphIdx, err);
			}

			// wprintf(
			// 	L" '%c' advance: %f, scaled: %f, harfbuzz: %f\n",
			// 	glyph->symbol,
			// 	glyph->advance,
			// 	glyph->advance / scale,
			// 	glyphInfo.advance);


			// auto& obj = objects[g.glyph->symbol];

			// obj.advance = g.glyph->advance;
			// obj.lbearing = g.glyph->lbearing;
			// obj.start = (void*)(index.size() * sizeof(GLushort));
			// vertOffsets[g.glyph->symbol] = (void*)(vert.size() * sizeof(vert[0]));

			// for (int i = 0; i < g.mesh->nvert; i++)
			// 	vert.push_back(float2 { g.mesh->vert[i].x, g.mesh->vert[i].y });

			// for (int i = 0; i < g.mesh->nfaces; i++)
			// {
			// 	index.push_back(g.mesh->faces[i].v1);
			// 	index.push_back(g.mesh->faces[i].v2);
			// 	index.push_back(g.mesh->faces[i].v3);
			// }

			// obj.numPoints = g.mesh->nfaces * 3;
		}
		catch (...)
		{
			// numErrors++;
		}
	}

	// printf("Encountered %d errors while loading\n", numErrors);
	// printf("\n\n In total\n   %d vertices\n   %d indices\n", vert.size(), index.size());

	if (font)
		ttf_free(font);

	return 0;
}
