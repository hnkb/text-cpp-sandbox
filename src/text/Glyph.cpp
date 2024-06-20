#include "Font.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include <iostream>

using namespace std;


struct Quadratic
{
	float2 p0, p1, p2;
	Quadratic() {}
	Quadratic(float2 p0, float2 p1, float2 p2) : p0(p0), p1(p1), p2(p2) {}
};

class Path
{
public:
	float2 currentPos = { 0, 0 };
	vector<vector<Quadratic>> contours;
};

float scale = 1.f;

// Move to callback
int move_to(const FT_Vector* to, void* user)
{
	// printf("Move to (%ld, %ld)\n", to->x, to->y);
	auto bezier = (Path*)user;
	bezier->currentPos = float2(to->x, to->y) / scale;
	bezier->contours.emplace_back();
	return 0;
}

// Line to callback
int line_to(const FT_Vector* to, void* user)
{
	Path* bezier = (Path*)user;
	// printf("Line to (%ld, %ld)\n", to->x, to->y);
	auto& p0 = bezier->currentPos;
	float2 p1 = float2(to->x, to->y) / scale;
	bezier->contours.back().emplace_back(p0, (p0 + p1) / 2.f, p1);
	bezier->currentPos = p1;
	return 0;
}

// Conic to callback
int conic_to(const FT_Vector* control, const FT_Vector* to, void* user)
{
	Path* bezier = (Path*)user;
	// printf(
	// 	"Conic to control (%ld, %ld) end (%ld, %ld)\n",
	// 	control->x,
	// 	control->y,
	// 	to->x,
	// 	to->y);
	auto& p0 = bezier->currentPos;
	float2 p1 = float2(control->x, control->y) / scale;
	float2 p2 = float2(to->x, to->y) / scale;
	bezier->contours.back().emplace_back(p0, p1, p2);
	bezier->currentPos = p2;
	return 0;
}

// Cubic to callback
int cubic_to(
	const FT_Vector* control1,
	const FT_Vector* control2,
	const FT_Vector* to,
	void* user)
{
	// Path* bezier = (Path*)user;
	// printf(
	// 	"Cubic to control1 (%ld, %ld) control2 (%ld, %ld) end (%ld, %ld)\n",
	// 	control1->x,
	// 	control1->y,
	// 	control2->x,
	// 	control2->y,
	// 	to->x,
	// 	to->y);
	// printf("====+> not supproted\n");
	// add_point(bezier, *control1);
	// add_point(bezier, *control2);
	// add_point(bezier, *to);
	return 0;
}



int saveFontUsingFreeTypeAndLibTess(const filesystem::path& filename)
{
	FT_Library library;  // Declare a FreeType library object
	FT_Face face;        // Declare a FreeType face object

	// Initialize the FreeType library
	FT_Error error = FT_Init_FreeType(&library);
	if (error)
	{
		cerr << "Failed to initialize FreeType library" << endl;
		return 1;
	}

	string* buffer = nullptr;
	if (filename.extension() == ".woff2")
	{
		buffer = readWOFF2(filename);
		error = FT_New_Memory_Face(library, (FT_Byte*)buffer->data(), buffer->size(), 0, &face);
	}
	else
		error = FT_New_Face(library, filename.c_str(), 0, &face);

	if (error == FT_Err_Unknown_File_Format)
	{
		cerr << "The font file could be opened and read, but it is in an unsupported format"
			 << endl;
		return 1;
	}
	else if (error)
	{
		cerr << "Failed to load the font file" << endl;
		return 1;
	}


#if 0
	for (FT_UInt gindex = 0; gindex < face->num_glyphs; gindex++)
	{
		// Load the glyph by its glyph index
		error = FT_Load_Glyph(face, gindex, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING);
		if (!error && face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			FT_GlyphSlot slot = face->glyph;
			FT_Outline* outline = &slot->outline;

			currentContour.clear();

			FT_Outline_Funcs funcs;
			funcs.move_to = moveTo;
			funcs.line_to = lineTo;
			funcs.conic_to = conicTo;
			funcs.cubic_to = cubicTo;
			funcs.shift = 0;
			funcs.delta = 0;

			if (FT_Outline_Decompose(outline, &funcs, nullptr))
				cerr << "Error decomposing outline." << endl;

			contours.push_back(ContourWithIndex({ gindex, currentContour }));
		}
		else
		{
			cerr << "Could not load glyph" << endl;
		}
	}
#endif


	rapidjson::Document document;
	auto& allocator = document.GetAllocator();
	document.SetArray();

	cout << "Loaded font: " << face->family_name << ", " << face->style_name << endl;

	scale = face->units_per_EM;


	for (FT_UInt gindex = 0; gindex < face->num_glyphs; gindex++)
	{
		// Load the glyph by its glyph index
		error = FT_Load_Glyph(face, gindex, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING);
		if (!error && face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			FT_GlyphSlot slot = face->glyph;
			FT_Outline outline = slot->outline;

			// Print some information about the glyph
			// printf("Number of contours: %d\n", outline.n_contours);
			// printf("Number of points: %d\n", outline.n_points);

			FT_Outline_Funcs funcs;
			funcs.move_to = move_to;
			funcs.line_to = line_to;
			funcs.conic_to = conic_to;
			funcs.cubic_to = cubic_to;
			funcs.shift = 0;
			funcs.delta = 0;

			Path path;

			error = FT_Outline_Decompose(&outline, &funcs, &path);
			if (error)
			{
				printf("Could not decompose outline\n");
				continue;
			}

			rapidjson::Value jcontours(rapidjson::kArrayType);

			// Print the quadratics
			for (const auto& contour: path.contours)
			{
				rapidjson::Value quads(rapidjson::kArrayType);

				// printf("Contour:\n");
				for (const auto& quad: contour)
				{
					rapidjson::Value jquad(rapidjson::kObjectType);
					jquad.AddMember(
						"p0",
						rapidjson::Value()
							.SetArray()
							.PushBack(quad.p0.x, allocator)
							.PushBack(quad.p0.y, allocator),
						allocator);
					jquad.AddMember(
						"p1",
						rapidjson::Value()
							.SetArray()
							.PushBack(quad.p1.x, allocator)
							.PushBack(quad.p1.y, allocator),
						allocator);
					jquad.AddMember(
						"p2",
						rapidjson::Value()
							.SetArray()
							.PushBack(quad.p2.x, allocator)
							.PushBack(quad.p2.y, allocator),
						allocator);
					quads.PushBack(jquad, allocator);

					// printf(
					// 	"  Quadratic: (%f, %f) (%f, %f) (%f, %f)\n",
					// 	quad.p0.x,
					// 	quad.p0.y,
					// 	quad.p1.x,
					// 	quad.p1.y,
					// 	quad.p2.x,
					// 	quad.p2.y);
				}
				jcontours.PushBack(quads, allocator);
				// printf("\n");
			}

			rapidjson::Value jglyph(rapidjson::kObjectType);
			jglyph.AddMember("index", gindex, allocator);
			jglyph.AddMember("contours", jcontours, allocator);
			document.PushBack(jglyph, allocator);
		}
	}
	{
		// Convert the JSON object to a string
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		document.Accept(writer);

		auto file = filename;
		File::writeAll(buffer.GetString(), buffer.GetSize(), file.replace_extension(".json"));

		// Output the JSON string
		// std::cout << buffer.GetString() << std::endl;
	}


	// Cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	if (buffer)
		delete buffer;

	return 0;
}
