#include "Mesh.h"

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>

using namespace std;


void saveSVG(const filesystem::path& filename)
{
	auto image = nsvgParseFromFile(filename.c_str(), "px", 96.0f);
	if (!image)
		throw runtime_error("Could not open SVG image.");

	printf("SVG image with size %f x %f\n", image->width, image->height);

	Collection output;

	for (auto shape = image->shapes; shape != NULL; shape = shape->next)
	{
		output.addMesh(shape->fill.color, shape->opacity);
		printf(
			"Shape '%s'  fill %08x  stroke %08x  opacity %f\n",
			shape->id,
			shape->fill.color,
			shape->stroke.color,
			shape->opacity);

		for (auto path = shape->paths; path != NULL; path = path->next)
		{
			output.addPath();
			// printf(
			// 	"    Path with %d points, closed=%d, bounds %f %f %f %f\n",
			// 	path->npts,
			// 	(int)path->closed,
			// 	path->bounds[0],
			// 	path->bounds[1],
			// 	path->bounds[2],
			// 	path->bounds[3]);

			for (int i = 0; i < path->npts - 1; i += 3)
			{
				auto& p0 = *(float2*)&path->pts[i * 2];
				auto& p1 = *(float2*)&path->pts[i * 2 + 2];
				auto& p2 = *(float2*)&path->pts[i * 2 + 4];
				auto& p3 = *(float2*)&path->pts[i * 2 + 6];

				// cout << "\n" << i << "\n";
				// cout << p0.x << ", " << p0.y << "\n";
				// cout << p1.x << ", " << p1.y << "\n";
				// cout << p2.x << ", " << p2.y << "\n";
				// cout << p3.x << ", " << p3.y << "\n";


				constexpr auto epsilon = 0.0001f;

				auto cross1 = cross(p1 - p0, p3 - p0);
				auto cross2 = cross(p2 - p0, p3 - p0);

				if (abs(cross1) < epsilon && abs(cross2) < epsilon)
				{
					// straight line
					// output.addPoint(p0);
					output.addPoint(p3);
				}
				else
					for (float t = 0; t <= 1; t += 0.05f)
						output.addPoint(bezier(t, p0, p1, p2, p3));
			}
		}
	}

	auto file = filename;
	output.save(file.replace_extension(".mesh"));

	nsvgDelete(image);
}
