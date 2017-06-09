#include "alfe/main.h"
#include "alfe/bitmap_png.h"

class Program : public ProgramBase
{
public:
	void run()
	{
		if (_arguments.count() < 2) {
			console.write("Syntax: " + _arguments[0] +
				" <input file name>.png\n");
			return;
		}
		String pngName = _arguments[1];

		String inputFileName = pngName;
		int i;
		for (i = inputFileName.length() - 1; i >= 0; --i)
			if (inputFileName[i] == '.')
				break;
		if (i != -1)
			inputFileName = inputFileName.subString(0, i);

		Bitmap<SRGB> input = PNGFileFormat<SRGB>().load(File(pngName, true));
		Vector size = input.size();
		Bitmap<SRGB> output(size);

		const Byte* line = input.data();
		Byte* outputLine = output.data();
		for (int y = 0; y < size.y; ++y) {
			const SRGB* p = reinterpret_cast<const SRGB*>(line);
			SRGB* outputP = reinterpret_cast<SRGB*>(outputLine);
			for (int x = 0; x < size.x; ++x) {
				SRGB c = *p;

				SRGB bestC;
				float bestMetric;
				for (int pair = 0; pair < 0x10000; ++pair) {
				}


				++p;
				++outputP;
			}
			line += input.stride();
			outputLine += output.stride();
		}


		PNGFileFormat<SRGB>().save(output,
			File(inputFileName + "_out.png", true));
	}
};