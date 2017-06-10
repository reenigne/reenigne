#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/set.h"
#include "alfe/cga.h"

#include "palette.h"

class Program : public ProgramBase
{
public:
	void run()
	{
		//Set<DWord> palette;
		//Linearizer linearizer;
		//for (int f = 0; f < 0x10; ++f) {
		//	SRGB fc = rgbiPalette[f];
		//	Colour fLinear = linearizer.linear(fc);
		//	for (int b = 0; b < 0x10; ++b) {
		//		SRGB bc = rgbiPalette[b];
		//		Colour bLinear = linearizer.linear(bc);
		//		for (int l = 0; l < 0x100000; ++l) {
		//			Colour cLinear = (fLinear*l + bLinear*(0x100000 - l))/1048576.0;
		//			SRGB c = linearizer.srgb(cLinear);
		//			palette.add(c.x*0x10000 + c.y*0x100 + c.z + 1);
		//		}
		//	}
		//}
		//int n = palette.count();
		//console.write(String(decimal(n)) + "\n");
		//Array<DWord> sortedPalette(n);
		//int ii = 0;
		//for (auto i : palette) {
		//	sortedPalette[ii] = i;
		//	++ii;
		//}
		//std::sort(&sortedPalette[0], &sortedPalette[n]);

		//int nn = 0;
		//for (ii = 0; ii < n; ++ii) {
		//	printf("0x%06x,",sortedPalette[ii] - 1);
		//	++nn;
		//	if (nn == 8) {
		//		printf("\n");
		//		nn = 0;
		//	}
		//	else
		//		printf(" ");
		//}
		int nPalette = sizeof(palette)/sizeof(palette[0]);

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
				float bestMetric = 1e99;

				for (int i = 0; i < nPalette; ++i) {
					DWord ct = palette[i];
					SRGB t((ct >> 16) & 0xff, (ct >> 8) & 0xff, ct & 0xff);
					float dr = static_cast<float>(c.x - t.x);
					float dg = static_cast<float>(c.y - t.y);
					float db = static_cast<float>(c.z - t.z);
					// Fast colour distance metric from
					// http://www.compuphase.com/cmetric.htm .
					float mr = (c.x + t.x)/512.0f;
					float metric = 4.0f*dg*dg + (2.0f + mr)*dr*dr +
						(3.0f - mr)*db*db;
					if (metric < bestMetric) {
						bestC = t;
						bestMetric = metric;
					}
				}

				*outputP = bestC;

				++p;
				++outputP;
			}
			line += input.stride();
			outputLine += output.stride();
			printf(".");
		}

		PNGFileFormat<SRGB>().save(output,
			File(inputFileName + "_out.png", true));
	}
};