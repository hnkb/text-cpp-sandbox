#include "TextLayout.h"
#include <iostream>

#include <ttf2mesh.h>
#include <hb.h>
#include <fstream>

#include <iostream>
#include <ft2build.h>
#include <freetype/freetype.h>
#include FT_FREETYPE_H
#include "tesselator.h"  // Include the libtess2 header

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

int saveFontUsingFreeTypeAndLibTess(const filesystem::path& filename) {
    FT_Library library;    // Declare a FreeType library object
    FT_Face face;          // Declare a FreeType face object

    // Initialize the FreeType library
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        std::cerr << "Failed to initialize FreeType library" << std::endl;
        return 1;
    }

    // Load a font face from a font file
    error = FT_New_Face(library, filename.c_str(), 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        std::cerr << "The font file could be opened and read, but it is in an unsupported format" << std::endl;
        return 1;
    } else if (error) {
        std::cerr << "Failed to load the font file" << std::endl;
        return 1;
    }

	FT_Set_Pixel_Sizes(face, 0, 48);

    // Iterate over all glyphs
    FT_ULong charcode;
    FT_UInt gindex;
    charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0) {
        // Load the glyph by its glyph index
        error = FT_Load_Glyph(face, gindex, FT_LOAD_DEFAULT);
        if (error) {
            std::cerr << "Could not load glyph\n";
            continue;  // Continue with next glyph on error
        }

        // Here you can access the glyph's data, for example, face->glyph->format
        // If you want to access the outline: face->glyph->outline

		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
			// Access the outline
			FT_Outline* outline = &face->glyph->outline;

			// Tessellation using libtess2
			TESSalloc ma;
			ma.memalloc = malloc;
			ma.memfree = free;
			ma.extraVertices = 256;  // Allocate extra vertices for intersection, etc.

			TESStesselator* tess = tessNewTess(&ma);
			if (!tess) {
				std::cerr << "Could not initialize tesselator" << std::endl;
				return 1;
			}

			std::vector<TESSreal> vertices;
			std::vector<int> indices;

			// Loop over contours
			for (int i = 0; i < outline->n_contours; i++) {
				int start = (i == 0) ? 0 : outline->contours[i - 1] + 1;
				int end = outline->contours[i];
				
				std::vector<TESSreal> contour;
				for (int j = start; j <= end; j++) {
					contour.push_back(outline->points[j].x);
					contour.push_back(outline->points[j].y);
				}
				tessAddContour(tess, 2, &contour[0], sizeof(TESSreal) * 2, (end - start + 1));
			}

			// Tesselate
			tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, 0);
			const TESSreal* verts = tessGetVertices(tess);
			const TESSindex* elems = tessGetElements(tess);
			int nelems = tessGetElementCount(tess);

			// Use tesselated data
			// Here, you would process the tesselated output, e.g., rendering or further geometric operations
		}

        std::cout << "Processed glyph for charcode: " << charcode << std::endl;

        // Get the next character code and glyph index
        charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    std::cout << "Loaded font: " << face->family_name << ", " << face->style_name << std::endl;

    // Cleanup
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}

int main()
{
	saveFontUsingFreeTypeAndLibTess("/Users/georgepomaskin/Projects/render-sandbox/client/cpp/assets/fonts/MPLUS1p-Regular.ttf");
	return 0;

	// saveFont("/Users/georgepomaskin/Projects/render-sandbox/client/cpp/assets/fonts/MPLUS1p-Regular.ttf");
	// return 0;

	// const auto text = "grape of\nmother";
	const auto text = "اگر آن ترک شیرازی دستش را به دل ما برد";

	const filesystem::path fontFF =
		"/Users/georgepomaskin/Projects/render-sandbox/client/cpp/assets/fonts/"
		"MPLUS1p-Regular.ttf";

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
