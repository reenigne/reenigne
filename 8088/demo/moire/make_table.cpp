#include "alfe/main.h"
#include "alfe/vectors.h"

class Program : public ProgramBase
{
public:
    int colour(Vector p)
    {
        int palette[16] =
            //            { 0, 8, 9, 1, 5, 13, 12, 4, 6, 14, 15, 7, 3, 11, 10, 2 };
            { 0, 1, 5, 4, 6, 7, 3, 2, 10, 11, 15, 14, 12, 13, 9, 8 };
        double d = sqrt(static_cast<double>(p.modulus2()));
        return palette[static_cast<int>(d) & 15];
    }
    //int colour(Vector p)
    //{
    //    int t = static_cast<int>(17 * 6 * atan2(p.y, p.x) / tau + 17 * 6);
    //    int bayer[16] = {
    //         0,  8,  2, 10,
    //        12,  4, 14,  6,
    //         3, 11,  1,  9,
    //        15,  7, 13,  5 };
    //    int palette[6] = { 12, 14, 10, 11, 9, 13 };
    //    int c1 = (t / 17) % 6;
    //    int c2 = (c1 + 1) % 6;
    //    int dither = t % 17;
    //    int xd = p.x & 3;
    //    int yd = p.y & 3;
    //    int b = bayer[yd * 4 + xd];
    //    //return palette[dither > b ? c2 : c1];
    //    return palette[c2];
    //}

    void run()
    {
        Vector screenSize(80, 50);
        int frames = 13125000 * 14 / (11 * 76 * 262);
        int maxRadius = 20;

        // Center of screen
        Vector2<double> c = Vector2Cast<double>(screenSize) / 2;

        // Positions of screen top-left relative to centre of each picture
        Array<Vector> p1s(frames);
        Array<Vector> p2s(frames);
        int minX = 0, maxX = 0;
        for (int t = 0; t < frames; ++t) {
            double f = static_cast<double>(t) / frames;
            double r = maxRadius; // *(1 - cos(f * tau)) / 2;
            Rotor2<double> z1(f * 6);
            Rotor2<double> z2(f * 7);
            Vector2<double> a1(r*cos(f*tau * 6), r*sin(f*tau * 7));
            Vector2<double> a2(r*cos(f*tau * 4), r*sin(f*tau * 5));

            // Positions of picture centres relative to screen top-left
            Vector p1 = -Vector2Cast<int>(c + a1);
            Vector p2 = -Vector2Cast<int>(c + a2);
            p1s[t] = p1;
            p2s[t] = p2;

            minX = min(min(minX, p1.x), p2.x);
            maxX = max(max(maxX, p1.x + screenSize.x), p2.x + screenSize.x);
        }
        int stride = (3 + maxX - minX) & ~3;

        // Offset in picture from start of screen to end
        int ss = (screenSize.y - 1)*stride + screenSize.x;

        Array<int> o1s(frames);
        Array<int> o2s(frames);
        int minO = 0, maxO = 0;
        for (int t = 0; t < frames; ++t) {
            Vector p1 = p1s[t];
            Vector p2 = p2s[t];

            // Offsets of screen top-left into pictures relative to pictures
            // center.
            int o1 = p1.y*stride + p1.x;
            int o2 = p2.y*stride + p2.x;

            int o1e = o1 + ss;
            int o2e = o2 + ss;

            // Picture bounds
            minO = min(min(minO, o1), o2);
            maxO = max(max(maxO, o1e), o2e);

            o1s[t] = o1;
            o2s[t] = o2;
        }

        FileHandle output = File("tables.asm").openWrite();
/*        output.write("cpu 8086\n"
            "segment _DATA public class = DATA\n"
            "\n"
            "global _picture, _motion\n"
            "\n"
            "\n"); */

        int d = ((-minO) / stride + 1)*stride + stride/2;

        int bytes = (maxO + 1 - minO) / 2;

        int xs = (minO + d) % stride;
        int ys = (minO - xs) / stride;
        console.write("First position: (" + decimal(xs) + ", " + decimal(ys) +
            ")\n");
        xs = (maxO + d) % stride;
        ys = (maxO - xs) / stride;
        console.write("Last position: (" + decimal(xs) + ", " + decimal(ys) +
            ")\n");
        console.write("Picture size: " + decimal(bytes) + "\n");
        console.write("Motion size: " + decimal(4 * frames) + "\n");

        output.write("frames equ " + decimal(frames) + "\n");
        output.write("stride equ " + decimal(stride/2) + "\n");
        output.write("p equ picture\n");
        output.write(
            "p2 equ pictureEnd+(pictureEnd-picture)+(headerEnd-header)\n\n");

        output.write("motion:");
        for (int t = 0; t < frames; ++t) {
            int o1 = o1s[t] - minO;
            int o2 = o2s[t] - minO;

            int sp = o1 / 2;
            if ((o1 & 1) != 0)
                sp += bytes;
            int bp = o2 / 2;
            if ((o2 & 1) != 0)
                bp += bytes;

            if (t % 3 == 0)
                output.write("\n  dw ");
            else
                output.write(", ");
            output.write("p+" + hex(sp, 4) + ", p+" + hex(bp, 4));
        }

        int lastX = 20;
        output.write("\n\n");

        int p2 = (maxO + 1 - minO) / 2;
        p2 += p2 - 1;

        output.write("transition:");
        Array<bool> cleared(20 * 13);
        for (int p = 0; p < 20 * 13; ++p)
            cleared[p] = false;
        int pp = 0;
        for (int t = 0; t < 1000000; ++t) {
            int r = 999 - t / 1000;
            int theta = t % 1000;
            Vector2<double> z =
                Vector2<double>(r/20.0, 0)*Rotor2<double>(theta / 1000.0);
            Vector p = Vector2Cast<int>(z + Vector2<double>(10, 6.25));
            if (p.x >= 0 && p.x < 20 && p.y >= 0 && p.y < 13) {
                int aa = p.y * 20 + p.x;
                if (cleared[aa])
                    continue;
                int a = p.y * 206 * 4 + p.x * 10;
                if (pp % 3 == 0)
                    output.write("\n  dw ");
                else
                    output.write(", ");
                ++pp;
                output.write("p2+" + hex(a, 4) + ", ");
                if (p.y == 12)
                    output.write("p2+" + hex(a, 4));
                else
                    output.write("p2+" + hex(a + 206*2, 4));
                cleared[aa] = true;
            }
        }
        console.write("pp = " + decimal(pp) + " \n");
        output.write("\n\npicture:");
        for (int o = minO; o < maxO + 1; o += 2) {
            int x = (o + d) % stride;
            int y = (o + d - x) / stride - d/stride;

            if (lastX == 20) {
                output.write("\n  db ");
                lastX = 0;
            }
            else
                output.write(", ");
            for (; lastX < x % 20; lastX += 2)
                output.write("      ");

            Vector p(x - stride / 2, y);
            int cL = colour(p);
            int cR = colour(p + Vector(1, 0));
            int b = cL | (cR << 4);
            output.write(String(hex(b, 2)));
            lastX += 2;
        }
        output.write("\n");
        output.write("pictureEnd:\n");
    }
};
