#include "alfe/main.h"
#include "alfe/complex.h"

class Program : public ProgramBase
{
public:
	void run()
	{
		String input = File("D:\\t\\c2.raw", true).contents();
		Array<double> output(1820*2000);
		Array<Byte> outputb(1820*2000);

		// Sample rate = 28.6MHz
		// low = 3.3MHz = 8.6 samples
		// high = 4.5MHz = 6.4 samples

		double lastPhase = 0;

		for (int x = 0; x < 1820*2000 - 7; ++x) {
			Complex<double> iq = 0;
			for (int t = 0; t < 8; ++t)
				iq += unit(((x + t)&7)/8.0)*input[x+t];
			double phase = iq.argument() / tau;
			double deltaPhase = 1.3 + phase - lastPhase;
			int deltaPhaseInt = static_cast<int>(deltaPhase);
			deltaPhase -= deltaPhaseInt;
			lastPhase = phase;
			outputb[x] = byteClamp(static_cast<int>(385-255*4.5*deltaPhase));


			//int v = input[x];
			//if (rising) {
			//	if (v < last) {
			//		int t1 = input[x-1] - input[x-2];
			//		int t2 = input[x-1] - input[x];
			//		if (t1 == t2) {
			//			delta = ((x-1) - x_last) - d_last;
			//			d_last = 0;
			//			x_last = x-1;
			//		}
			//		else
			//			if (t1 < t2) {
			//				d = 0.5+t1/(t2*2);   // 1 puts peak at 0.5, 0.5 puts peak at 1
			//				delta = ((x-1) - x_last) - (d_last - d);
			//				d_last = d;
			//				x_last = x-1;
			//			}
			//			else {
			//				d = 0.5-t2/(t1*2);   // 1 puts peak at 0.5, 0.5 puts peak at 1
			//				delta = (x - x_last) - (d_last - d);
			//				d_last = d;
			//				x_last = x;
			//			}
			//		rising = false;
			//	}
			//}
			//else
			//	if (v > last)
			//		rising = true;
			//last = v;
			//++count;
			//output[x] = 255-(delta-6)*80;
		}
		FileHandle h = File("D:\\t\\vcr_decoded.raw", true).openWrite();
		h.write(outputb);
	}
};