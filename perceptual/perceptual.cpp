#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <time.h>
#include <float.h>
#include "alfe/user.h"
#include "alfe/main.h"
#include "alfe/minimum_maximum.h"
#include "alfe/colour_space.h"

double random(double maximum)
{
    return (static_cast<double>(rand())*maximum)/static_cast<double>(RAND_MAX);
}

ColourSpace colourSpace = ColourSpace::lab();

class Particle
{
public:
    void init()
    {
        _lab = colourSpace.fromSrgb(
            Colour(random(256.0), random(256.0), random(256.0)));
    }
    void simulate()
    {
        Colour srgb = colourSpace.toSrgb(_lab);
        double delta = 0.00001;
        bool collision = false;
        if (srgb.x < 0 || srgb.x > 256) {
            srgb.x = clamp(0.0, srgb.x, 256.0);
            collision = true;
        }
        if (srgb.y < 0 || srgb.y > 256) {
            srgb.y = clamp(0.0, srgb.y, 256.0);
            collision = true;
        }
        if (srgb.z < 0 || srgb.z > 256) {
            srgb.z = clamp(0.0, srgb.z, 256.0);
            collision = true;
        }
        if (collision)
            _lab = colourSpace.fromSrgb(srgb);
    }
    double distance(const Particle& other)
    {
        Colour delta = other.lab() - lab();
        return sqrt(delta.modulus2());
    }
    const Colour& lab() const { return _lab; }
    void moveTo(const Colour& lab) { _labNew = lab; }
    void update() { _lab = _labNew; }
    void print()
    {
        SRGB srgb = colourSpace.toSrgb24(_lab);
        printf("  <span style='background-color: #%02x%02x%02x'>&nbsp;&nbsp;</span>\n",
            srgb.x, srgb.y, srgb.z);
    }
    void debug()
    {
        SRGB srgb = colourSpace.toSrgb24(_lab);
        fprintf(stderr, "  %02x%02x%02x", srgb.x, srgb.y, srgb.z);
    }
    void plot(Vector size, Byte* buffer, int byteWidth, int x, int y, DWord c)
    {
        if (x >= 0 && y >= 0 && x < size.x && y < size.y)
            reinterpret_cast<DWord*>(&buffer[y*byteWidth])[x] = c;
    }
    void draw(Vector size, Byte* buffer, int byteWidth, double* matrix)
    {
        SRGB srgb = colourSpace.toSrgb24(_lab);
        double transformed[3];
        double x = _lab.x/100.0;
        double y = _lab.y/100.0;
        double z = _lab.z/100.0;
        transformed[0] = x*matrix[0] + y*matrix[1] + z*matrix[2];
        transformed[1] = x*matrix[3] + y*matrix[4] + z*matrix[5];
        transformed[2] = x*matrix[6] + y*matrix[7] + z*matrix[8];
        transformed[0] /= (transformed[2] + 3.0)/2.0;
        transformed[1] /= (transformed[2] + 3.0)/2.0;
        int x0 = static_cast<int>(size.x*(transformed[0] + 2.5)/5.0);
        int y0 = static_cast<int>(size.y*(transformed[1] + 2.5)/5.0);
        DWord c = (srgb.x << 16) | (srgb.y << 8) | srgb.z;
        if (c == 0) {
            c = 0xffffff;
            plot(size, buffer, byteWidth, x0 - 1, y0 - 2, c);
            plot(size, buffer, byteWidth, x0,     y0 - 2, c);
            plot(size, buffer, byteWidth, x0 + 1, y0 - 2, c);
            plot(size, buffer, byteWidth, x0 - 2, y0 - 1, c);
            plot(size, buffer, byteWidth, x0 + 2, y0 - 1, c);
            plot(size, buffer, byteWidth, x0 - 2, y0, c);
            plot(size, buffer, byteWidth, x0 + 2, y0, c);
            plot(size, buffer, byteWidth, x0 - 2, y0 + 1, c);
            plot(size, buffer, byteWidth, x0 + 2, y0 + 1, c);
            plot(size, buffer, byteWidth, x0 - 1, y0 + 2, c);
            plot(size, buffer, byteWidth, x0,     y0 + 2, c);
            plot(size, buffer, byteWidth, x0 + 1, y0 + 2, c);
        }
        else {
            plot(size, buffer, byteWidth, x0 - 1, y0 - 2, c);
            plot(size, buffer, byteWidth, x0,     y0 - 2, c);
            plot(size, buffer, byteWidth, x0 + 1, y0 - 2, c);
            plot(size, buffer, byteWidth, x0 - 2, y0 - 1, c);
            plot(size, buffer, byteWidth, x0 - 1, y0 - 1, c);
            plot(size, buffer, byteWidth, x0,     y0 - 1, c);
            plot(size, buffer, byteWidth, x0 + 1, y0 - 1, c);
            plot(size, buffer, byteWidth, x0 + 2, y0 - 1, c);
            plot(size, buffer, byteWidth, x0 - 2, y0, c);
            plot(size, buffer, byteWidth, x0 - 1, y0, c);
            plot(size, buffer, byteWidth, x0,     y0, c);
            plot(size, buffer, byteWidth, x0 + 1, y0, c);
            plot(size, buffer, byteWidth, x0 + 2, y0, c);
            plot(size, buffer, byteWidth, x0 - 2, y0 + 1, c);
            plot(size, buffer, byteWidth, x0 - 1, y0 + 1, c);
            plot(size, buffer, byteWidth, x0,     y0 + 1, c);
            plot(size, buffer, byteWidth, x0 + 1, y0 + 1, c);
            plot(size, buffer, byteWidth, x0 + 2, y0 + 1, c);
            plot(size, buffer, byteWidth, x0 - 1, y0 + 2, c);
            plot(size, buffer, byteWidth, x0,     y0 + 2, c);
            plot(size, buffer, byteWidth, x0 + 1, y0 + 2, c);
        }
    }
    bool operator==(const Particle& other) { return _lab == other._lab; }
    //bool operator<(const Particle& other) { return _lab < other._lab; }
    //bool operator>(const Particle& other) { return other._lab < _lab; }
    //bool operator<=(const Particle& other) { return !(other._lab < _lab); }
    //bool operator>=(const Particle& other) { return !(_lab < other._lab); }
private:
    Colour _lab;
    Colour _labNew;
};

class Simulation
{
public:
    void init()
    {
        for (int i = 0; i < _n; ++i)
            _particles[i].init();
        _bestDistance = 0;
        _done = false;
    }
    Simulation(int n) : _n(n), _particles(n), _iters(0) { }
    double bestDistance()
    {
        return _bestDistance;
    }
    void print()
    {
        //std::sort(_bestParticles.begin(), _bestParticles.end());
        printf("<p>%i colours, distance = %lf</p><p>\n", _n, _bestDistance);
        for (int i = 0; i < _n; ++i)
            _bestParticles[i].print();
        printf("</p>\n");
    }
    void debug()
    {
        fprintf(stderr, "%lf  ", _bestDistance);
        for (int i = 0; i < _n; ++i)
            _bestParticles[i].debug();
        fprintf(stderr, "\n");
    }
    void simulateFrame()
    {
        double minimumDistance = 100000;
        for (int i = 0; i < _n; ++i) {
            int closest = 0;
            double d = 100000;
            for (int j = 0; j < _n; ++j)
                if (j != i) {
                    double dd = _particles[i].distance(_particles[j]);
                    if (dd < d) {
                        d = dd;
                        closest = j;
                    }
                }
            Colour delta = _particles[i].lab() - _particles[closest].lab();
            delta /= sqrt(delta.modulus2());
            _particles[i].moveTo(_particles[i].lab() + delta*0.1);

            if (d < minimumDistance)
                minimumDistance = d;
        }
        if (minimumDistance > _bestDistance) {
            _bestIter = _iters;
            _bestParticles = _particles;
            _bestDistance = minimumDistance;
        }
        for (int i = 0; i < _n; ++i) {
            _particles[i].update();
            _particles[i].simulate();
        }
        ++_iters;
        if ((_iters % 10000) == 0)
            fprintf(stderr, ".");

        if (_iters > _bestIter + 100000)
            _done = true;
    }
    void draw(Vector size, Byte* buffer, int byteWidth, double* matrix)
    {
        Byte* l = buffer;
        for (int y = 0; y < size.y; ++y) {
            DWord* p = reinterpret_cast<DWord*>(l);
            for (int x = 0; x < size.x; ++x)
                *(p++) = 0;
            l += byteWidth;
        }
        for (int i = 0; i < _n; ++i)
            _particles[i].draw(size, buffer, byteWidth, matrix);
    }
    bool operator==(const Simulation& other)
    {
        for (int i = 0; i < _n; ++i)
            if (!(_particles[i] == other._particles[i]))
                return false;
        return true;
    }
    bool done() const { return _done; }
private:
    int _n;
    double _bestDistance;
    std::vector<Particle> _particles;
    std::vector<Particle> _bestParticles;
    int _iters;
    int _bestIter;
    bool _done;
};

//int main()
//{
//    srand(static_cast<unsigned int>(time(0)));
//    printf("<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n");
//    printf("<html xmlns='http://www.w3.org/1999/xhtml' dir='ltr' lang='en-US'>\n");
//    printf("<head><meta http-equiv='Content-Type' content='text/html; charset=UTF-8' /></head><body>\n");
//    for (int particles = 2; particles <= 36; ++particles) {
//        int sims = 10;
//        std::vector<Simulation> simulations(sims, particles);
//        fprintf(stderr, "Particles = %i\n", particles);
//        int best = 0;
//        double bestDistance = 0;
//        for (int simulation = 0; simulation < sims; ++simulation) {
//			simulations[simulation].init();
//			do {
//				simulations[simulation].simulateFrame();
//			} while (!simulations[simulation].done());
//            double d = simulations[simulation].bestDistance();
//            fprintf(stderr, "Iteration %i: ", simulation);
//			simulations[simulation].debug();
//            if (d > bestDistance) {
//                bestDistance = d;
//                best = simulation;
//            }
//        }
//        simulations[best].print();
//    }
//    printf("</body></html>\n");
//}


template<class Base> class ThreeDWindow : public Base
{
public:
    class Params
    {
        friend class ThreeDWindow;
    public:
        Params(typename Base::Params bp)
          : _bp(bp) { }
    private:
        typename Base::Params _bp;
    };

    ThreeDWindow(Params p)
      : Base(p._bp)
    {
        for (int i = 0; i < 3; ++i)
            _axes[i] = 0;
        _keys = "QWEASD";
        for (int i = 0; i < 6; ++i)
            _pressed[i] = false;
    }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_KEYUP:
                for (int i = 0; i < 6; ++i)
                    if (wParam == _keys[i])
                        _pressed[i] = false;
                break;
            case WM_KEYDOWN:
                for (int i = 0; i < 6; ++i)
                    if (wParam == _keys[i])
                        _pressed[i] = true;
                break;
            case WM_PAINT:
                for (int i = 0; i < 6; ++i)
                    if (_pressed[i])
                        _axes[i%3] += (i >= 3 ? 1 : -1);
                _image->setAxes(_axes);
                break;
        }
        return Base::handleMessage(uMsg, wParam, lParam);
    }
private:
    char* _keys;
    bool _pressed[6];
    int _axes[3];
};


class PerceptualImage : public Image
{
public:
    PerceptualImage() : _simulation(36) { _simulation.init(); }

    void plot(Vector size, Byte* buffer, int byteWidth, int x, int y, DWord c)
    {
        if (x >= 0 && y >= 0 && x < size.x && y < size.y)
            reinterpret_cast<DWord*>(&buffer[y*byteWidth])[x] = c;
    }
    void plot(Vector size, Byte* buffer, int byteWidth, double* matrix, Colour srgb)
    {
        Colour lab = colourSpace.fromSrgb(srgb);
        double transformed[3];
        double x = lab.x/100.0;
        double y = lab.y/100.0;
        double z = lab.z/100.0;
        transformed[0] = x*matrix[0] + y*matrix[1] + z*matrix[2];
        transformed[1] = x*matrix[3] + y*matrix[4] + z*matrix[5];
        transformed[2] = x*matrix[6] + y*matrix[7] + z*matrix[8];
        transformed[0] /= (transformed[2] + 3.0)/4.0;
        transformed[1] /= (transformed[2] + 3.0)/4.0;
        int x0 = static_cast<int>(size.x*(transformed[0] + 2.5)/5.0);
        int y0 = static_cast<int>(size.y*(transformed[1] + 2.5)/5.0);
        DWord c = (clamp(0, static_cast<int>(srgb.x), 255) << 16) | (clamp(0, static_cast<int>(srgb.y), 255) << 8) | clamp(0, static_cast<int>(srgb.z), 255);
        plot(size, buffer, byteWidth, x0, y0, c);
    }

    void paint(const PaintHandle& paint)
    {
        _simulation.simulateFrame();
        _simulation.draw(_size, getBits(), _byteWidth, _matrix);

        //Byte* l = getBits();
        //for (int y = 0; y < _size.y; ++y) {
        //    DWord* p = reinterpret_cast<DWord*>(l);
        //    for (int x = 0; x < _size.x; ++x)
        //        *(p++) = 0xffffff;
        //    l += _byteWidth;
        //}
        //for (double i = 0; i < 256; i += .1) {
        //    for (double j = 0; j < 256; j += 16) {
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(j, 0, i));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(j, 255, i));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(0, j, i));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(255, j, i));

        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(j, i, 0));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(j, i, 255));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(0, i, j));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(255, i, j));

        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(i, 0, j));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(i, j, 0));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(i, j, 255));
        //        plot(_size, getBits(), _byteWidth, _matrix, Colour(i, 255, j));
        //    }
        //    plot(_size, getBits(), _byteWidth, _matrix, Colour(255, 255, i));
        //    plot(_size, getBits(), _byteWidth, _matrix, Colour(255, i, 255));
        //    plot(_size, getBits(), _byteWidth, _matrix, Colour(i, 255, 255));
        //}
        Image::paint(paint);
    }

    void setAxes(int* axes)
    {
        for (int i = 0; i < 9; ++i)
            _matrix[i] = (i%4 == 0 ? 1 : 0);
        for (int i = 0; i < 3; ++i) {
            double a = static_cast<double>(axes[i] & 0x7f)*tau/128.0;
            double c = cos(a);
            double s = sin(a);
            double m[9];
            for (int j = 0; j < 9; ++j)
                m[j] = (j%4 == 0 ? 1 : 0);
            static const int a1[3] = {0, 0, 1};
            static const int a2[3] = {1, 2, 2};
            int axis1 = a1[i];
            int axis2 = a2[i];
            m[axis1*3 + axis1] = c;
            m[axis1*3 + axis2] = s;
            m[axis2*3 + axis1] = -s;
            m[axis2*3 + axis2] = c;
            double newMatrix[9];
            newMatrix[0] = _matrix[0]*m[0] + _matrix[1]*m[3] + _matrix[2]*m[6];
            newMatrix[1] = _matrix[0]*m[1] + _matrix[1]*m[4] + _matrix[2]*m[7];
            newMatrix[2] = _matrix[0]*m[2] + _matrix[1]*m[5] + _matrix[2]*m[8];
            newMatrix[3] = _matrix[3]*m[0] + _matrix[4]*m[3] + _matrix[5]*m[6];
            newMatrix[4] = _matrix[3]*m[1] + _matrix[4]*m[4] + _matrix[5]*m[7];
            newMatrix[5] = _matrix[3]*m[2] + _matrix[4]*m[5] + _matrix[5]*m[8];
            newMatrix[6] = _matrix[6]*m[0] + _matrix[7]*m[3] + _matrix[8]*m[6];
            newMatrix[7] = _matrix[6]*m[1] + _matrix[7]*m[4] + _matrix[8]*m[7];
            newMatrix[8] = _matrix[6]*m[2] + _matrix[7]*m[5] + _matrix[8]*m[8];
            for (int j = 0; j < 9; ++j)
                _matrix[j] = newMatrix[j];
        }
    }
    void destroy() { }
private:
    Simulation _simulation;
    double _matrix[9];
};

class Program : public ProgramBase
{
public:
    void run()
    {
        srand(time(0));
        PerceptualImage image;

        Window::Params wp(&_windows, L"Separation in perceptual colour space"/*,
            Vector(480, 480)*/);
        typedef RootWindow<Window> RootWindow;
        RootWindow::Params rwp(wp);
        typedef ImageWindow<RootWindow, PerceptualImage> ImageWindow;
        ImageWindow::Params iwp(rwp, &image);
        typedef AnimatedWindow<ImageWindow> AnimatedWindow;
        AnimatedWindow::Params awp(iwp);
        typedef ThreeDWindow<AnimatedWindow> ThreeDWindow;
        ThreeDWindow::Params fwp(awp);
        ThreeDWindow window(awp);

        window.show(_nCmdShow);
        pumpMessages();
    }
};
