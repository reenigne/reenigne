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
            && (extension[2] == 'o' || extension[2] == 'O')
            && (extension[3] == 'm' || extension[3] == 'M')) {
            offset = 0x100;
        }

        double yr = 20.5;
        double xr = 6*yr/5;
        int yri = static_cast<int>(yr);
        int xri = static_cast<int>(xr);

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
        AppendableArray<int> codeOffsets;
        AppendableArray<int> initialDi;

        FileStream output = File(_arguments[2], true).openWrite();
        output.write(inputCode);

        int positions = 2;

        offset += inputCode.length();
        //offset += positions*sizeof(Word);

        // Generate sprite data
        for (int x0 = 0; x0 < positions; ++x0) {
            dataPointers.append(data.count());
            codeOffsets.append(code.count());
            code.append(0xbe);
            code.append(0);
            code.append(0);

            int di = 0;
            for (int y = 0; y < yri*2 + 1; ++y) {
                double yy = (y - yri)/yr;
                double y2 = yy*yy;

                int bytes = 0;
                bool extraNybble = false;
                int last = 0;
                bool started = false;
                for (int x = 0; x < xri*2 + 1; ++x) {
                    double xx = (x - xri)/xr;
                    double x2 = xx*xx;
                    double z2 = 1 - (x2 + y2);
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
                            else
                                initialDi.append(((x + x0)>>1));
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
                if (y < yri*2) {
                    code.append(0x83); code.append(0xc7);
                }
            }
            code.append(0xc3);
        }
        int nx = 1 + tau*(80-xri);
        int ny = 1 + tau*(50-yri);
        output.write<Word>(nx);
        output.write<Word>(ny);
        int codeOffset = offset + 4 + 4*nx + 2*ny;

        // Patch sprite data positions into sprite code
        for (int x0 = 0; x0 < positions; ++x0) {
            Word dataPointer = codeOffset + code.count() + dataPointers[x0];
            int patch = codeOffsets[x0] + 1;
            code[patch] = dataPointer & 0xff;
            code[patch + 1] = dataPointer >> 8;
        }

        //X:
        //  Number of possible positions = 160-xri*2
        //  x = ((sin(k*t)+1)/2)*(160-xri*2) = sin(t*tau/N)*(80-xri)+(80-xri)
        //  then clamp and round down
        //  At fastest point (t==0), dx/dt = tau*(80-xri)/N = 1, N = tau*(80-xri)
        //  For xri = 30, 314 positions
        //Y:
        //  Number of possible positions = 100-yri*2, N = tau*(50-yri)
        //  For yri = 20, 188 positions
        //Want an integral number of positions, so round up

        // Output X sine table, DI part
        for (int t = 0; t < nx; ++t) {
            int x = xSine(t, xri);
            output.write<Word>(x/2 + initialDi[x % positions]);
        }
        // Output X sine table, sprite pointer part
        for (int t = 0; t < nx; ++t) {
            output.write<Word>(codeOffset +
                codeOffsets[xSine(t, xri) % positions]);
        }
        // Output Y sine table
        for (int t = 0; t < ny; ++t)
            output.write<Word>(ySine(t, yri)*80);

        output.write(code);
        output.write(data);
    }
    int xSine(int t, int xr)
    {
        int nx = 1 + tau*(80-xr);
        int x = sin(t*tau/nx)*(80-xr)+(80-xr);
        return clamp(0, x, 160-(xr*2 + 1));
    }
    int ySine(int t, int yr)
    {
        int ny = 1 + tau*(50-yr);
        int y = sin(t*tau/ny)*(50-yr)+(50-yr);
        return clamp(0, y, 100-(yr*2 + 1));
    }
};