#include "alfe/main.h"
#include "alfe/complex.h"

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= M_PI;
    return sin(z)/z;
}

static const int lobes = 5;

float lanczos(float z)
{
    return sinc(z)*sinc(z/lobes);
}

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> input;
        FileHandle inputHandle = File("D:\\t\\c2.raw", true).openRead();
        int n = inputHandle.size();
        input.allocate(n);
        inputHandle.read(&input[0], n);

        Array<float> output(1820*2000);
        Array<Byte> outputb(1820*2000);
        static const int resampleFactor = 100;
        Array<float> resampled(1820*resampleFactor);

        // Sample rate = 28.6MHz*100
        // low = 3.3MHz = 860 samples  = 
        // high = 4.5MHz = 640 samples

        float lastPhase = 0;
        float cutoffSamples = 2.8;
        float kernelSize = lobes;

        for (int y = 1; y < 240 /*1999*/; ++y) {
            //float minResampled = 1e99;
            //float maxResampled = -1e99;
            for (int x = 0; x < 1820*resampleFactor; ++x) {
                float kFrac = static_cast<float>(x)/resampleFactor;
                int k = static_cast<int>(kFrac);
                kFrac -= k;
                float z0 = -kFrac/cutoffSamples;                     // In cutoff periods
                k += y*1820;
                int firstInput = -kernelSize*cutoffSamples + kFrac;
                int lastInput = kernelSize*cutoffSamples + kFrac;
                float v = 0;
                float t = 0;
                for (int j = firstInput; j <= lastInput; ++j) {
                    float s = lanczos(j/cutoffSamples + z0);      // When j=0 we're looking at 
                    int i = j + k;
                    float z = s*input[i];
                    v += z;
                    t += s;
                }
                v /= t;
                resampled[x] = v;
                //if (v < -1000 || v > 1000)
                //    printf("v = %f\n",v);
                //minResampled = min(v, minResampled);
                //maxResampled = max(v, maxResampled);
            }
            //printf("%f %f\n",minResampled,maxResampled);

            //Array<Byte> resampledb(1820*100);
            //for (int i = 0; i < 1820*100; ++i)
            //    resampledb[i] = byteClamp(resampled[i]);
            //FileHandle h = File("D:\\t\\resampled.raw", true).openWrite();
            //h.write(resampledb);
            //return;

            //

            // Value range is 38..57
            // 
            float low = 38;
            float high = 57;
            bool rising = true;
            bool passedZero = true;
            int lastChange = 0;
            int length = 0;
            for (int x = 0; x < 1820*resampleFactor; ++x) {
                float v = resampled[x];
                if (rising) {
                    if (!passedZero) {
                        if (v > (high + low)/2.0) {
                            //length = x - lastChange;
                            //lastChange = x;
                            passedZero = true;
                            high = v;
                        }
                    }
                    else {
                        high = max(high, v);
                        if (high - v > 5) {
                            rising = false;
                            passedZero = false;
                        }
                    }
                }
                else {
                    if (!passedZero) {
                        if (v <= (high + low)/2.0) {
                            length = x - lastChange;
                            lastChange = x;
                            passedZero = true;
                            low = v;
                        }
                    }
                    else {
                        low = min(low, v);
                        if (v - low > 5) {
                            rising = true;
                            passedZero = false;
                        }
                    }
                }
                outputb[x/resampleFactor + y*1820] = byteClamp(878 - length*100.0/resampleFactor);
            }
            printf(".");
        }

        // Output sample = x
        // Output sample in cutoffs = x/100/cutoffSamples
        // Input sample = j+k
        // Input sample in cutoffs = (j+k)/cutoffSamples
        // Difference = (j+k)/cutoffSamples - x/100/cutoffSamples
        // = j/cutoffSamples + (k - kFrac)/cutoffSamples


        //for (int x = 0; x < 1820*2000 - 7; ++x) {
        //    Complex<double> iq = 0;
        //    for (int t = 0; t < 8; ++t)
        //        iq += unit(((x + t)&7)/8.0)*input[x+t];
        //    double phase = iq.argument() / tau;
        //    double deltaPhase = 1.3 + phase - lastPhase;
        //    int deltaPhaseInt = static_cast<int>(deltaPhase);
        //    deltaPhase -= deltaPhaseInt;
        //    lastPhase = phase;
        //    outputb[x] = byteClamp(static_cast<int>(385-255*4.5*deltaPhase));


        //    //int v = input[x];
        //    //if (rising) {
        //    //	if (v < last) {
        //    //		int t1 = input[x-1] - input[x-2];
        //    //		int t2 = input[x-1] - input[x];
        //    //		if (t1 == t2) {
        //    //			delta = ((x-1) - x_last) - d_last;
        //    //			d_last = 0;
        //    //			x_last = x-1;
        //    //		}
        //    //		else
        //    //			if (t1 < t2) {
        //    //				d = 0.5+t1/(t2*2);   // 1 puts peak at 0.5, 0.5 puts peak at 1
        //    //				delta = ((x-1) - x_last) - (d_last - d);
        //    //				d_last = d;
        //    //				x_last = x-1;
        //    //			}
        //    //			else {
        //    //				d = 0.5-t2/(t1*2);   // 1 puts peak at 0.5, 0.5 puts peak at 1
        //    //				delta = (x - x_last) - (d_last - d);
        //    //				d_last = d;
        //    //				x_last = x;
        //    //			}
        //    //		rising = false;
        //    //	}
        //    //}
        //    //else
        //    //	if (v > last)
        //    //		rising = true;
        //    //last = v;
        //    //++count;
        //    //output[x] = 255-(delta-6)*80;
        //}
        FileHandle h = File("D:\\t\\vcr_decoded.raw", true).openWrite();
        h.write(outputb);
    }
};