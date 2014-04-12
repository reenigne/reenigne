#include "alfe/main.h"
#include "alfe/vectors.h"

class Program : public ProgramBase
{
public:
    int colour(Vector p)
    {
        int palette[16] =
            { 0, 8, 9, 1, 5, 13, 12, 4, 6, 14, 15, 7, 3, 11, 10, 2 };
        double d = /*sqrt(*/static_cast<double>(p.modulus2())/64 /*)*/;
        return palette[static_cast<int>(d) & 15];
    }
    void run()
    {
        int stride = 80;
        _screenSize = Vector(80, 50);
        int frames = 13125000 * 14 / (11 * 76 * 262);
        int maxRadius = 20;

        _o1s.allocate(frames);
        _o2s.allocate(frames);
        int minO = 0, maxO = 0;
        for (int t = 0; t < frames; ++t) {
            double f = static_cast<double>(t) / frames;
            double r = maxRadius; // *(1 - cos(f * tau)) / 2;
            Rotor2<double> z1(f * 3);
            Rotor2<double> z2(f * 4);
            Vector2<double> a1 = Vector2<double>(r, 0) * z1;
            Vector2<double> a2 = Vector2<double>(r, 0) * z2;

            Vector2<double> c = Vector2Cast<double>(_screenSize) / 2;

            // Positions of picture centres relative to screen top-left
            Vector2<double> p1 = c + a1;
            Vector2<double> p2 = c + a2;

            // Positions of screen top-left on relative to pictures center
            Vector s1 = -Vector2Cast<int>(p1);
            Vector s2 = -Vector2Cast<int>(p2);

            // Offsets of screen top-left into pictures relative to pictures
            // center.
            int o1 = s1.y*stride + s1.x;
            int o2 = s2.y*stride + s2.x;

            // Offset in picture from start of screen to end
            int ss = (_screenSize.y - 1)*stride + _screenSize.x;

            int o1e = o1 + ss;
            int o2e = o2 + ss;

            // Picture bounds
            minO = min(min(minO, o1), o2);
            maxO = max(max(maxO, o1e), o2e);

            _o1s[t] = o1;
            _o2s[t] = o2;
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

        output.write("frames: dw " + decimal(frames) + "\n\n");

        output.write("motion:");
        for (int t = 0; t < frames; ++t) {
            int o1 = _o1s[t] - minO;
            int o2 = _o2s[t] - minO;

            int si = o1/2;
            if ((o1 & 1) != 0)
                si += bytes;
            if ((o2 & 1) != 0)
                si |= 0x8000;
            if ((o2 & 2) != 0)
                si |= 0x4000;

            int ip = (o2 / 4) * 10;

            if (t % 4 == 0)
                output.write("\n  dw ");
            else
                output.write(", ");
            output.write(hex(si, 4) + ", " + hex(ip, 4));
        }

        int lastX = 20;
        output.write("\n\n");
        output.write("picture:");
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
private:
    Vector _screenSize;
    Array<int> _o1s;
    Array<int> _o2s;
};
