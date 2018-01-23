#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> observed(0x1000000);
        Array<Byte> expected(0x1000000);
        String dir("M:\\Program Files (x86)\\Apache Software Foundation\\"
            "Apache24\\htdocs\\");
        //for (int i = 0; i < 0x100; ++i) {
        //    File(dir + "VXBaff12_jb2NCDE" + decimal(i) + ".dat", true).
        //        openRead().read(&observed[i << 16], 0x10000);

        for (int i = 0; i < 0x100; ++i) {
            if (i < 193) {
                File(dir + "86-7-WTkKuASmtlj" + decimal(i) + ".dat", true).
                    openRead().read(&observed[i << 16], 0x10000);
            }
            else {
                File(dir + "5egBqeiqajT-IYHl" + decimal(i - 193) + ".dat",
                    true).openRead().read(&observed[i << 16], 0x10000);
            }
        }

        for (int dividend = 0; dividend < 0x10000; ++dividend) {
            for (int divisor = 0; divisor < 0x100; ++divisor) {
                int t = 239;
                int remainder = dividend;
                int quotient = 0;
                int qbit = 0x80;
                int x = divisor << 8;
                if (remainder < x) {
                    t = 194;
                    for (int b = 0; b < 8; ++b) { 
                        x >>= 1;
                        if (remainder >= x) {
                            remainder -= x;
                            ++t;
                            if (b == 7)
                                t += 2;
                            quotient |= qbit;
                        }
                        qbit >>= 1;
                    }
                }
                int o = ((dividend & 0xff00) << 8) + (divisor << 8) + (dividend & 0xff);
                expected[o] = t;
            }
        }
        Array<Byte> delta(0x1000000);
        Array<int> hist(513);
        for (int i = 0; i < 513; ++i)
            hist[i] = 0;
        for (int y = 0; y < 0x1000; ++y) {
            for (int x = 0; x < 0x1000; ++x) {
                int divisor = y & 0xff;
                int dividend = x + ((y & 0xf00) << 4);
                int o = ((dividend & 0xff00) << 8) + (divisor << 8) + (dividend & 0xff);
                int c = observed[o];
//                int c = expected[o] - observed[o] + 128;
                delta[y*0x1000 + x] = c;
                int i = (c - 128) + 256;
                ++hist[i];
                //if (c != 128 && divisor <= 128)
                //    printf("Delta at divisor %i\n",divisor);
            }
        }

        File("diff.raw").save(delta);
        File("hist.raw").save(hist);
    }
};