#include "Font.h"

#include <ft2build.h>
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
vector<vector<float2>> currentContour;

float2 FT_Vector_to_float2(const FT_Vector* v)
{
	return float2(v->x, v->y) * normalizationMul;
}


#define CURVES_PRECISION 0.01

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
	return bezier(t, P0, P1, P2, P3);
}

// Function to calculate the Euclidean distance between two points
double distance(float2 a, float2 b)
{
	return sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
}

// Function to calculate the angle between two vectors
double angleBetween(float2 a, float2 b)
{
	double dotProduct = a.x * b.x + a.y * b.y;
	double magnitudeA = sqrt(a.x * a.x + a.y * a.y);
	double magnitudeB = sqrt(b.x * b.x + b.y * b.y);
	return acos(dotProduct / (magnitudeA * magnitudeB));
}

// Estimator function
int estimateBezierSegmentsCubic(float2 P0, float2 P1, float2 P2, float2 P3, double precision)
{
	double width = max({ P0.x, P1.x, P2.x, P3.x }) - min({ P0.x, P1.x, P2.x, P3.x });
	double height = max({ P0.y, P1.y, P2.y, P3.y }) - min({ P0.y, P1.y, P2.y, P3.y });
	double maxDimension = max(width, height);

	float2 vector1 = { P1.x - P0.x, P1.y - P0.y };
	float2 vector2 = { P3.x - P2.x, P3.y - P2.y };
	double maxAngleDeviation = angleBetween(vector1, vector2);

	return ceil((maxDimension * maxAngleDeviation) / precision);
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

	int segments = estimateBezierSegmentsCubic(P0, P1, P2, P3, CURVES_PRECISION);
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
	return (1 - t) * (1 - t) * P0 + 2 * (1 - t) * t * P1 + t * t * P2;
}

int estimateBezierSegmentsQuadratic(float2 P0, float2 P1, float2 P2, double precision)
{
	double width = max({ P0.x, P1.x, P2.x }) - min({ P0.x, P1.x, P2.x });
	double height = max({ P0.y, P1.y, P2.y }) - min({ P0.y, P1.y, P2.y });
	double maxDimension = max(width, height);

	float2 vector1 = { P1.x - P0.x, P1.y - P0.y };
	float2 vector2 = { P2.x - P1.x, P2.y - P1.y };
	double maxAngleDeviation = angleBetween(vector1, vector2);

	return ceil((maxDimension * maxAngleDeviation) / precision);
}

// Function to flatten a quadratic Bezier curve using line segments
int conicTo(const FT_Vector* control, const FT_Vector* to, void* user)
{
	float2 P0 = currentContour.back().back();  // Start point is the last point added
	float2 P1 = FT_Vector_to_float2(control);  // Control point
	float2 P2 = FT_Vector_to_float2(to);       // End point

	const int segments =
		estimateBezierSegmentsQuadratic(P0, P1, P2, CURVES_PRECISION);  // Number of line
																		// segments to
																		// approximate the
																		// Bezier curve
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

	// ...

	// Unicode string to hold all characters
	string allChars = "[";

	// Iterating over all characters in the font
	FT_ULong charcode;
	FT_UInt gindex;
	charcode = FT_Get_First_Char(face, &gindex);
	while (gindex != 0)
	{
		allChars += to_string(charcode) + ", ";
		charcode = FT_Get_Next_Char(face, charcode, &gindex);
	}
	allChars += "]";

	cout << allChars << endl;

	// ...

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
	if (buffer)
		delete buffer;

	currentContour.clear();
	contours.clear();

	cout << "Saved" << endl;

	return 0;
}
