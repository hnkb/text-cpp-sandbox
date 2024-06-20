#pragma once

#include "../utils/Math.h"
#include "../utils/File.h"
#include <delabella.h>


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
	~Collection()
	{
		if (idb)
			idb->Destroy();
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
	}

	void addPath()
	{
		if (meshes.empty())
			addMesh();

		if (currentPathStartVertex < vertices.size())
		{
			delabellaTriangulate(
				vertices.data() + currentPathStartVertex,
				vertices.size() - currentPathStartVertex);
			currentPathStartVertex = vertices.size();
		}
	}

	void addPoint(const float2& point) { vertices.push_back(point); }
	void addPoint(float x, float y) { vertices.emplace_back(x, y); }

	void save(const std::filesystem::path& filename);

private:
	void delabellaTriangulate(float2* start, size_t count);
	void finishMesh()
	{
		if (meshes.size())
		{
			addPath();
			meshes.back().indexCount = indices.size() - meshes.back().startIndex;
		}
	}

	int currentPathStartVertex = 0;

	std::vector<float2> vertices;
	std::vector<uint32_t> indices;
	std::vector<Mesh> meshes;

	IDelaBella2<float>* idb = nullptr;
};
