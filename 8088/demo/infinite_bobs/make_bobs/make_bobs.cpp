#include "alfe/main.h"

int fade[] = {4, 12, 13, 15, 15};

int bayer[] = {
    0,  8,  2, 10,
   12,  4, 14,  6,
    3, 11,  1,  9,
   15,  7, 13,  5};

//  left 8 yellow
//       4 red
//       2 blue
// right 1 green

class Program : public ProgramBase
{
public:
    void run()
    {
        String inputCodeFilename = _arguments[1];
        String inputCode = File(inputCodeFilename, true).contents();
        int inputCodeLength = inputCode.length();
        int offset = 0;
        String extension = inputCodeFilename.
            subString(inputCodeFilename.length() - 4, 4);
        if (extension[0] == '.' && (extension[1] == 'c' || extension[1] == 'C')
            && (extension[1] == 'o' || extension[1] == 'O')
            && (extension[1] == 'm' || extension[1] == 'M')) {
            offset = 0x100;
        }

        double yr = 20;
        double xr = 6*yr/5;

        double l = sqrt(1/3.0);
        double lx = -l;
        double ly = -l;
        double lz = l;

        // Maximum l is 1
        // Minimum l is -2*sqrt(1/2)*sqrt(1/3) = -2*sqrt(1/6) = -sqrt(2/3)
        double n = sqrt(2/3.0);

        int colours = 3*16 + 1;

        Array<int> cc(160);
        AppendableArray<Byte> code;
        AppendableArray<Byte> data;

        FileStream output = File(_arguments[2], true).openWrite();
        output.write(inputCode);

        int positions = 2;

        offset += 2*sizeof(Word);

        for (int x0 = 0; x0 < positions; ++x0) {
            output.write<Word>(offset + 4 + code.count());
            code.append(0xbe);
            code.append(0);
            code.append(0);

            for (int y = 0; y < yr*2 + 1; ++y) {
                double yy = (y - (yr-1))/yr;
                double y2 = yy*yy;

                for (int x = 0; x < xr*2 + 1; ++x) {
                    double xx = (x - (xr - 1))/xr;
                    double x2 = xx*xx;
                    double z2 = 1 - (x2 + y2);
                    bool started = false;
                    int last = 0;
                    if (z2 >= 0) {
                        double zz = sqrt(z2);
                        double l = xx*lx + yy*ly + zz*lz;
                        double l1 = (l + n)/(1 + n);
                        int c = clamp(0, static_cast<int>(l1*(colours + 1)),
                            colours - 1);
                        int f = c >> 4;
                        if ((c & 15) > bayer[(y & 3)*4 + (x & 3)])
                            ++f;
                        cc[x] = fade[f];
                        if (!started) {
                            started = true;
                            if (((x+x0) & 1) != 0) {
                                code.append(0x26); code.append(0x8a);
                                code.append(0x05);
                                code.append(0x24); code.append(0xf0);
                                code.append(0x0c);
                                code.append(fade[f]);
                                code.append(0xaa);
                                continue;
                            }
                        }
                        if (((x+x0) & 1) != 0)
                            data.append((last << 4) | fade[f]);
                        else
                            last = fade[f];
                    }
                }
                code.append(0xb9);
                code.append()

            }
        }

        output.write(code);
        output.write(data);
    }
};