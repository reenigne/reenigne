#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        srand(12345);
        //Byte* p = reinterpret_cast<Byte*>(run);

        Array<float> l(256*256);
        float minV = 1.0e20;
        float maxV = -1.0e20;
        Array<double> as(256*256);
        Array<double> ps(256*256);
        for (int xf = 1; xf < 256; ++xf) {
            for (int yf = 1; yf < 256; ++yf) {
                int r1 = rand();
                double a = r1/32768.0/sqrt(xf*xf + yf*yf);
                int r2 = rand();
                double p = tau*r2/32768.0;
                as[yf*256 + xf] = a;
                ps[yf*256 + xf] = p;

            }
        }

        for (int y = 0; y < 256; ++y) {
            for (int x = 0; x < 256; ++x) {
                float v = 0;
                for (int xf = 1; xf < 25; ++xf)
                    for (int yf = 1; yf < 25; ++yf)
                        v += sin(x*xf*tau/256 + y*yf*tau/256 + ps[yf*256 + xf])*as[yf*256 + xf];
                l[y*256 + x] = v;
                minV = min(minV, v);
                maxV = max(maxV, v);
            }
        }
        printf("%f %f\n",minV,maxV);
        Array<Byte> lb(256*256);
        for (int y = 0; y < 256; ++y) {
            for (int x = 0; x < 256; ++x) {
                lb[y*256 + x] = byteClamp((l[y*256 + x]-minV)*256/(maxV-minV));
            }
        }
        //for (int i = 0; i < 0x10000; ++i)
        //    lb[i] = 180;
        //for (int xf = 1; xf < 12; ++xf) {
        //    for (int yf = 1; yf < 12; ++yf) {
        //        float phase = rand() & 0xff;
        //        float amplitude = (rand() & 0xff)/static_cast<float>(4*xf*yf);
        //        for (int y = 0; y < 256; ++y) {
        //            for (int x = 0; x < 256; ++x)
        //                lb[y*256 + x] += sin((x*xf + y*yf + phase)*tau/256)*amplitude;
        //        }
        //    }
        //}

        File("output.raw").openWrite().write(lb);

        Array<Byte> screen(320*200);
        float xp = 128;
        float yp = 128;
        for (int x = 0; x < 320; ++x) {
            int maxY = 0;
            float theta = x/320.0f*tau/4;
            for (int r = 1; r < 100; ++r) {
                int xh = static_cast<int>(xp + r*sin(theta)) & 0xff;
                int yh = static_cast<int>(yp + r*cos(theta)) & 0xff;
                int h = lb[yh*256 + xh];
                int y = clamp(0, static_cast<int>(25*(h - 256)/r + 180), 199);
                for (int yy = maxY; yy <= y; ++yy)
                    screen[(199-yy)*320 + x] = lb[yh*256 + ((xh + 128)&0xff)];
                maxY = max(maxY, y);
            }
            for (int yy = maxY; yy < 200; ++yy)
                screen[(199-yy)*320 + x] = 0;
        }
        File("screen.raw").openWrite().write(screen);
    }

    //void run()
    //{
    //    Byte* rn = reinterpret_cast<Byte*>(0x123456);
    //    Byte* d = reinterpret_cast<Byte*>(0x234567);
    //    volatile Byte* v = reinterpret_cast<volatile Byte*>(0x345678);
    //    for (int i = 0; i < 0x10000; ++i)
    //        d[i] = 128;
    //    for (int xf = 1; xf < 25; ++xf) {
    //        for (int yf = 1; yf < 25; ++yf) {
    //            float phase = *rn++;
    //            float amplitude = (*rn++)/static_cast<float>(4*xf*yf);
    //            for (int y = 0; y < 256; ++y) {
    //                for (int x = 0; x < 256; ++x)
    //                    d[y*256 + x] += sin((x*xf + y*yf + phase)*tau/256)*amplitude;
    //            }
    //        }
    //    }
    //    int xp = 0;
    //    do {
    //        for (int x = 0; x < 320; ++x) {
    //            int maxY = 0;
    //            float theta = (x + xp)/320.0f*tau/4;
    //            for (int r = 1; r < 100; ++r) {
    //                int xh = (xp + static_cast<int>(r*sin(theta))) & 0xff;
    //                int yh = static_cast<int>(r*cos(theta)) & 0xff;
    //                int h = d[yh*256 + xh];
    //                int y = clamp(0, static_cast<int>(25*(h - 256)/r + 180), 199);
    //                for (int yy = maxY; yy <= y; ++yy)
    //                    v[(199-yy)*320 + x] = d[yh*256 + ((xh + 128)&0xff)];
    //                maxY = max(maxY, y);
    //            }
    //            for (int yy = maxY; yy < 200; ++yy)
    //                v[(199-yy)*320 + x] = 0;
    //        }
    //        ++xp;
    //    } while (true);
    //}

};