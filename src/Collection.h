#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <iostream>
#include <tesselator.h>
#include "clipper.hpp"

#include "vectors.h"

using namespace std;
using namespace ClipperLib;

#define OUTPUT_TRIANGLES 1
#define APPLY_UNION 0

struct Mesh
{
	int startIndex;
	int indexCount;

	Mesh(int startIndex) : startIndex(startIndex), indexCount(0) {
		//
	}

	Mesh() {
		//
	}
};

class Collection {
private:
    int startVertex = 0;
    TESStesselator* tess = nullptr;

    vector<float2> vertices;
    vector<uint32_t> indices;
    vector<Mesh> meshes;

    ClipperLib::Paths pathBuffer;

public:
    Collection()
    {
        vertices.reserve(1000);
        indices.reserve(6000);
    }

    void addMesh(uint32_t color = 0xff000000, float opacity = 1.0f)
    {
        finishMesh();
        meshes.emplace_back(indices.size());

        const auto r = (color >> 0) & 0xff;
        const auto g = (color >> 8) & 0xff;
        const auto b = (color >> 16) & 0xff;
        const auto a = (color >> 24) & 0xff;

        const auto alpha = (int)(a * opacity);

        startVertex = (int)vertices.size();
    }

    void addPath(const vector<float2>& points)
    {
		ClipperLib::Path path;
		for (const auto& p : points) {
			path << ClipperLib::IntPoint(static_cast<int>(p.x * 100000), static_cast<int>(p.y * 100000));
		}
		pathBuffer.push_back(path);

        if (meshes.empty())
            addMesh();
    }

    void save(const filesystem::path& filename);

private:
    void finishMesh()
    {
		ClipperLib::Paths solution;
		if (!pathBuffer.empty()) {
			ClipperLib::Clipper clipper;
			clipper.AddPaths(pathBuffer, ClipperLib::ptSubject, true);
			if (APPLY_UNION) {
				clipper.Execute(ClipperLib::ctUnion, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
			} else {
				solution = pathBuffer;
			}
		}

		if (OUTPUT_TRIANGLES) {
			// Convert Clipper solution to libtess2 input
			tess = tessNewTess(nullptr);
			for (const auto& path : solution) {
				vector<float2> tessInput;
				for (const auto& pt : path) {
					tessInput.push_back({static_cast<float>(pt.X) / 100000.0f, static_cast<float>(pt.Y) / 100000.0f});
				}
				tessAddContour(tess, 2, tessInput.data(), sizeof(float2), tessInput.size());
			}

			if (tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, nullptr)) {
				auto* tessVertices = (float2*)tessGetVertices(tess);
				int numVertices = tessGetVertexCount(tess);
				copy_n(tessVertices, numVertices, back_inserter(vertices));

				const auto* elem = tessGetElements(tess);
				int numIndices = 3 * tessGetElementCount(tess);
				for (int i = 0; i < numIndices; i++) {
					indices.push_back(elem[i] + startVertex);
				}
			}

			tessDeleteTess(tess);
			tess = nullptr;
		} else {
			vector<float2> points;
			for (const auto& path : solution) {
				points.clear();
				for (const auto& pt : path) {
					points.push_back({static_cast<float>(pt.X) / 100000.0f, static_cast<float>(pt.Y) / 100000.0f});
				}
				
				for (size_t i = 0; i < points.size(); ++i) {
					indices.push_back(startVertex + i);     // Index of current vertex
					indices.push_back(startVertex + (i + 1) % points.size()); // Index of next vertex
				}

				for (size_t i = 0; i < points.size(); ++i) {
					vertices.push_back(points[i]);
				}

        		startVertex = (int)vertices.size();
				// cout << "points count: " << points.size() << endl;
			}
		}

		pathBuffer.clear();  // Clear the path buffer after union

		if (!meshes.empty())
			meshes.back().indexCount = indices.size() - meshes.back().startIndex;
	}
};

template <typename T>
void writeToFile(const filesystem::path& filename, const vector<T>& data)
{
	ofstream file(filename, ios::binary);
	file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));
}

void Collection::save(const filesystem::path& filename)
{
	finishMesh();

	auto outFile = filename;
	outFile.replace_extension(".vert");
	writeToFile(outFile, vertices);

	outFile.replace_extension(".idx");
	writeToFile(outFile, indices);

	outFile.replace_extension(".mesh");
	writeToFile(outFile, meshes);

	printf(
		"Collection '%s' with %zu meshes, %zu vertices, and %zu indices saved.\n",
		filename.stem().c_str(),
		meshes.size(),
		vertices.size(),
		indices.size());
}