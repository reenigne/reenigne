#include "alfe/main.h"
#include <stdio.h>
#include "alfe/colour_space.h"

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

class Bar
{
public:
    Bar(int base)
    {
        _ambient = fromIndex(baseColours[base*3]);
        _diffuse = fromIndex(baseColours[base*3 + 1]) - _ambient;
        _specular = fromIndex(baseColours[base*3 + 2]) - (_ambient + _diffuse);
        _radius = 12;
    }
    SRGB getColour(int y)
    {
        Vector3<double> l = Vector3<double>(0, 1, 1).normalized();
        Vector3<double> v = Vector3<double>(0, 0, 1).normalized();
        Vector3<double> h = (l + v).normalized();

        Vector3<double> p(0, -y, 0);
        double z2 = _radius*_radius;
        if (z2 < 0)
            return SRGB(0, 0, 0);
        p.z = sqrt(z2);
        p.normalize();

        double lDotN = dot(p, l);
        Colour c = _ambient;
        if (lDotN > 0)
            c += _diffuse*lDotN + _specular*pow(dot(h, p), 100);

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
                Colour diff = c2f - c1f;
                double weight = diff.modulus2();
                Colour n = diff.normalized();
                double t = dot(n, c - c1f);
                if (t < 0)
                    t = 0;
                double e = (c2f - c1f).modulus();
                if (t > e)
                    t = e;
                Colour w = t*n;
                Colour closest = c1f + w;
                Colour d = closest - c;
                double d2 = d.modulus2()*weight;
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
        return ColourSpace::srgb().toSrgb24(cCGA);
    }
    Colour _ambient;
    Colour _diffuse;
    Colour _specular;
    int _radius;
    int _y;
private:
    Colour fromIndex(int i)
    {
        return Colour(palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]);
    }
};

class Program : public ProgramBase
{
public:
    void run()
    {
        FILE* out = fopen("raster.raw", "wb");

        Bar red(2);
        Bar green(1);
        Bar blue(0);
        Array<Bar*> bars(3);
        bars[0] = &red;
        bars[1] = &green;
        bars[2] = &blue;
        Array<Colour> x(200*838);
        for (int y = 0; y < 200*838; ++y)
            x[y] = Colour(128, 128, 128);
        double decay = 1.0;
        Colour waterColours[3] =
            { Colour(1, 1, 0), Colour(0, 1, 1), Colour(1, 0, 1)};
        console.write("Starting\n");
        for (int tt = 0;; ++tt) {
            double a = (tt*2 + 1)/12.0;
            double te = a*838/14;
            if (tt >= 1) //te >= 838)
                break;
            double ye = 100 + (100 - red._radius)*sin((a + tt/3.0)*tau);
            int t0 = static_cast<int>(te+1);
            for (int t1 = 0; t1 < 838; ++t1) {
                int t2 = (t0 + t1)%838;
                double tp = (t0 + t1 - te)*0.01; // *(100 - red._radius)*tau*14.0/838;
                double dp = exp(-decay*tp);
                for (int n = 1; n < 20; ++n) {
                    double p = cos(n*ye*tau/(2*200));
                    double tdp = dp*cos(sqrt(n*n-decay*decay)*tp)*p;
                    Colour c = 20.0*waterColours[tt%3]*tdp;
                    double pp = n*tau/(2*200);
                    for (int y = 0; y < 200; ++y)
                        x[t2*200+y] += c*cos(y*pp);
                }
            }
            console.write(".");
        }
        console.write("Water complete\n");

        for (int t = 0; t < 838; ++t) {
            Array<SRGB> column(200);
            for (int y = 0; y < 200; ++y)
                column[y] = ColourSpace::srgb().toSrgb24(x[y+t*200]);
            for (int base = 0; base < 3; ++base) {
                double a = t*14.0/838;
                Bar* bar = bars[base];
                bar->_y = 100 + (100 - bar->_radius)*sin((a + base/3.0)*tau);
            }
            for (int base = 0; base < 3; ++base) {
                double a = t*14.0/838;
                Bar* bar = bars[(base + static_cast<int>(a*6))%3];
                for (int y = -bar->_radius; y <= bar->_radius; ++y) {
                    int yy = y + bar->_y;
                    if (yy >= 0 && yy < 200)
                        column[yy] = bar->getColour(y);
                }
            }
            for (int y = 0; y < 200; ++y) {
                SRGB srgb = column[y];
                fputc(srgb.x, out);
                fputc(srgb.y, out);
                fputc(srgb.z, out);
            }
        }
        fclose(out);
        console.write("Bars complete\n");
    }
};


//for (int x = 0; x < 160; ++x) {
//    SRGB srgb = ColourSpace::srgb().toSrgb24(c);
//    fputc(srgb.x, out);
//    fputc(srgb.y, out);
//    fputc(srgb.z, out);
//}
//for (int x = 0; x < 160; ++x) {
//    SRGB srgb = ColourSpace::srgb().toSrgb24(cCGA1);
//    fputc(srgb.x, out);
//    fputc(srgb.y, out);
//    fputc(srgb.z, out);
//}
//for (int x = 0; x < 160; ++x) {
//    SRGB srgb = ColourSpace::srgb().toSrgb24(cCGA2);
//    fputc(srgb.x, out);
//    fputc(srgb.y, out);
//    fputc(srgb.z, out);
//}
//for (int x = 0; x < 160; ++x) {
//    SRGB srgb = ColourSpace::srgb().toSrgb24(cClosest);
//    fputc(srgb.x, out);
//    fputc(srgb.y, out);
//    fputc(srgb.z, out);
//}
//for (int x = 0; x < 160; ++x) {
//    SRGB srgb = ColourSpace::srgb().toSrgb24(cCGA);
//    fputc(srgb.x, out);
//    fputc(srgb.y, out);
//    fputc(srgb.z, out);
//}
