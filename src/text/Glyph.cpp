#include "Font.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include FT_FREETYPE_H
#include "tesselator.h"  // Include the libtess2 header

#include <iostream>

using namespace std;

int saveFontUsingFreeTypeAndLibTess(const filesystem::path& filename)
{
	FT_Library library;  // Declare a FreeType library object
	FT_Face face;        // Declare a FreeType face object

	// Initialize the FreeType library
	FT_Error error = FT_Init_FreeType(&library);
	if (error)
	{
		std::cerr << "Failed to initialize FreeType library" << std::endl;
		return 1;
	}

	// Load a font face from a font file
	error = FT_New_Face(library, filename.c_str(), 0, &face);
	if (error == FT_Err_Unknown_File_Format)
	{
		std::cerr
			<< "The font file could be opened and read, but it is in an unsupported format"
			<< std::endl;
		return 1;
	}
	else if (error)
	{
		std::cerr << "Failed to load the font file" << std::endl;
		return 1;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	// Iterate over all glyphs
	FT_ULong charcode;
	FT_UInt gindex;
	charcode = FT_Get_First_Char(face, &gindex);
	while (gindex != 0)
	{
		// Load the glyph by its glyph index
		error = FT_Load_Glyph(face, gindex, FT_LOAD_DEFAULT);
		if (error)
		{
			std::cerr << "Could not load glyph\n";
			continue;  // Continue with next glyph on error
		}

		// Here you can access the glyph's data, for example, face->glyph->format
		// If you want to access the outline: face->glyph->outline

		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			// Access the outline
			FT_Outline* outline = &face->glyph->outline;

			// Tessellation using libtess2
			TESSalloc ma;
			ma.memalloc = malloc;
			ma.memfree = free;
			ma.extraVertices = 256;  // Allocate extra vertices for intersection, etc.

			TESStesselator* tess = tessNewTess(&ma);
			if (!tess)
			{
				std::cerr << "Could not initialize tesselator" << std::endl;
				return 1;
			}

			std::vector<TESSreal> vertices;
			std::vector<int> indices;

			// Loop over contours
			for (int i = 0; i < outline->n_contours; i++)
			{
				int start = (i == 0) ? 0 : outline->contours[i - 1] + 1;
				int end = outline->contours[i];

				std::vector<TESSreal> contour;
				for (int j = start; j <= end; j++)
				{
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
			// Here, you would process the tesselated output, e.g., rendering or further
			// geometric operations
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
