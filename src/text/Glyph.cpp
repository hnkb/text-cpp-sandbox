#include "Font.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <iostream>

using namespace std;


float normalizationMul;

struct ContourWithIndex
{
	FT_UInt index;
	vector<vector<float2>> subContours;

	bool operator==(const ContourWithIndex& rhs) const { return this->index == rhs.index; }
};
vector<ContourWithIndex> contours;

float2 FT_Vector_to_float2(const FT_Vector* v)
{
	return float2({ v->x * normalizationMul, v->y * normalizationMul });
}

vector<vector<float2>> currentContour;

int moveTo(const FT_Vector* to, void* user)
{
	currentContour.push_back({});
	currentContour.back() = { FT_Vector_to_float2(to) };
	return 0;  // Return value of 0 indicates success
}

int lineTo(const FT_Vector* to, void* user)
{
	currentContour.back().push_back(FT_Vector_to_float2(to));
	return 0;
}

// Evaluate a cubic Bezier at a given t
float2 cubicBezier(
	const float2& P0,
	const float2& P1,
	const float2& P2,
	const float2& P3,
	double t)
{
	double x =
		pow(1 - t, 3) * P0.x + 3 * pow(1 - t, 2) * t * P1.x + 3 * (1 - t) * pow(t, 2) * P2.x
		+ pow(t, 3) * P3.x;
	double y =
		pow(1 - t, 3) * P0.y + 3 * pow(1 - t, 2) * t * P1.y + 3 * (1 - t) * pow(t, 2) * P2.y
		+ pow(t, 3) * P3.y;
	return float2({ (float)x, (float)y });
}

// Simple function to approximate a cubic Bezier curve with line segments
int cubicTo(
	const FT_Vector* control1,
	const FT_Vector* control2,
	const FT_Vector* to,
	void* user)
{
	float2 P0 = currentContour.back().back();  // Last point added is the start of this curve
	float2 P1 = FT_Vector_to_float2(control1);
	float2 P2 = FT_Vector_to_float2(control2);
	float2 P3 = FT_Vector_to_float2(to);

	int segments = 20;  // This can be adjusted based on desired precision
	for (int i = 1; i <= segments; ++i)
	{
		double t = i / double(segments);
		float2 pt = cubicBezier(P0, P1, P2, P3, t);
		currentContour.back().push_back(pt);
	}

	return 0;
}

// Evaluate a quadratic Bezier curve at a given t
float2 quadraticBezier(const float2& P0, const float2& P1, const float2& P2, double t)
{
	double x = (1 - t) * (1 - t) * P0.x + 2 * (1 - t) * t * P1.x + t * t * P2.x;
	double y = (1 - t) * (1 - t) * P0.y + 2 * (1 - t) * t * P1.y + t * t * P2.y;
	return float2({ (float)x, (float)y });
}

// Function to flatten a quadratic Bezier curve using line segments
int conicTo(const FT_Vector* control, const FT_Vector* to, void* user)
{
	float2 P0 = currentContour.back().back();  // Start point is the last point added
	float2 P1 = FT_Vector_to_float2(control);  // Control point
	float2 P2 = FT_Vector_to_float2(to);       // End point

	const int segments = 20;  // Number of line segments to approximate the Bezier curve
	for (int i = 1; i <= segments; ++i)
	{
		double t = i / double(segments);
		float2 pt = quadraticBezier(P0, P1, P2, t);
		currentContour.back().push_back(pt);
	}

	return 0;  // Return 0 to indicate success
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

	// Load a font face from a font file
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

	normalizationMul = 1.0f / (float)face->units_per_EM;

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

	/*
	// sanity check: everything should be sequential
	for (int contourIndex = 0; contourIndex < contours.size(); contourIndex++) {
		ContourWithIndex contour = contours[contourIndex];
		if (contour.index != contourIndex) {
			cout << contour.index << " is at " << contourIndex << endl;
		}
	}
	*/

	cout << "Loaded font: " << face->family_name << ", " << face->style_name << endl;

	cout << "Clipping and Tesselating..." << endl;
	auto start = chrono::high_resolution_clock::now();

	Collection output;
	for (int contourIndex = 0; contourIndex < contours.size(); contourIndex++)
	{
		output.addMesh();
		ContourWithIndex contour = contours[contourIndex];
		for (int subContourIndex = 0; subContourIndex < contour.subContours.size();
			 subContourIndex++)
		{
			vector<float2> subContour = contour.subContours[subContourIndex];
			if (subContour.size() > 0)
				output.addPath(subContour);
		}
	}
	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> duration = end - start;
	cout << "Execution time: " << duration.count() << " seconds" << endl;

	cout << "Saving..." << endl;

	auto outfile = filename;
	output.save(outfile.replace_extension(".bin"));

	// Cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(library);

	cout << "Saved" << endl;

	return 0;
}
