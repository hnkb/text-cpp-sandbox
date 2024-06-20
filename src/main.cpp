#include "graphics/Mesh.h"
#include "text/Font.h"
#include <chrono>
#include <set>

using namespace std;


int main()
{
	if (0)
	{
		const auto start = chrono::high_resolution_clock::now();

		void saveSVG(const filesystem::path& filename);
		saveSVG("/Users/hani/Downloads/Logo.svg");

		const auto end = chrono::high_resolution_clock::now();
		const auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
		printf("Conversion took %lld ms\n", duration);

		return 0;
	}

	const filesystem::path folder = "/Users/hani/Downloads/fonts/";
	const set<string> extentions = { ".woff2", ".ttf", ".otf" };

	for (auto& entry: filesystem::directory_iterator(folder))
	{
		auto& path = entry.path();
		if (extentions.find(path.extension()) != extentions.end())
			saveFontUsingFreeTypeAndLibTess(path);
	}
	return 0;
}
