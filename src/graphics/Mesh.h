#pragma once

#include "../utils/Math.h"
#include "../utils/File.h"
#include <tesselator.h>
#include <clipper.hpp>


struct Mesh
{
	int startIndex;
	int indexCount;

	Mesh() : startIndex(0), indexCount(0) {}
	Mesh(int startIndex) : startIndex(startIndex), indexCount(0) {}
	Mesh(int startIndex, int indexCount) : startIndex(startIndex), indexCount(indexCount) {}
};

class Collection
{
public:
	Collection()
	{
		vertices.reserve(1000);
		indices.reserve(6000);
	}

	void addMesh(uint32_t color = 0xff00'0000, float opacity = 1.0f)
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

	void addPath(const std::vector<float2>& points)
	{
		ClipperLib::Path path;
		for (const auto& p: points)
		{
			path << ClipperLib::IntPoint(
				static_cast<int>(p.x * 100'000),
				static_cast<int>(p.y * 100'000));
		}
		pathBuffer.push_back(path);

		if (meshes.empty())
			addMesh();
	}

	void save(const std::filesystem::path& filename);

private:
	void finishMesh()
	{
		ClipperLib::Paths solution;
		if (!pathBuffer.empty())
		{
			ClipperLib::Clipper clipper;
			clipper.AddPaths(pathBuffer, ClipperLib::ptSubject, true);
			clipper.Execute(
				ClipperLib::ctUnion,
				solution,
				ClipperLib::pftNonZero,
				ClipperLib::pftNonZero);

			pathBuffer.clear();  // Clear the path buffer after union
		}

		// Convert Clipper solution to libtess2 input
		tess = tessNewTess(nullptr);
		for (const auto& path: solution)
		{
			std::vector<float2> tessInput;
			for (const auto& pt: path)
			{
				tessInput.push_back(
					{ static_cast<float>(pt.X) / 100000.0f,
					  static_cast<float>(pt.Y) / 100000.0f });
			}
			tessAddContour(tess, 2, tessInput.data(), sizeof(float2), tessInput.size());
		}

		if (tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, nullptr))
		{
			auto* tessVertices = (float2*)tessGetVertices(tess);
			int numVertices = tessGetVertexCount(tess);
			copy_n(tessVertices, numVertices, back_inserter(vertices));

			const auto* elem = tessGetElements(tess);
			int numIndices = 3 * tessGetElementCount(tess);
			for (int i = 0; i < numIndices; i++)
				indices.push_back(elem[i] + startVertex);
		}

		tessDeleteTess(tess);
		tess = nullptr;

		if (!meshes.empty())
			meshes.back().indexCount = indices.size() - meshes.back().startIndex;
	}

	int startVertex = 0;
	TESStesselator* tess = nullptr;

	std::vector<float2> vertices;
	std::vector<uint32_t> indices;
	std::vector<Mesh> meshes;

	ClipperLib::Paths pathBuffer;
};
