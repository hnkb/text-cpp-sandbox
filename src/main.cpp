#include "graphics/Mesh.h"
#include "text/Font.h"
#include <set>

using namespace std;


int main()
{
	if (0)
	{
		void saveSVG(const filesystem::path& filename);
		saveSVG("/Users/hani/Downloads/Logo.svg");
		return 0;
	}

	const filesystem::path folder = "/Users/hani/Downloads/fonts/";
	const set<string> extentions = { ".woff2", ".ttf", ".otf" };

	for (auto& entry: filesystem::directory_iterator(folder))
	{
		auto& path = entry.path();
		if (extentions.find(path.extension()) != extentions.end())
			saveFont_ttf2mesh(path);
	}
	return 0;
}
