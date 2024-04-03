#include "TextLayout.h"
#include <iostream>

#include <ttf2mesh.h>
#include <hb.h>
#include <fstream>
using namespace std;

extern hb_font_extents_t extents;


struct Mesh
{
	int startIndex;
	int indexCount;
};

template <typename T>
void writeToFile(const filesystem::path& filename, const vector<T>& data)
{
	std::ofstream file(filename, std::ios::binary);
	file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));
}

void saveFont(const filesystem::path& filename)
{
	ttf_t* ttf = nullptr;
	ttf_load_from_file(filename.u8string().c_str(), &ttf, false);
	if (!ttf)
	{
		fprintf(stderr, "Error opening font from '%s'\n", filename.u8string().c_str());
		return;
	}

	// name = ttf->names.full_name;
	// family = ttf->names.family;

	// metrics.lineHeight = (ttf->hhea.ascender - ttf->hhea.descender + ttf->hhea.lineGap);

	std::vector<Mesh> meshes;
	std::vector<float2> vertices;
	std::vector<uint32_t> indices;
	meshes.resize(ttf->nglyphs);
	vertices.reserve(ttf->nglyphs * 125);
	indices.reserve(ttf->nglyphs * 125 * 3);

	int numErrors = 0;

	for (int glyphIdx = 0; glyphIdx < ttf->nglyphs; glyphIdx++)
	{
		auto inputGlyph = &ttf->glyphs[glyphIdx];
		auto& outputGlyph = meshes[glyphIdx];

		ttf_mesh_t* mesh = nullptr;
		if (inputGlyph->symbol == ' '
			|| ttf_glyph2mesh(inputGlyph, &mesh, TTF_QUALITY_HIGH, TTF_FEATURE_IGN_ERR) != TTF_DONE)
		{
			outputGlyph.startIndex = 0;
			outputGlyph.indexCount = 0;
			numErrors++;
			continue;
		}

		const auto startVertex = (uint32_t)vertices.size();
		outputGlyph.startIndex = (int)indices.size();
		outputGlyph.indexCount = mesh->nfaces * 3;

		for (int i = 0; i < mesh->nvert; i++)
			vertices.emplace_back(mesh->vert[i].x, mesh->vert[i].y);

		for (int i = 0; i < mesh->nfaces; i++)
		{
			indices.push_back(mesh->faces[i].v1 + startVertex);
			indices.push_back(mesh->faces[i].v2 + startVertex);
			indices.push_back(mesh->faces[i].v3 + startVertex);
		}
	}

	ttf_free(ttf);

	auto outFile = filename;
	outFile.replace_extension(".vert");
	writeToFile(outFile, vertices);

	outFile.replace_extension(".idx");
	writeToFile(outFile, indices);

	outFile.replace_extension(".mesh");
	writeToFile(outFile, meshes);

	printf(
		"Font '%s' loaded into %.2f MB buffer with %zu glyphs, %d errors, %zu vertices, %zu "
		"indices.\n",
		filename.stem().c_str(),
		(vertices.size() * sizeof(vertices[0]) + indices.size() * sizeof(indices[0])) / 1024.
			/ 1024.,
		meshes.size(),
		numErrors,
		vertices.size(),
		indices.size());
}


int main()
{
	saveFont(
		"/Users/hani/dev/hani/render-sandbox/client/cpp/assets/fonts/MPLUS1p-Regular.ttf");
	return 0;

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
