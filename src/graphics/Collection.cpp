#include "Mesh.h"


void Collection::delabellaTriangulate(float2* start, size_t count)
{
	if (!idb)
		idb = IDelaBella2<float>::Create();

	int verts = idb->Triangulate((int)count, &start->x, &start->y, sizeof(float2));
	if (verts > 0)
	{
		const auto tris = idb->GetNumPolygons();
		auto dela = idb->GetFirstDelaunaySimplex();
		for (int i = 0; i < tris; i++)
		{
			indices.push_back(dela->v[0]->i + currentPathStartVertex);
			indices.push_back(dela->v[1]->i + currentPathStartVertex);
			indices.push_back(dela->v[2]->i + currentPathStartVertex);
			dela = dela->next;
		}
	}
	else
	{
		fprintf(stderr, "ERROR: no points given or all points are colinear\n");
	}
}

void Collection::save(const std::filesystem::path& filename)
{
	finishMesh();

	File::Pack output(filename, 'w', "FNTMSH");
	output.add("vert", vertices, 20);
	output.add("idx", indices, 20);
	output.add("mesh", meshes, 20);

	printf(
		"Collection '%s' with %zu meshes, %zu vertices, and %zu indices saved.\n",
		filename.stem().c_str(),
		meshes.size(),
		vertices.size(),
		indices.size());
}
