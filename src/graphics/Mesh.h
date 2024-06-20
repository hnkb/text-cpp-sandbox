#pragma once

#include "../utils/Math.h"
#include "../utils/File.h"
#include <tesselator.h>


struct Mesh
{
	int startIndex;
	int indexCount;
	uint32_t color;

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

		meshes.back().color = (r << 24) | (g << 16) | (b << 8) | (alpha & 0xff);

		startVertex = (int)vertices.size();
		tess = tessNewTess(nullptr);
		if (!tess)
		{
			fprintf(stderr, "Error: Couldn't create tessellator");
			return;
		}
		tessSetOption(tess, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);
	}

	void addPath(const std::vector<float2>& points)
	{
		if (meshes.empty())
			addMesh();
		tessAddContour(tess, 2, points.data(), sizeof(float2), points.size());
	}

	void save(const std::filesystem::path& filename);

private:
	void finishMesh()
	{
		if (tess)
		{
			if (!tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, nullptr))
			{
				fprintf(stderr, "Error: Couldn't tessellate");
				return;
			}

			copy_n(
				(float2*)tessGetVertices(tess),
				tessGetVertexCount(tess),
				back_inserter(vertices));

			auto elem = tessGetElements(tess);
			const auto numIndices = 3 * tessGetElementCount(tess);
			for (int i = 0; i < numIndices; i++)
				indices.push_back(elem[i] + startVertex);

			tessDeleteTess(tess);
			tess = nullptr;
		}
		if (meshes.size())
			meshes.back().indexCount = indices.size() - meshes.back().startIndex;
	}

	int startVertex = 0;
	TESStesselator* tess = nullptr;

	std::vector<float2> vertices;
	std::vector<uint32_t> indices;
	std::vector<Mesh> meshes;
};
