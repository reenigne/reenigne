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

        auto background =
            PNGFileFormat<DWORD>().load(File("../../../../../Pictures/reenigne/cga2ntsc/g1k_000_out.png", false));

        aspect = 1;  // 3.0/5

        yr = 10.5;

        // Diamond     2.42
        // Flint glass 1.62
        // Crown glass 1.52
        refractiveIndex = 2;
        D = 0.25;  // Distance from sphere to image

        //for (int yi = 0; yi < yri*2 + 1; ++yi) {
        //    double y = (yi - yri)/yr;
        //    for (int xi = 0; xi < xri*2 + 1; ++xi) {
        //        double x = (xi - xri)/xr;
        //        double r = sqrt(x*x + y*y);
        //        if (r >= 1)
        //            //printf("        ");
        //            printf("          ");
        //        else {
        //            double incidence = asin(r);
        //            double sinRefraction = r/refractiveIndex;
        //            double refraction = asin(sinRefraction);
        //            double alpha = incidence - 2*refraction;
        //            double d = (D+(1 - cos(alpha)))/(cos(alpha + incidence));  // distance ray travels before entering sphere
        //            double a = sin(alpha) + d*sin(alpha + incidence);          // distance along picture from sphere center to ray start
        //            double xp = -a*x/r;
        //            double yp = -a*y/r;
        //            int xpi = lround(xp*xr);
        //            int ypi = lround(yp*yr);
        //            int address = xpi*2 + ypi*160;
        //            //printf("0x%04x, ", address & 0xffff);
        //            printf("%4i,%4i ",xpi,ypi);
        //        }
        //    }
        //    printf("\n");
        //}

        Vector windowSize = Vector(912, 525);

        Vector imageOffset = (windowSize - background.size())/2;

        Vector lensPosition = windowSize / 2;

        do {
            UInt32 startTime = SDL_GetTicks();

            SDLTextureLock lock(&texture);
            UInt8* row = reinterpret_cast<UInt8*>(lock._pixels);
            int pitch = lock._pitch;
            for (int y = 0; y < windowSize.y; ++y) {
                UInt32* output = reinterpret_cast<UInt32*>(row);
                for (int x = 0; x < windowSize.x; ++x) {
                    Vector v(x, y);
                    Vector l = v - lensPosition;
                    l = lens(l) + lensPosition;
                    v = l - imageOffset;
                    if (v.inside(background.size()))
                        *output = background[v];
                    else
                        *output = 0;
                    ++output;
                }
                row += pitch;
            }

            renderer.renderTexture(&texture);

            SDL_Event e;
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT)
                    return;
                if (e.type == SDL_MOUSEMOTION)
                    lensPosition = Vector(e.motion.x, e.motion.y);
                if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                    switch (e.key.keysym.sym) {
                        case SDLK_UP:
                            yr += 5;
                            break;
                        case SDLK_DOWN:
                            yr -= 5;
                            break;
                        case SDLK_LEFT:
                            refractiveIndex += 0.1;
                            break;
                        case SDLK_RIGHT:
                            refractiveIndex -= 0.1;
                            break;
                        case SDLK_q:
                            D += 0.05;
                            break;
                        case SDLK_w:
                            D -= 0.05;
                            break;
                    }
                    printf("i = %f, D = %f\n",refractiveIndex, D);
                }
            }

            UInt32 endTime = SDL_GetTicks();
            if (endTime - startTime < 16)
                SDL_Delay(16 - (endTime - startTime));
        } while (true);
    }
private:
    Vector lens(Vector p)
    {
        xr = yr*aspect;
        yri = static_cast<int>(yr);
        xri = static_cast<int>(xr);

        double y = p.y/yr;
        double x = p.x/xr;
        double r = sqrt(x*x + y*y);
        if (r < 1) {
            double incidence = asin(r);
            double sinRefraction = r/refractiveIndex;
            double refraction = asin(sinRefraction);
            double alpha = incidence - 2*refraction;
            double d = (D+(1 - cos(alpha)))/(cos(alpha + incidence));  // distance ray travels before entering sphere
            double a = sin(alpha) + d*sin(alpha + incidence);          // distance along picture from sphere center to ray start
            double xp = -a*x/r;
            double yp = -a*y/r;
            return Vector(lround(xp*xr), lround(yp*yr));
        }
        return p;
    }
    double xr;
    double yr;
    int xri;
    int yri;
    double refractiveIndex;
    double D;
    double aspect;
};