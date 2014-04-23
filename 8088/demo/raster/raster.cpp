#include "alfe/main.h"
#include <stdio.h>

//static const int nBars = 10;
//static const int barWidth = 7;
//int barColours[barWidth] = {4, 6, 14, 15, 11, 9, 1};
int palette[16 * 3] = {
    0x00, 0x00, 0x00,
    0x00, 0x00, 0xaa,
    0x00, 0xaa, 0x00,
    0x00, 0xaa, 0xaa,
    0xaa, 0x00, 0x00,
    0xaa, 0x00, 0xaa,
    0xaa, 0x55, 0x00,
    0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55,
    0x55, 0x55, 0xff,
    0x55, 0xff, 0x55,
    0x55, 0xff, 0xff,
    0xff, 0x55, 0x55,
    0xff, 0x55, 0xff,
    0xff, 0xff, 0x55,
    0xff, 0xff, 0xff };

int baseColours[3*3] = { 1, 9, 15, 2, 10, 15, 4, 12, 15 };

//int bayer[4] = { 0, 2, 1, 3 };
int bayer[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

class Program : public ProgramBase
{
public:
    void run()
    {
        FILE* out = fopen("raster.raw", "wb");
        int radius = 100;
        for (int base = 0; base < 3; ++base)
            for (int y = 0; y < 2*radius + 1; ++y) {
                Colour ambient = fromIndex(baseColours[base*3]);
                Colour diffuse = fromIndex(baseColours[base*3 + 1]) - ambient;
                Colour specular = fromIndex(baseColours[base*3 + 2]) - (ambient + diffuse);
                Vector3<double> l = Vector3<double>(0, 1, 1).normalized();
                Vector3<double> v = Vector3<double>(0, 0, 1).normalized();
                Vector3<double> h = (l + v).normalized();

                Vector3<double> p(0, radius - y, 0);
                double z2 = radius*radius;
                if (z2 < 0)
                    continue;
                p.z = sqrt(z2);
                p.normalize();

                double lDotN = dot(p, l);
                Colour c = ambient;
                if (lDotN > 0)
                    c += diffuse*lDotN + specular*pow(dot(h, p), 10);

                int bestPair = 0;
                double bestD2 = DBL_MAX;
                Colour cCGA;
                Colour cCGA1;
                Colour cCGA2;
                Colour cClosest;
                for (int c1 = 0; c1 < 16; ++c1)
                    for (int c2 = 0; c2 < c1; ++c2) {
                        Colour c1f = fromIndex(c1);
                        Colour c2f = fromIndex(c2);
                        Colour n = (c2f - c1f).normalized();
                        double t = dot(n, c - c1f);
                        //  c == c1f => t == 0
                        //  c == c2f => t == (c1f - c2f).modulus()
                        if (t < 0)
                            t = 0;
                        double e = (c2f - c1f).modulus();
                        if (t > e)
                            t = e;
                        Colour w = t*n;
                        Colour closest = c1f + w;
                        Colour d = closest - c;
                        double d2 = d.modulus2();
                        if (d2 < bestD2) {
                            bestD2 = d2;
                            cCGA1 = c1f;
                            cCGA2 = c2f;
                            cClosest = closest;
                            if (static_cast<int>(9*t/e) > bayer[y&7])
                                cCGA = cCGA2;
                            else
                                cCGA = cCGA1;
                        }

                    }

                for (int x = 0; x < 160; ++x) {
                    SRGB srgb = ColourSpace::srgb().toSrgb24(c);
                    fputc(srgb.x, out);
                    fputc(srgb.y, out);
                    fputc(srgb.z, out);
                }
                for (int x = 0; x < 160; ++x) {
                    SRGB srgb = ColourSpace::srgb().toSrgb24(cCGA1);
                    fputc(srgb.x, out);
                    fputc(srgb.y, out);
                    fputc(srgb.z, out);
                }
                for (int x = 0; x < 160; ++x) {
                    SRGB srgb = ColourSpace::srgb().toSrgb24(cCGA2);
                    fputc(srgb.x, out);
                    fputc(srgb.y, out);
                    fputc(srgb.z, out);
                }
                for (int x = 0; x < 160; ++x) {
                    SRGB srgb = ColourSpace::srgb().toSrgb24(cClosest);
                    fputc(srgb.x, out);
                    fputc(srgb.y, out);
                    fputc(srgb.z, out);
                }
                for (int x = 0; x < 160; ++x) {
                    SRGB srgb = ColourSpace::srgb().toSrgb24(cCGA);
                    fputc(srgb.x, out);
                    fputc(srgb.y, out);
                    fputc(srgb.z, out);
                }
            }
        fclose(out);
    }
private:
    Colour fromIndex(int i)
    {
        return Colour(palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]);
    }
};
