//#include "def.h"
#include "unity/user.h"
//#include <math.h>
//#include "fractal.h"
//#include <vector>
//
//class Pixel
//{
//public:
//    Pixel() { reset(); }
//    void reset() { hits = 0; }
//    int colour(float exposure) const 
//    {
//        return 255 - static_cast<int>(255.0f*exp(hits/exposure));
//    }
//    int getHits() const { return hits; }
//    void increment() { ++hits; }
//private:
//    int hits;
//};
//
//class BifCalcThread : public CalcThread
//{
//public:
//    BifCalcThread(Region region) : CalcThread(region) { initialize(); }
//
//    void draw(Byte* buffer, int byteWidth)
//    {
//        if (_region.pixels() == 0)
//            return;
//
//        int n = 0;
//        float exposure = 0;
//        for (std::vector<Pixel>::const_iterator pp = pixels.begin(); pp != pixels.end(); ++pp) {
//            int hits = pp->getHits();
//            if (hits > 0) {
//                ++n;
//                exposure -= hits;
//            }
//        }
//        exposure /= n;
//
//        std::vector<Pixel>::const_iterator pp = pixels.begin();
//        for (int ys=0;ys<_region.getSize().y;++ys) {
//            Byte* p = buffer;
//            for (int xs=0;xs<_region.getSize().x;++xs) {
//                int c = pp->colour(exposure);
//                *(p++) = c;
//                *(p++) = c;
//                *(p++) = c;
//                ++p;
//                ++pp;
//            }
//            buffer += byteWidth;
//        }
//    }
//
//private:
//    void calculate()
//    {
//        offset += e;
//        if (offset >= 1.0)
//            offset -= 1.0;
//        float y = 0.6;
//        for (int xs=_region.getSize().x - 1; xs>=0; --xs) {
//            float x = static_cast<float>(_region.cxFromSx(xs + offset));
//            for (int i=0;i<100;++i) 
//                y=x*y*(1.0f-y);
//            for (int i=0;i<1000;++i) {
//                y=x*y*(1.0f-y);
//                int ys = _region.syFromCy(y);
//                if (ys>=0 && ys<_region.getSize().y)
//                    pixels[ys*_region.getSize().x+xs].increment();
//            }
//            if (y > 10 || y < -10 || (y>-1e-20 && y<1e-20))
//                y = 0.6;
//        }
//    }
//
//    void restart()
//    {
//        pixels.resize(_region.pixels());
//        for (std::vector<Pixel>::iterator pp = pixels.begin(); pp != pixels.end(); ++pp)
//            pp->reset();
//        offset = 0;
//        e = exp(1.0f)-2; // Any old irrational number will do here
//    }
//
//    std::vector<Pixel> pixels;
//    float offset;
//    float e;
//};

INT APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT nCmdShow)
{
    BEGIN_CHECKED {
//        ComInitializer ci;
        Windows windows(hInst);
//        Region region(Vector2<double>(3.5f, 0.0f), Vector2<double>(4.0f, 1.0f), Vector2<int>(2, 2));
//        typedef FractalImage<BifCalcThread> Image;
//        Image image(region);
//
//        Window::Params wp(&windows, L"Bifurcation Fractal");
//        typedef RootWindow<Window> RootWindow;
//        RootWindow::Params rwp(wp);
//        typedef ImageWindow<RootWindow, Image> ImageWindow;
//        ImageWindow::Params iwp(rwp, &image);
//        typedef AnimatedWindow<ImageWindow> AnimatedWindow;
//        AnimatedWindow::Params awp(iwp);
//        typedef ZoomableWindow<AnimatedWindow> ZoomableWindow;
//        ZoomableWindow::Params zwp(awp);
//        ZoomableWindow window(zwp);
//
//        window.show(nCmdShow);
//        return pumpMessages();
    } END_CHECKED(Exception& e) {
        e.display(); 
    }
    return 0;
}