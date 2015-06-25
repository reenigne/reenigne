#include "alfe/main.h"
#include "alfe/complex.h"

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= M_PI;
    return sin(z)/z;
}

static const int lobes = 5;

float lanczos(float z)
{
    return sinc(z)*sinc(z/lobes);
}

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> input;
        FileHandle inputHandle = File("D:\\t\\c2.raw", true).openRead();
        int nn = inputHandle.size();
        input.allocate(nn);
        inputHandle.read(&input[0], nn);

        Array<Byte> outputb(1820*2000);
        int z = 2;
        for (int x = 3*z; x < 1820*2000 - 3*z; ++x) {
            // Sample rate is 28.6MHz
            // Luma carrier is 3.4MHz to 4.4MHz = 6.5 to 8.4 samples
            // d is tau/8.4 to tau/6.5 = 0.75 to 0.97
            // c is 0.57 to 0.73

            // c = cos(d)
            // s = sin(d)
            // v = a + b*x + e*cos(d*x) + f*sin(d*x)

            int v_3 = input[x-z*3];  // = a - 3*b + e*cos(3*d) - f*sin(3*d) 
            int v_2 = input[x-z*2];  // = a - 2*b + e*cos(2*d) - f*sin(2*d)
            int v_1 = input[x-z];    // = a - b + e*c - f*s
            int v0 = input[x];       // = a + e
            int v1 = input[x+z];     // = a + b + e*c + f*s
            int v2 = input[x+z*2];   // = a + 2*b + e*cos(2*d) + f*sin(2*d)
            int v3 = input[x+z*3];   // = a + 3*b + e*cos(3*d) + f*sin(3*d)

            int o = v0;       // = a + e
            int m = v_1 + v1; // = 2*a + 2*e*c
            int n = v_2 + v2; // = 2*a + 2*e*cos(2*d) = 2*a + 2*e*(2*c*c - 1)
            int p = v1 - v_1; // = 2*b + 2*f*s
            int q = v2 - v_2; // = 4*b + 2*f*sin(2*d) = 4*b + 4*f*s*c
            int r = v3 - v_3; // = 6*b + 2*f*sin(3*d) = 6*b + 2*f*(3*s + 4*s*s*s) = 6*b + 14*f*s - 8*c*c*f*s

            int g0 = 2*m - 4*o; // = 4*e*(c - 1)
            int h0 = 2*o - n;   // = 4*e*(1 - c*c)
            int i0 = - h0 - g0; // = 4*e*(c*c - c)
            int g1 = 2*q - 4*p; // = 8*f*s*(c - 1)
            int h1 = r - 3*p;   // = 8*f*s*(1 - c*c)
            int i1 = - h1 - g1; // = 8*f*s*(c*c - c)

            // Now we have two quadratic equations in c: 
            //   c*c*g0 + c*h0 + i0 = 0
            //   c*c*g1 + c*h1 + i1 = 0

            int k0 = h0*h0 - 4*g0*i0;
            int k1 = h1*h1 - 4*g1*i1;
            if (k0 < 0 || k1 < 0)
                printf("Out of range!\n");
            float j0 = sqrt(static_cast<float>(k0)); // = 4*e*(c*c - 2*c + 1)
            float j1 = sqrt(static_cast<float>(k1)); // = 8*f*s*(c*c - 2*c + 1)

            float c0 = (-h0 + j0)/(2*g0);  // The -j solutions give c = 1
            float c1 = (-h1 + j1)/(2*g1);  

            float c = c1; //(c0*g0*g0 + c1*g1*g1)/(g0*g0 + g1*g1);
            float d = acos(c);
            //outputb[x] = byteClamp((d-0.74)*255/(0.98 - 0.74));
            outputb[x] = byteClamp(d*255/M_PI);
        }
        FileHandle h = File("D:\\t\\vcr_decoded.raw", true).openWrite();
        h.write(outputb);
    }
};