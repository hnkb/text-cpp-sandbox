#include "text/Font.h"
#include <iostream>

using namespace std;


int main()
{
	const auto text = "playground\nfor text";

	const filesystem::path font =
		"/Users/hani/Downloads/fonts/Overpass/Overpass-VariableFont_wght.ttf";

	auto glyphs = shapeWithHarfbuzz(text, font);
	for (const auto& glyph: glyphs)
	{
		cout << "Glyph ID: " << glyph.index << ", ";
		cout << "Pos: " << glyph.pos.x << ", " << glyph.pos.y << endl;
	}

	return 0;
}
