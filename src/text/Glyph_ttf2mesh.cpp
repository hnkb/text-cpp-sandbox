#include "Font.h"
#include "../utils/File.h"
#include <ttf2mesh.h>

using namespace std;


void saveFont_ttf2mesh(const filesystem::path& filename)
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
			|| ttf_glyph2mesh(inputGlyph, &mesh, TTF_QUALITY_HIGH, TTF_FEATURE_IGN_ERR)
				   != TTF_DONE)
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

	for (auto& v: vertices)
		v.y = 1 - v.y;

	auto outFile = filename;
	outFile.replace_extension(".vert");
	File::writeAll(vertices, outFile);

	outFile.replace_extension(".idx");
	File::writeAll(indices, outFile);

	outFile.replace_extension(".mesh");
	File::writeAll(meshes, outFile);

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
