#include "alfe/sdl2.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"
#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        SDLWindow window;
        SDLRenderer renderer(&window);
        SDLTexture texture(&renderer);


        double yr = 10.5;
        double xr = 3*yr/5;
        int yri = static_cast<int>(yr);
        int xri = static_cast<int>(xr);

        // Diamond     2.42
        // Flint glass 1.62
        // Crown glass 1.52
        double refractiveIndex = 2;
        double D = 0.25;  // Distance from sphere to image

        for (int yi = 0; yi < yri*2 + 1; ++yi) {
            double y = (yi - yri)/yr;
            for (int xi = 0; xi < xri*2 + 1; ++xi) {
                double x = (xi - xri)/xr;
                double r = sqrt(x*x + y*y);
                if (r >= 1)
                    //printf("        ");
                    printf("          ");
                else {
                    double incidence = asin(r);
                    double sinRefraction = r/refractiveIndex;
                    double refraction = asin(sinRefraction);
                    double alpha = incidence - 2*refraction;
                    double d = (D+(1 - cos(alpha)))/(cos(alpha + incidence));  // distance ray travels before entering sphere
                    double a = sin(alpha) + d*sin(alpha + incidence);          // distance along picture from sphere center to ray start
                    double xp = -a*x/r;
                    double yp = -a*y/r;
                    int xpi = lround(xp*xr);
                    int ypi = lround(yp*yr);
                    int address = xpi*2 + ypi*160;
                    //printf("0x%04x, ", address & 0xffff);
                    printf("%4i,%4i ",xpi,ypi);
                }
            }
            printf("\n");
        }
    }
};