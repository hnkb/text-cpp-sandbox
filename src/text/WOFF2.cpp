#include "Font.h"
#include "../utils/File.h"
#include <woff2/decode.h>

using namespace std;


string* readWOFF2(const filesystem::path& filename)
{
	const auto compressed = File::readAll(filename);
	auto estimate = woff2::ComputeWOFF2FinalSize(compressed.data(), compressed.size());
	auto buffer = new string(estimate, 0);
	woff2::WOFF2StringOut output(buffer);
	if (!woff2::ConvertWOFF2ToTTF(compressed.data(), compressed.size(), &output))
	{
		printf("%s decompression failed.\n", filename.u8string().c_str());
		buffer->clear();
	}
	return buffer;
}
