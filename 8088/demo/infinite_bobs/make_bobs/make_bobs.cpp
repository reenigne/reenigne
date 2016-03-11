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

        AppendableArray<Byte> code;
        AppendableArray<Byte> data;
        AppendableArray<Word> dataPointers;
        AppendableArray<int> codePatches;

        FileStream output = File(_arguments[2], true).openWrite();
        output.write(inputCode);

        int positions = 2;

        offset += inputCode.length();
        offset += positions*sizeof(Word);

        for (int x0 = 0; x0 < positions; ++x0) {
            output.write<Word>(offset + code.count());
            dataPointers.append(data.count());
            code.append(0xbe);
            codePatches.append(code.count());
            code.append(0);
            code.append(0);

            int di = 0;
            for (int y = 0; y < yr*2 + 1; ++y) {
                double yy = (y - (yr-1))/yr;
                double y2 = yy*yy;

                int bytes = 0;
                bool extraNybble = false;
                int last = 0;
                for (int x = 0; x < xr*2 + 1; ++x) {
                    double xx = (x - (xr - 1))/xr;
                    double x2 = xx*xx;
                    double z2 = 1 - (x2 + y2);
                    bool started = false;
                    if (z2 >= 0) {
                        double zz = sqrt(z2);
                        double l = xx*lx + yy*ly + zz*lz;
                        double l1 = (l + n)/(1 + n);
                        int c = clamp(0, static_cast<int>(l1*(colours + 1)),
                            colours - 1);
                        int f = c >> 4;
                        if ((c & 15) > bayer[(y & 3)*4 + (x & 3)])
                            ++f;
                        if (!started) {
                            started = true;
                            if (y != 0)
                                code.append(80 + ((x + x0)>>1) - di);
                            if (((x+x0) & 1) != 0) {
                                code.append(0x26); code.append(0x8a);
                                code.append(0x05);
                                code.append(0x24); code.append(0xf0);
                                code.append(0x0c); code.append(fade[f]);
                                code.append(0xaa);
                                continue;
                            }
                        }
                        if (((x+x0) & 1) != 0) {
                            data.append((last << 4) | fade[f]);
                            ++bytes;
                            extraNybble = false;
                        }
                        else {
                            last = fade[f];
                            extraNybble = true;
                        }
                        di = 1 + ((x + x0)>>1);
                    }
                }
                if (bytes > 1) {
                    code.append(0xb9); code.append(bytes >> 1);
                    code.append(bytes >> 9);
                    code.append(0xf3); code.append(0xa5);
                }
                if ((bytes & 1) != 0)
                    code.append(0xa4);
                if (extraNybble) {
                    code.append(0x26); code.append(0x8a); code.append(0x05);
                    code.append(0x24); code.append(0x0f);
                    code.append(0x0c); code.append(last << 4);
                    code.append(0xaa);
                }
                if (y < yr*2) {
                    code.append(0x83); code.append(0xc7);
                }
            }
            code.append(0xc3);
        }
        offset += code.count();
        for (int x0 = 0; x0 < positions; ++x0) {
            Word dataPointer = offset + dataPointers[x0];
            int patch = codePatches[x0];
            code[patch] = dataPointer & 0xff;
            code[patch + 1] = dataPointer >> 8;
        }

        output.write(code);
        output.write(data);
    }
};