#include "alfe/main.h"

class Program : public ProgramBase
{
public:
	void run()
	{
		Array<Byte> output(1600*1200*3);
		Byte* p = &output[0];
		for (int y = 0; y < 1200; ++y)
			for (int x = 0; x < 1600; ++x) {
				int r = ((x%200)*256)/200;
				int g = ((y%200)*256)/200;
				int b = (((x/200) + (y/200)*8)*256)/48;
				p[0] = r;
				p[1] = g;
				p[2] = b;
				p += 3;
			}
		File("output.raw", true).openWrite().write(output);
	}
};