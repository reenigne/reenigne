#include "alfe/main.h"

class Program : public ProgramBase
{
public:
	void run()
	{
		static const int length = 60*44100;
		Array<SInt16> output(length);

		Array<int> sine(256);
		for (int i = 0; i < 256; ++i)
			sine[i] = sin(i*tau/256.0)*256;
		Array<Byte> active(16*16);
		for (int i = 0; i < 256; ++i)
			active[i] = 0;
		Array<Word> phase(16);
		for (int i = 0; i < 16; ++i)
			phase[i] = 0;
		Array<Byte> volumes(16);
		for (int i = 0; i < 16; ++i)
			volumes[i] = 0;

		//float freqs[16] = {
		//	// A    F#     E	 D	  B
		//	1760, 1467, 1320, 1173, 990,
		//	 880,  733,  660,  587, 495, 
		//	 440,  367,  330,  293, 248,
		//	 220 };

		float freqs[16] = {
			// A     G     E     D     C
			1760, 1568, 1320, 1173, 1047,
			 880,  784,  660,  587,  523,
			 440,  392,  330,  293,  262,
			 220 };

		Array<int> scale(16);
		for (int i = 0; i < 16; ++i)
			scale[i] = static_cast<int>(freqs[i]*44100/65536);

		int nActive = 16;
		for (int i = 0; i < nActive; ++i) {
			int n = rand() % (256 - i);
			for (int j = 0; j < 256; ++j) {
				if (active[j] == 0) {
					if (n == 0) {
						active[j] = 1;
						break;
					}
					--n;
				}
			}
		}

		int beatInPattern = 0;
		int frameInBeat = 0;
		int sampleInFrame = 0;
		int beatFrames = 20;
		int frameSamples = 367; //735;

		for (int t = 0; t < length; ++t) {
			int sample = 0;
			for (int i = 0; i < 16; ++i) {
				phase[i] += scale[i];
				sample += sine[(phase[i] >> 8) & 0xff]*volumes[i];
			}
			sample = (sample*4)/32;
			output[t] = sample;
			++sampleInFrame;
			if (sampleInFrame == frameSamples) {
				for (int i = 0; i < 16; ++i)
					volumes[i] = (volumes[i]*250)/255;
				sampleInFrame = 0;
				++frameInBeat;
				if (frameInBeat == beatFrames) {
					frameInBeat = 0;
					++beatInPattern;
					for (int i = 0; i < 16; ++i)
						if (active[beatInPattern + i*16] != 0)
							volumes[i] = 255;
						
					if (beatInPattern == 16) {
						beatInPattern = 0;

						int nOn = rand() % (256 - nActive);
						int nOff = rand() % nActive;
						for (int j = 0; j < 256; ++j) {
							if (active[j] == 0) {
								if (nOn == 0)
									active[j] = 1;
								--nOn;
							}
							else {
								if (nOff == 0)
									active[j] = 0;
								--nOff;
							}
						}
						//for (int i = 0; i < 256; ++i) {
						//	printf("%i",active[i]);
						//	if ((i & 15) == 15)
						//		printf("\n");
						//}
					}
				}
			}
		}

		File("output.pcm", true).openWrite().write(output);
	}
};