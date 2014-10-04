#include "alfe/main.h"
#include "alfe/complex.h"

class CGASimulator
{
public:
    void init()
    {
        double burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = simulateCGA(6, 6, i);
        Complex<double> iq;
        iq.x = burst[0] - burst[2];
        iq.y = burst[1] - burst[3];
        _iqAdjust =
            -iq.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/
            iq.modulus();
    }
    double simulateCGA(int left, int right, int phase)
    {
        static double intensity[4] = {
            0, 0.047932237386703491, 0.15110087022185326, 0.18384206667542458};

        double c = _chroma[((left & 7) << 5) | ((right & 7) << 2) | phase];
        double i = intensity[(left >> 3) | ((right >> 2) & 2)];
        if (!_newCGA)
            return c+i;
        double r = intensity[((left >> 2) & 1) | ((right >> 1) & 2)];
        double g = intensity[((left >> 1) & 1) | (right & 2)];
        double b = intensity[(left & 1) | ((right << 1) & 1)];
        return (c/0.72)*0.29 + (i/0.28)*0.32 + (r/0.28)*0.1 + (g/0.28)*0.22 +
            (b/0.28)*0.07;
    }
    Colour decode(int pixels)
    {
        int rgbi[4];
        rgbi[0] = pixels & 15;
        rgbi[1] = (pixels >> 4) & 15;
        rgbi[2] = (pixels >> 8) & 15;
        rgbi[3] = (pixels >> 12) & 15;
        double s[4];
        for (int t = 0; t < 4; ++t)
            s[t] = simulateCGA(rgbi[t], rgbi[(t+1)&3], t);
        double dc = (s[0] + s[1] + s[2] + s[3])/4;
        Complex<double> iq;
        iq.x = (s[0] - s[2])/2;
        iq.y = (s[1] - s[3])/2;
        double y = dc*_contrast + _brightness;
        iq *= _iqAdjust;
        double r = 255*(y + 0.9563*iq.x + 0.6210*iq.y);
        double g = 255*(y - 0.2721*iq.x - 0.6474*iq.y);
        double b = 255*(y - 1.1069*iq.x + 1.7046*iq.y);
        if (_fixPrimaries)
            return Colour(
                 1.5073*r -0.3725*g -0.0832*b,
                -0.0275*r +0.9350*g +0.0670*b,
                -0.0272*r -0.0401*g +1.1677*b);
        return Colour(r, g, b);
    }

    bool _newCGA;
    bool _fixPrimaries;
    double _chroma[256];
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
    Complex<double> _iqAdjust;
};

class Particle
{
public:
    void plot(Bitmap<DWORD> bitmap, Vector rPosition)
    {
        Vector size = bitmap.size();
        Byte* buffer = bitmap.data();
        int byteWidth = bitmap.stride();
        double zOffset = rPosition.x*0.01;
        double scale = rPosition.y*0.01;
        double x = _position.x/(_position.z + zOffset)*scale;
        double y = _position.y/(_position.z + zOffset)*scale;
        int x0 = static_cast<int>(size.x*x/5.0 + size.x/2);
        int y0 = static_cast<int>(size.x*y/5.0 + size.y/2);
        int r = byteClamp(_colour.x);
        int g = byteClamp(_colour.y);
        int b = byteClamp(_colour.z);
        DWord c = (r << 16) | (g << 8) | b;
        plot(bitmap, Vector(x0,     y0    ), c);
        if (!_big) {
            if (r < 16 && g < 16 && b < 16) {
                c = 0xffffff;
                plot(bitmap, Vector(x0 - 1, y0 - 1), c);
                plot(bitmap, Vector(x0 + 1, y0 - 1), c);
                plot(bitmap, Vector(x0 - 1, y0 + 1), c);
                plot(bitmap, Vector(x0 + 1, y0 + 1), c);
            }
            return;
        }
        plot(bitmap, Vector(x0 - 1, y0 - 2), c);
        plot(bitmap, Vector(x0,     y0 - 2), c);
        plot(bitmap, Vector(x0 + 1, y0 - 2), c);
        plot(bitmap, Vector(x0 - 2, y0 - 1), c);
        plot(bitmap, Vector(x0 - 1, y0 - 1), c);
        plot(bitmap, Vector(x0,     y0 - 1), c);
        plot(bitmap, Vector(x0 + 1, y0 - 1), c);
        plot(bitmap, Vector(x0 + 2, y0 - 1), c);
        plot(bitmap, Vector(x0 - 2, y0    ), c);
        plot(bitmap, Vector(x0 - 1, y0    ), c);
        plot(bitmap, Vector(x0 + 1, y0    ), c);
        plot(bitmap, Vector(x0 + 2, y0    ), c);
        plot(bitmap, Vector(x0 - 2, y0 + 1), c);
        plot(bitmap, Vector(x0 - 1, y0 + 1), c);
        plot(bitmap, Vector(x0,     y0 + 1), c);
        plot(bitmap, Vector(x0 + 1, y0 + 1), c);
        plot(bitmap, Vector(x0 + 2, y0 + 1), c);
        plot(bitmap, Vector(x0 - 1, y0 + 2), c);
        plot(bitmap, Vector(x0,     y0 + 2), c);
        plot(bitmap, Vector(x0 + 1, y0 + 2), c);
        if (r < 16 && g < 16 && b < 16) {
            c = 0xffffff;
            plot(bitmap, Vector(x0 - 1, y0 - 3), c);
            plot(bitmap, Vector(x0,     y0 - 3), c);
            plot(bitmap, Vector(x0 + 1, y0 - 3), c);
            plot(bitmap, Vector(x0 - 1, y0 + 3), c);
            plot(bitmap, Vector(x0,     y0 + 3), c);
            plot(bitmap, Vector(x0 + 1, y0 + 3), c);
            plot(bitmap, Vector(x0 - 2, y0 - 2), c);
            plot(bitmap, Vector(x0 + 2, y0 - 2), c);
            plot(bitmap, Vector(x0 - 3, y0 - 1), c);
            plot(bitmap, Vector(x0 + 3, y0 - 1), c);
            plot(bitmap, Vector(x0 - 3, y0    ), c);
            plot(bitmap, Vector(x0 + 3, y0    ), c);
            plot(bitmap, Vector(x0 - 3, y0 + 1), c);
            plot(bitmap, Vector(x0 + 3, y0 + 1), c);
            plot(bitmap, Vector(x0 - 2, y0 + 2), c);
            plot(bitmap, Vector(x0 + 2, y0 + 2), c);
        }
    }
    void plot(Bitmap<DWORD> bitmap, Vector p, DWord c)
    {
        if (p.inside(bitmap.size()))
            bitmap[p] = c;
    }
    bool operator<(const Particle& other) const { return _position.z > other._position.z; }
    void transform(double* matrix)
    {
        Vector3<double> c = (_colour - Vector3<double>(128.0, 128.0, 128.0))/128.0;
        _position.x = c.x*matrix[0] + c.y*matrix[1] + c.z*matrix[2];
        _position.y = c.x*matrix[3] + c.y*matrix[4] + c.z*matrix[5];
        _position.z = c.x*matrix[6] + c.y*matrix[7] + c.z*matrix[8];
    }

    Colour _colour;
    Vector3<double> _position;
    bool _big;
};

CGASimulator simulator;

class GamutBitmapWindow : public BitmapWindow
{
public:
    GamutBitmapWindow()
      : _lButton(false), _rButton(false), _rPosition(1000, 1000)
    {
        _matrix[0] = 1; _matrix[1] = 0; _matrix[2] = 0;
        _matrix[3] = 0; _matrix[4] = 1; _matrix[5] = 0;
        _matrix[6] = 0; _matrix[7] = 0; _matrix[8] = 1;
    }
    void create()
    {
        setSize(Vector(640, 480));

        _particles.allocate(65536);
        int i;
        for (i = 0; i < 65536; ++i) {
            _particles[i]._colour = simulator.decode(i);
            _particles[i]._big = true;
        }
        line(Colour(0, 0, 0), Colour(255, 0, 0));
        line(Colour(0, 0, 0), Colour(0, 255, 0));
        line(Colour(0, 0, 0), Colour(0, 0, 255));
        line(Colour(255, 0, 0), Colour(255, 255, 0));
        line(Colour(255, 0, 0), Colour(255, 0, 255));
        line(Colour(0, 255, 0), Colour(255, 255, 0));
        line(Colour(0, 255, 0), Colour(0, 255, 255));
        line(Colour(0, 0, 255), Colour(0, 255, 255));
        line(Colour(0, 0, 255), Colour(255, 0, 255));
        line(Colour(255, 255, 0), Colour(255, 255, 255));
        line(Colour(255, 0, 255), Colour(255, 255, 255));
        line(Colour(0, 255, 255), Colour(255, 255, 255));

        BitmapWindow::create();
        _animation.setWindow(this);
        _animation.start();
    }
    void paint(PaintHandle* paint)
    {
        _animation.onPaint();
        draw();
        BitmapWindow::paint(paint);
    }
    void draw()
    {
        _bitmap.fill(0);
        for (int i = 0; i < _particles.count(); ++i)
            _particles[i].transform(_matrix);
        std::sort(&_particles[0], &_particles[_particles.count()]);
        for (int i = 0; i < _particles.count(); ++i)
            _particles[i].plot(_bitmap, _rPosition);
    }
    void line(Colour c1, Colour c2)
    {
        for (int i = 0; i < 101; ++i) {
            Particle p;
            p._colour = c1*(i*0.01) + c2*((100-i)*0.01);
            p._big = false;
            _particles.append(p);
        }
    }
    bool mouseInput(Vector position, int buttons)
    {
        bool mouseDown = false;
        if ((buttons & MK_LBUTTON) != 0 && !_lButton) {
            _lButton = true;
            mouseDown = true;
            _lastPosition = position;
        }
        if ((buttons & MK_LBUTTON) == 0)
            _lButton = false;
        if ((buttons & MK_RBUTTON) != 0 && !_rButton) {
            _rButton = true;
            mouseDown = true;
            _lastPosition = position;
        }
        if ((buttons & MK_RBUTTON) == 0)
            _rButton = false;
        if (_lButton && position != _lastPosition) {
            Vector delta = position - _lastPosition;
            _lastPosition = position;
            double theta = delta.x*0.01;
            double phi = delta.y*0.01;

            _rotor = Rotor3<double>::yz(-phi)*Rotor3<double>::zx(theta)*_rotor;
            _rotor.toMatrix(_matrix);
        }
        if (_rButton && position != _lastPosition) {
            _rPosition += (position - _lastPosition);
            _lastPosition = position;
        }

        return mouseDown;
    }

    AnimationThread _animation;
    Rotor3<double> _rotor;
    double _matrix[9];
    AppendableArray<Particle> _particles;
    Vector _lastPosition;
    Vector _rPosition;
    bool _lButton;
    bool _rButton;
};

class GamutWindow : public RootWindow
{
public:
    void create()
    {
        setText("CGA Gamut");
        setSize(Vector(640, 480));
        add(&_bitmap);
        RootWindow::create();
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            _bitmap.remove();
    }

private:
    GamutBitmapWindow _bitmap;
};

class Program : public WindowProgram<GamutWindow>
{
public:
    void run()
    {
        simulator._newCGA = false;
        simulator._fixPrimaries = false;
        bool unknownArgument = false;
        for (int i = 1; i < _arguments.count(); ++i) {
            String argument = _arguments[i];
            CharacterSource s(argument);
            if (s.get() == '-') {
                int o = s.get();
                if (o == 'n') {
                    simulator._newCGA = true;
                    continue;                    
                }
                if (o == 'f') {
                    simulator._fixPrimaries = true;
                    continue;                    
                }
            }
            unknownArgument = true;
            break;
        }
        if (unknownArgument) {
            console.write("Syntax: " + _arguments[0] +
                " [options]\n");
            console.write("Options are:\n");
            console.write("  -n - use new CGA card type\n");
            console.write("  -f - correct for sRGB/NTSC primaries\n");
            return;
        }

        simulator._hue = 0;
        simulator._saturation = 0.8194739483137471237842610024737;
        simulator._brightness = (255 - (41.900353 + 213.147241))/(2*255);
        simulator._contrast = 0.8515625 * 256/256.031452;
        simulator._brightness = 0.5 - 0.5*simulator._contrast + simulator._brightness*simulator._contrast;

        printf("Preset: brightness = %f, contrast = %f, saturation = %f\n",simulator._brightness,simulator._contrast,simulator._saturation);

#if 0
        AutoHandle h = File("output.dat").openRead();
        h.read(reinterpret_cast<Byte*>(_tSamples), 1024*sizeof(double));

        double low = 1e99;
        double high = -1e99;
        for (int i = 0; i < 256; ++i) {
            double s;
            s = _tSamples[((i & 0xe0) << 1) | (i & 0x1f)];
            low = min(s, low);
            high = max(s, high);
            simulator._chroma[i] = s;
        }
        printf("Low = %lf, High = %lf\n", low, high);

        printf("unsigned char chroma[256] = {\n");
        for (int i = 0; i < 256; ++i) {
            if ((i & 15) == 0)
                printf("    ");
            printf("%3i", static_cast<int>((simulator._chroma[i] - low)*256.0/(high-low)));
            if (i != 255) {
                printf(",");
                if ((i & 15) == 15)
                    printf("\n");
                else
                    if ((i & 3) == 3)
                        printf(" ");
            }
        }
        printf("};\n");
#else
        static Byte chromaData[256] = {
             65, 11, 62,  6, 121, 87, 63,  6,  60,  9,120, 65,  61, 59,129,  5,
            121,  6, 58, 58, 134, 65, 62,  6,  57,  9,108, 72, 126, 72,125, 77,
             60, 98,160,  6, 113,195,194,  8,  53, 94,218, 64,  56,152,225,  5,
            118, 90,147, 56, 115,154,156,  0,  52, 92,197, 73, 107,156,213, 62,
            119, 10, 97,122, 178, 77, 60, 87, 119, 12,174,205, 119, 58,135, 88,
            185,  6, 54,158, 194, 67, 57, 87, 114, 10,101,168, 181, 67,114,160,
             64,  8,156,109, 121, 73,177,122,  58,  8,244,207,  65, 58,251,137,
            127,  5,141,156, 126, 58,144, 97,  57,  7,189,168, 106, 55,201,162,
            163,124, 62, 10, 185,159, 59,  8, 135,104,128, 80, 119,142,140,  5,
            241,141, 59, 57, 210,160, 61,  5, 137,108,103, 61, 177,140,110, 65,
             59,107,124,  4, 180,201,122,  6,  52,104,194, 77,  55,159,197,  3,
            130,128,121, 51, 174,197,123,  3,  52,100,162, 62, 101,156,171, 51,
            173, 11, 60,113, 199, 93, 58, 77, 167, 11,118,196, 132, 63,129, 74,
            256,  9, 54,195, 192, 55, 59, 74, 183, 14,103,199, 206, 74,118,154,
            153,108,156,105, 255,202,188,123, 143,107,246,203, 164,208,250,129,
            209,103,148,157, 253,195,171,120, 163,106,196,207, 245,202,249,208};
        for (int i = 0; i < 256; ++i)
            simulator._chroma[i] = chromaData[i]*(0.727546-0.070565)/256.0+0.070565;
#endif

        int clips = 0;
        double maxSS = 0;
        simulator.init();
        double yBlack = 0;
        double yWhite = 0;
        for (int block = 0; block < 1024; ++block) {
            int rgbi[4];
            rgbi[0] = ((block & 8) != 0 ? (block >> 4) : (block >> 8)) & 15;
            rgbi[1] = ((block & 4) != 0 ? (block >> 4) : (block >> 8)) & 15;
            rgbi[2] = ((block & 2) != 0 ? (block >> 4) : (block >> 8)) & 15;
            rgbi[3] = ((block & 1) != 0 ? (block >> 4) : (block >> 8)) & 15;
            int i = (rgbi[0]<<12) | (rgbi[1]<<8) | (rgbi[2]<<4) | rgbi[3];
            
            Colour rgb = simulator.decode(i);
            double r = rgb.x;
            double g = rgb.y;
            double b = rgb.z;
            double y = 0.299*r + 0.587*g + 0.114*b;
            bool clip = false;
            if (r < 0 || r >= 256 || g < 0 || g >= 256 || b < 0 || b >= 256) {
                ++clips;
                clip = true;
            }
            if (i == 0)
                yBlack = y;
            if (i == 65535)
                yWhite = y;

            double rr = r - y;
            double gg = g - y;
            double bb = b - y;
            double ss = Vector3<double>(rr, gg, bb).modulus();
            if (ss > maxSS) {
                printf("Colour 0x%04x has saturation %f\n",i,ss);
                maxSS = ss;
            }
            if (clip && clips == 1)
                printf("Clipping 0x%04x, r=%f, g=%f, b=%f\n",i,r,g,b);
        }
        printf("Black level %f\n",yBlack);
        printf("White level %f\n",yWhite);
        printf("Max SS = %f\n",maxSS);
        printf("%i colours clipped\n",clips);


        WindowProgram::run();
    }
private:
    double _tSamples[1024];
};
