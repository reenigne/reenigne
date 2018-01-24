#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> observed(0x1000000);
        String dir("M:\\Program Files (x86)\\Apache Software Foundation\\"
            "Apache24\\htdocs\\");
        for (int i = 0; i < 0x100; ++i) {
            File(dir + "VXBaff12_jb2NCDE" + decimal(i) + ".dat", true).
                openRead().read(&observed[i << 16], 0x10000);
        }
        observed[98 + (204 << 8)] = 194;
        observed[(1261 & 0xff) + ((667 - 512) << 8) + (((1261 + 8192) & 0xff00) << 8)] = 196;
        observed[(1263 & 0xff) + ((667 - 512) << 8) + (((1263 + 8192) & 0xff00) << 8)] = 199;
        observed[(1276 & 0xff) + ((667 - 512) << 8) + (((1276 + 8192) & 0xff00) << 8)] = 199;
        observed[(1630 & 0xff) + ((409 - 256) << 8) + (((1630 + 4096) & 0xff00) << 8)] = 199;

        //for (int i = 0; i < 0x100; ++i) {
        //    if (i < 193) {
        //        File(dir + "86-7-WTkKuASmtlj" + decimal(i) + ".dat", true).
        //            openRead().read(&observed[i << 16], 0x10000);
        //    }
        //    else {
        //        File(dir + "5egBqeiqajT-IYHl" + decimal(i - 193) + ".dat",
        //            true).openRead().read(&observed[i << 16], 0x10000);
        //    }
        //}

        Array<Byte> expected(0x1000000);
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
                observed[o] -= t;
            }
        }
        //for (int dividend1 = 0; dividend1 < 0x10000; ++dividend1) {
        //    for (int divisor1 = 0; divisor1 < 0x100; ++divisor1) {
        //        int dividend = dividend1;
        //        int divisor = divisor1;

        //        int t = 214;
        //        bool negative = false;
        //        bool dividendNegative = false;
        //        if ((dividend & 0x8000) != 0) {
        //            dividend = (~dividend + 1) & 0xffff;
        //            negative = true;
        //            dividendNegative = true;
        //            t += 4;
        //        }
        //        if ((divisor & 0x80) != 0) { 
        //            divisor = (~divisor + 1) & 0xff;
        //            negative = !negative;
        //        }
        //        else
        //            t += 1;
        //        int remainder = dividend;
        //        int quotient = 0;
        //        int qbit = 0x80;
        //        int x = divisor << 8;
        //        if (remainder < x) {
        //            for (int b = 0; b < 8; ++b) { 
        //                x >>= 1;
        //                if (remainder >= x) {
        //                    remainder -= x;
        //                    ++t;
        //                    if (b == 7)
        //                        t += 2;
        //                    quotient |= qbit;
        //                }
        //                qbit >>= 1;
        //            }
        //            if (/*remainder >= x*/ (quotient & 0x80) != 0)
        //                t += 105;
        //            else {
        //                if (negative)
        //                    quotient = ~quotient + 1;
        //                if (dividendNegative)
        //                    remainder = ~remainder + 1;
        //            }
        //        }
        //        else
        //            t += 34;

        //        int o = ((dividend1 & 0xff00) << 8) + (divisor1 << 8) + (dividend1 & 0xff);
        //        expected[o] = t;
        //    }
        //}

        Array<Byte> rearranged(0x1000000);
        for (int quotient = 0; quotient < 0x100; ++quotient) {
            for (int divisor = 0; divisor < 0x100; ++divisor) {
                for (int remainder = 0; remainder < 0x100; ++remainder) {
                    int dividend = quotient*divisor + remainder;
                    int o = ((dividend & 0xff00) << 8) + (divisor << 8) + (dividend & 0xff);
                    int p = ((quotient & 0xf) << 8) + remainder + ((((quotient & 0xf0) << 4) + divisor) << 12);
                    if (dividend < 0x10000 && remainder < divisor)
                        rearranged[p] = observed[o];
                    else
                        rearranged[p] = 0;
                }
            }
        }
        File("rearranged.raw").save(rearranged);

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
                //int c = expected[o] - observed[o];
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