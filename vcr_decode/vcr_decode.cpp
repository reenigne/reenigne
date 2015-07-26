#include "alfe/main.h"
#include "alfe/complex.h"
#include "alfe/colour_space.h"
#include "fftw3.h"

float preamp[] = {
  50.0, //  131.717972,
  50.0, //  99.066330,
  50.0, //  57.235191,
  50.0, //  31.985214,
  50.0, //  42.455242,
  50.0, //  54.362297,
  50.0, //  53.158817,
  50.0, //  44.854256,
  50.0, //  38.591202,
  50.0, //  41.439701,
  50.0, //  47.131847,
  50.0, //    50.507721,
  50.0, //    47.894325,
  50.0, //    43.842155,
  50.0, //    44.105804,
  51.250809,
  55.293766,
  52.829926,
  48.648155,
  53.144211,
  60.727947,
  62.886639,
  59.236660,
  59.417824,
  65.317169,
  70.915886,
  71.066559,
  70.171486,
  71.635559,
  76.507912,
  79.595840,
  80.578156,
  80.147758,
  81.534538,
  84.577538,
  88.057564,
  88.007866,
  87.471085,
  88.784683,
  93.012978,
  93.933510,
  92.771317,
  92.870209,
  96.410736,
  98.649315,
  97.632446,
  95.400223,
  98.003036,
  100.857582,
  100.781311,
  98.689583,
  100.130798,
  102.200394,
  102.702477,
  101.589912,
  103.132942,
  105.029419,
  105.195679,
  104.136658,
  105.785393,
  108.758194,
  109.016121,
  107.399094,
  108.403770,
  110.286720,
  111.278099,
  110.423889,
  111.651085,
  113.737419,
  113.947441,
  113.132278,
  114.636902,
  117.386353,
  118.569450,
  116.435684,
  116.505341,
  118.826492,
  120.834862,
  119.372063,
  117.781837,
  118.840759,
  120.393631,
  119.886414,
  119.425652,
  118.937302,
  119.470383,
  119.114441,
  119.132561,
  118.913589,
  119.520508,
  119.259209,
  119.819557,
  119.517570,
  118.784134,
  118.154175,
  119.327705,
  118.075691,
  116.252640,
  114.706184,
  115.571106,
  117.492737,
  117.208977,
  116.983711,
  125.138832,
  128.440567,
  131.280502,
  130.454224,
  130.403000,
  130.925812,
  132.234375,
  132.378281,
  133.168411,
  133.402359,
  133.105240,
  132.756943,
  133.854874,
  134.704895,
  134.662704,
  134.000244,
  135.714157,
  137.518478,
  137.914352,
  137.566513,
  138.444214,
  140.500549,
  140.843521,
  138.504379,
  138.448563,
  139.768539,
  141.214066,
  139.467224,
  136.000351,
  136.020477,
  138.070114,
  138.947449,
  139.354599,
  138.483765,
  138.798721,
  139.347412,
  140.662689,
  139.692627,
  138.783386,
  138.318176,
  139.321045,
  140.523178,
  138.861862,
  137.958008,
  138.954071,
  140.904022,
  140.341705,
  138.652130,
  138.883118,
  139.999985,
  141.033356,
  139.607620,
  137.664337,
  138.053360,
  138.895218,
  139.113297,
  137.314926,
  134.596329,
  134.654266,
  135.920090,
  135.505508,
  133.371048,
  130.916534,
  130.823639,
  130.787933,
  129.587631,
  126.507828,
  125.233131,
  124.430168,
  124.094849,
  121.953766,
  120.755920,
  119.105507,
  118.158447,
  115.818001,
  115.033089,
  112.617516,
  111.608864,
  110.166626,
  109.140785,
  106.590744,
  105.259476,
  103.301109,
  101.927650,
  99.601349,
  96.281143,
  92.747803,
  91.090240,
  89.640907,
  86.257065,
  83.085129,
  81.199257,
  79.358566,
  76.602379,
  72.350037,
  68.928642,
  66.846069,
  64.539261,
  60.905903,
  57.590942,
  54.329788,
  50.727127,
  47.210518};

int scanlineOffsets[8] = {0, 0, 0, 0, 0, 0, 0, 0};

class Program : public ProgramBase
{
public:
    void run()
    {
        FileHandle inputHandle = File("U:\\c2.raw", true).openRead();
        //FileHandle inputHandle = File("U:\\captured.bin", true).openRead();
        int nn = inputHandle.size();

        FileHandle h = File("U:\\vcr_decoded.bin", true).openWrite();

        int samplesPerFrame = 1824*253;
        Array<Byte> input(samplesPerFrame);
        Array<Complex<float>> fftData(2048);
        Array<Complex<float>> chromaData(2048);
        Array<Complex<float>> burstData(91);
        fftwf_plan forward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&fftData[0]), reinterpret_cast<fftwf_complex*>(&fftData[0]), -1, FFTW_MEASURE);
        fftwf_plan backward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&fftData[0]), reinterpret_cast<fftwf_complex*>(&fftData[0]), 1, FFTW_MEASURE);
        fftwf_plan chromaForward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&chromaData[0]), reinterpret_cast<fftwf_complex*>(&chromaData[0]), -1, FFTW_MEASURE);
        fftwf_plan chromaBackward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&chromaData[0]), reinterpret_cast<fftwf_complex*>(&chromaData[0]), 1, FFTW_MEASURE);
        Complex<float> localOscillatorFrequency = unit(4.4f*11/315);
        Complex<float> chromaOscillatorFrequency = unit(2.0f/91);  // 0.629 == 315/11 / 2 / 4 * 16 / 91      0.629f*11/315 == 16/8/91


        //static const float cutoff = 3.0f*2048*11/315;  // 3.3f
        //static const int cutoff = 208;
        static const int cutoff = static_cast<int>(2.2f*2048*11/315);
        static const int chromaCutoff = static_cast<int>(2048.0*2.0/91)*0.75;  // *0.5;

        Array<SRGB> outputb0(2048);
        Array<SRGB> outputb1(2048);

        //Array<float> amplitudes(2048*253);

        Array<Complex<float>> amplitudes(253);
        for (int y = 0; y < 253; ++y)
            amplitudes[y] = 0;

        //for (int x = 0; x < 253; ++x)
        //    if (preamp[x] < 50 || x < 10)
        //        preamp[x] = 50;

        Array<Vector3<float>> frameCache(3*2048*253);
        for (int i = 0; i < 3*2048*253; ++i)
            frameCache[i] = Vector3<float>(0, 0, 0);

        int frame = 0;
        Complex<float> burstAcc = 0;
        Complex<float> framePhase = unit((90 - 33)/360.0f);
        while (nn > 0) {
            inputHandle.read(&input[0], min(nn, samplesPerFrame));
            nn -= samplesPerFrame;

            Complex<float> linePhase;
            if ((frame & 1) == 0)
                linePhase = framePhase * Complex<float>(0, 1);
            else
                linePhase = framePhase;

            int p = 1824*2 + 1820;
            int x;
            int lows = 0;
            for (x = 0; x < 1820; ++x) {
                if (input[p] < 22)
                    ++lows;
                if (lows > 20)
                    break;
                ++p;
                if (p >= samplesPerFrame - 1)
                    break;
            }
            bool weirdField = x < 1000;
            p += 133 - 20;
            p -= 1820 - 10;
            if (weirdField) {
                printf("%i",frame);
                p += 1820;
            }

            for (int y = 3; y < 253; ++y) {
                float pFrac;
                p += 1820 - 10;
                for (x = 0; x < 20; ++x) {
                    if (input[p] >= 22)
                        break;
                    ++p;
                    if (p >= samplesPerFrame - 1)
                        break;
                }
                int d = input[p] - input[p - 1];
                if (d == 0)
                    pFrac = 0;
                else
                    pFrac = static_cast<float>(input[p] - 22)/d;
                // 22 position is input[p - pFrac]
                    
                for (x = 0; x < 1820; ++x) {
                    if (p + x - 64 >= samplesPerFrame)
                        break;
                    fftData[x] = input[p + x - 64];
                }
                for (; x < 2048; ++x)
                    fftData[x] = 0;

                // TODO: shift by pFrac

                Complex<float> burst = 0;
                int nBurst = (y & 1 ? 46 : 45);
                for (int x = 0; x < nBurst; ++x)
                    burst += fftData[x + 76]*unit(x*2.0f/91);
                burst /= nBurst;
                burstAcc = burstAcc*0.9f + burst*0.1f;

                Complex<float> localOscillator = 1;
                Complex<float> chromaOscillator = 1;

                float total = 0;
                for (int i = 0; i < 1666; ++i) {
                    float v = fftData[i + 82].x;
                    chromaData[i + 82] = v * chromaOscillator;
                    total += v;
                }
                float mean = total/1666;
                for (int i = 0; i < 160; ++i)
                    chromaData[i] = mean;
                for (int i = 1666+82; i < 2048; ++i)
                    chromaData[i] = mean;


                for (int i = 0; i < 2048; ++i) {
                    chromaData[i] *= chromaOscillator;
                    fftData[i] *= localOscillator;
                    localOscillator *= localOscillatorFrequency;
                    chromaOscillator *= chromaOscillatorFrequency;
                }

                fftwf_execute(forward);
                fftwf_execute(chromaForward);
                for (int x = cutoff; x < 2048 - cutoff; ++x)
                    fftData[x] = 0;
                for (int x = chromaCutoff; x < 2048 - chromaCutoff; ++x)
                    chromaData[x] = 0;
                fftwf_execute(chromaBackward);

                //for (int x = 0; x < 206; ++x) {
                //    fftData[x + 2] *= 50.0f/preamp[x];
                //    fftData[2048 - (x + 2)] *= 50.0f/preamp[x];
                //}

                fftwf_execute(backward);
                //float lastPhase = 0;
                //for (int x = 0; x < 2048; ++x) {
                //    float phase = fftData[x].argument() / static_cast<float>(tau);
                //    double deltaPhase = 1.5 + phase - lastPhase;
                //    int deltaPhaseInt = static_cast<int>(deltaPhase);
                //    deltaPhase -= deltaPhaseInt;
                //    lastPhase = phase;
                //    deltaPhase -= 0.51f;
                //    deltaPhase *= 57.0f/2;
                //    deltaPhase += 0.5f;

                //    fftData[x] = 255 - 255*deltaPhase;
                //}
                for (int x = 0; x < 2047; ++x) {
                    float deltaPhase = (fftData[x + 1]/fftData[x]).argument();
                    // 0  MHz                     <=> 0 deltaPhase
                    // 3.4MHz <=> -40   IRE                             1 MHz
                    //              7.5 IRE <=>   0
                    // 4.4MHz <=> 100   IRE <=> 255                     0 MHz
                    // 315/11MHz                  <=> tau deltaPhase
                    float mhz = deltaPhase*315/(tau*11);
                    float ire = (1.0f - mhz)*140 - 40;
                    fftData[x] = (ire - 7.5f)*255.0f/(100 - 7.5);
                    //fftData[x] = (deltaPhase + tau/2.0f)*255.0f/tau;
                }


                //fftwf_execute(forward);
                //for (int x = 0; x < 206; ++x) {
                //    fftData[x + 2] *= 50.0f/(preamp[x]*2048.0f);
                //    fftData[2048 - (x + 2)] *= 50.0f/(preamp[x]*2048.0f);
                //}
                //fftwf_execute(backward);

                Complex<float> chromaPhase = burstAcc.conjugate()*linePhase/burstAcc.modulus();

                for (int x = 0; x < 2048; ++x) {
                    //outputb[x] = byteClamp(fftData[x].x / 2048.0f);
                    float yy = fftData[x].x;
                    Complex<float> c = chromaData[x]*chromaPhase / 20.48f; //  / 100.0f;
                    float ii = c.x;
                    float qq = c.y;
                    frameCache[(((frame + 2)%3)*253 + y)*2048 + x] =
                        Vector3<float>(
                            yy + 0.9563*c.x + 0.6210*c.y,
                            yy - 0.2721*c.x - 0.6474*c.y,
                            yy - 1.1069*c.x + 1.7046*c.y);
                }
                if ((frame & 1) != 0)
                    linePhase *= Complex<float>(0, -1);
                else
                    linePhase *= Complex<float>(0, 1);

                //if (frame >= 1041 && frame <= 1638) {
                //    fftwf_execute(forward);
                //    //for (int x = 0; x < 2048; ++x)
                //    //    amplitudes[y*2048 + x] = fftData[x].modulus() / 2048.0f;
                //    amplitudes[y] += fftData[y + 2];
                //}

                //h.write(outputb);
            }
            printf(".");

#if 1
            for (int y = 3; y < 253; ++y) {
                int p = (((frame + 1)%3)*253 + y)*2048;
                int pu = (((frame + 1)%3)*253 + y - 1)*2048;
                if (y == 0)
                    pu = p;
                int pd = (((frame + 1)%3)*253 + y + 1)*2048;
                if (y == 252)
                    pd = p;
                int pp = ((frame%3)*253 + y)*2048;
                int pn = (((frame + 2)%3)*253 + y)*2048;

                for (int x = 0; x < 2048; ++x) {
                    Vector3<float> c = frameCache[p + x];
                    outputb0[x] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
                    c = (frameCache[pu + x] + frameCache[pd + x] + frameCache[pp + x] + frameCache[pn + x])/4.0f;
                    outputb1[x] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
                }

                if (frame%2 != 0) {
                    h.write(outputb0);
                    h.write(outputb1);
                }
                else {
                    h.write(outputb1);
                    h.write(outputb0);
                }
            }
#else
            for (int y = 3; y < 253; ++y) {
                int p = (((frame + 2)%3)*253 + y)*2048;
                for (int x = 0; x < 2048; ++x) {
                    Vector3<float> c = frameCache[p + x];
                    outputb0[x] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
                }
                h.write(outputb0);
            }
#endif

            //for (int y = 0; y < 253; ++y) {
            //    int offset0 = scanlineOffsets[(frame + 7)%8];
            //    int offset1 = scanlineOffsets[frame%8];
            //    int y0 = clamp(0, y + offset0, 252);
            //    int y1 = clamp(0, y + offset1, 252);
            //    int y2 = clamp(0, y + offset0 + 1, 252);
            //    int y3 = clamp(0, y + offset1 + 1, 252);
            //    int p0 = (((frame + 1)%3)*253 + y0)*2048;
            //    int p1 = (((frame + 2)%3)*253 + y1)*2048;
            //    int p2 = (((frame + 1)%3)*253 + y2)*2048;
            //    int p3 = (((frame + 2)%3)*253 + y3)*2048;

            //    if (frame%2 == 0)
            //        for (int x = 0; x < 2048; ++x) {
            //            outputb0[x] = byteClamp((frameCache[p0 + x] + frameCache[p1 + x])/2.0f);
            //            outputb1[x] = byteClamp((frameCache[p2 + x] + frameCache[p1 + x])/2.0f);
            //        }
            //    else
            //        for (int x = 0; x < 2048; ++x) {
            //            outputb0[x] = byteClamp((frameCache[p0 + x] + frameCache[p1 + x])/2.0f);
            //            outputb1[x] = byteClamp((frameCache[p3 + x] + frameCache[p1 + x])/2.0f);
            //        }

            //    h.write(outputb0);
            //    h.write(outputb1);
            //}

            //if (frame >= 1041 && frame <= 1638) {
            //    int bestOffset;
            //    float bestAmplitude = 0;
            //    for (int offset = -10; offset < 10; ++offset) {
            //        float amplitude = 0;
            //        for (int y = 0; y < 253; ++y)
            //            if (y+offset >= 0)
            //                amplitude += amplitudes[y*2048 + y + offset];
            //        if (amplitude > bestAmplitude) {
            //            bestAmplitude = amplitude;
            //            bestOffset = offset;
            //        }
            //    }
            //    printf("frame %i strongest offset %i amplitude %f\n", frame, bestOffset);
            //}

            ++frame;

            if ((frame & 1) == 0)
                framePhase *= Complex<float>(0, 1);

        }
        //for (int y = 0; y < 253; ++y)
        //    printf("  %f, %f,\n", amplitudes[y].x / (1639 - 1041), amplitudes[y].y / (1639 - 1041));
    }
};