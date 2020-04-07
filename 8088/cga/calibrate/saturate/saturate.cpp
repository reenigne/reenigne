#include "alfe/main.h"
#include "alfe/complex.h"

class Program : public ProgramBase
{
public:
    void integrate(Byte rgbi0, Byte rgbi1, Byte rgbi2, Byte rgbi3, double* dc, Complex<double>* iq/*, double* hf*/)
    {
        double s0 = _tSamples[(rgbi0 << 6) | (rgbi1 << 2)];
        double s1 = _tSamples[(rgbi1 << 6) | (rgbi2 << 2) | 1];
        double s2 = _tSamples[(rgbi2 << 6) | (rgbi3 << 2) | 2];
        double s3 = _tSamples[(rgbi3 << 6) | (rgbi0 << 2) | 3];

        *dc = (s0 + s1 + s2 + s3)/4;
        iq->x = (s0 - s2)/2;
        iq->y = (s1 - s3)/2;
//        *hf = ((s0 + s2) - (s1 + s3))/4;
    }

    Colour generate(Byte rgbi0, Byte rgbi1, Byte rgbi2, Byte rgbi3)
    {
        double dc;
        Complex<double> iq;
        integrate(rgbi0, rgbi1, rgbi2, rgbi3, &dc, &iq);

        double y = dc*_contrast + _brightness;
        iq *= _iqAdjust;

        double r = 256*(y + 0.9563*iq.x + 0.6210*iq.y);
        double g = 256*(y - 0.2721*iq.x - 0.6474*iq.y);
        double b = 256*(y - 1.1069*iq.x + 1.7046*iq.y);
        return Colour(r, g, b);
    }

    void run()
    {
        _saturation = 0.8755;
        //_saturation = 1;
        //_saturation = 0.658;
        //_saturation = 0.561;
        _hue = 0;

        _contrast = 0.95841076343534182;
        _brightness = -0.0089814384392818136;

        Stream s = File("output.dat").openRead();
        s.read(reinterpret_cast<Byte*>(_tSamples), 1024*sizeof(double));
        for (int i = 0; i < 1024; ++i)
            _tSamples[i] = _tSamples[i]*_contrast + _brightness;

        _contrast = 1;
        _brightness = 0;

        double dc;
        Complex<double> iq;
        integrate(6, 6, 6, 6, &dc, &iq);

        printf("iq = %lf, %lf, modulus = %lf, argument = %lf\n", iq.x, iq.y, iq.modulus(), iq.argument());

        _iqAdjust = -iq.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/iq.modulus();

        Colour black = generate(0, 0, 0, 0);
        printf("black = %lf, %lf, %lf\n", black.x, black.y, black.z);
        double avgBlack = black.x*0.299 + black.y*0.587 + black.z*0.114;
        Colour white = generate(15, 15, 15, 15);
        printf("white = %lf, %lf, %lf\n", white.x, white.y, white.z);
        double avgWhite = white.x*0.299 + white.y*0.587 + white.z*0.114;

        //// Adjust brightness and contrast so that black is 38 and white is 203
        //_contrast = (203 - 38)/(avgWhite - avgBlack);
        //_brightness = (38 - _contrast*avgBlack)/256.0;

        printf("unsigned char oldCGA[1024] = {\n");
        for (int i = 0; i < 1024; ++i) {
            if ((i & 15) == 0)
                printf("    ");
            printf("%3i", static_cast<int>(_tSamples[i]*256.0));
            if (i != 1024)
                printf(",");
            if ((i & 15) == 15)
                printf("\n");
            else
                if ((i & 3) == 3)
                    printf(" ");
        }
        printf("};\n");

        double minR = 255, maxR = 0, minG = 255, maxG = 0, minB = 255, maxB = 0;
        int minRc, maxRc, minGc, maxGc, minBc, maxBc;
        int clips = 0;
        for (int c = 0; c < 65536; ++c) {
            int rgbi0 = c >> 12;
            int rgbi1 = (c >> 8) & 15;
            int rgbi2 = (c >> 4) & 15;
            int rgbi3 = c & 15;
            //if (rgbi0 != rgbi1) {
            //    if (rgbi0 != rgbi2 || rgbi0 != rgbi3)
            //        continue;
            //}
            //else {
            //    if (rgbi1 != rgbi2)
            //        if (rgbi1 != rgbi3)
            //            continue;
            //}

            Colour cc = generate(rgbi0, rgbi1, rgbi2, rgbi3);
            if (cc.x < minR) {
                minR = cc.x;
                minRc = c;
            }
            if (cc.x > maxR) {
                maxR = cc.x;
                maxRc = c;
            }
            if (cc.y < minG) {
                minG = cc.y;
                minGc = c;
            }
            if (cc.y > maxG) {
                maxG = cc.y;
                maxGc = c;
            }
            if (cc.z < minB) {
                minB = cc.z;
                minBc = c;
            }
            if (cc.z > maxB) {
                maxB = cc.z;
                maxBc = c;
            }
            if (cc.x < 0 || cc.y < 0 || cc.z < 0 || cc.x >= 256 || cc.y >= 256 || cc.z >= 256)
                ++clips;

        }
        printf("clips = %i\n", clips);
        printf("min R %lf at %04x\n", minR, minRc);
        printf("max R %lf at %04x\n", maxR, maxRc);
        printf("min G %lf at %04x\n", minG, minGc);
        printf("max G %lf at %04x\n", maxG, maxGc);
        printf("min B %lf at %04x\n", minB, minBc);
        printf("max B %lf at %04x\n", maxB, maxBc);
    }

    double _tSamples[1024];
    Complex<double> _iqAdjust;
    double _saturation;
    double _hue;
    double _brightness;
    double _contrast;
};
