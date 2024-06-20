#include "Mesh.h"


void Collection::save(const std::filesystem::path& filename)
{
	finishMesh();

	for (auto& v: vertices)
		v.y = 1 - v.y;

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
