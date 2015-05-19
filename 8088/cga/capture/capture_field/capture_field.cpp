#include "alfe/main.h"
#include "alfe/complex.h"
#include "alfe/terminal6.h"
#include "alfe/bitmap_png.h"
#include "alfe/evaluate.h"
#include "alfe/ntsc_decode.h"

//class Slider
//{
//public:
//    Slider() { }
//    Slider(double low, double high, double initial, String caption, float* p, int max = 512)
//      : _low(low), _high(high), _caption(caption), _p(p), _max(max)
//    {
//        *p = initial;
//        _dragStartX = (initial - low)*max/(high - low);
//    }
//    void setBitmap(Bitmap<SRGB> bitmap) { _bitmap = bitmap; }
//    void slideTo(int x)
//    {
//        if (x < 0)
//            x = 0;
//        if (x >= _max)
//            x = _max - 1;
//        _dragStartX = x;
//        *_p = (_high - _low)*x/_max + _low;
//        draw();
//    }
//    void draw()
//    {
//        char buffer[0x20];
//        for (int i = 0; i < 0x20; ++i)
//            buffer[i] = 0;
//        sprintf(buffer, ": %lf", *_p);
//        int x = 0;
//        for (; x < _caption.length(); ++x)
//            drawCharacter(x, _caption[x]);
//        for (int x2 = 0; x2 < 0x20; ++x2)
//            drawCharacter(x + x2, buffer[x2]);
//    }
//
//    void drawCharacter(int x, char c)
//    {
//        for (int y = 0; y < 8; ++y)
//            for (int xx = 0; xx < 6; ++xx)
//                _bitmap[Vector(x*6 + xx, y)] = (((glyphs[c*8 + y] << xx) & 0x80) != 0 ? SRGB(0xff, 0xff, 0x00) : SRGB(0, 0, 0xff));
//    }
//
//    int currentX() { return _dragStartX; }
//private:
//    double _low;
//    double _high;
//    String _caption;
//    float* _p;
//    int _dragStartX;
//    int _max;
//
//    Bitmap<SRGB> _bitmap;
//};
//
//class CalibrateImage;
//typedef RootWindow<Window> RootWindow2;
//typedef ImageWindow<RootWindow2, CalibrateImage> ImageWindow2;
//
//class CalibrateImage : public Image
//{
//public:
//    void setWindow(ImageWindow2* window, NTSCDecoder* decoder)
//    {
//        _output = Bitmap<SRGB>(Vector(1536, 1024));
//
//        _window = window;
//        _decoder = decoder;
//
//        _sliderCount = 6;
//        //_sliders[0] = Slider(0, 1, 0.25, "saturation", &decoder->saturation);
//        //_sliders[1] = Slider(-180, 180, 0, "hue", &decoder->hue);
//        //_sliders[2] = Slider(-25500, 255, -72.8, "brightness", &decoder->brightness);
//        //_sliders[3] = Slider(0, 100, 1.97, "contrast", &decoder->contrast);
//        //_sliders[4] = Slider(0, 0.01, 0, "wobbleAmplitude", &decoder->wobbleAmplitude);
//        //_sliders[5] = Slider(0, 1, 0.5, "wobblePhase", &decoder->wobblePhase);
//        _sliders[0] = Slider(0, 1, 0.303, "saturation", &decoder->saturation);
//        _sliders[1] = Slider(-180, 180, 0, "hue", &decoder->hue);
//        _sliders[2] = Slider(-255, 255, -11.0 /* -72.8*/, "brightness", &decoder->brightness);
//        _sliders[3] = Slider(0, 4, 1.41, "contrast", &decoder->contrast);
//        _sliders[4] = Slider(0, 0.01, 0.0042, "wobbleAmplitude", &decoder->wobbleAmplitude);
//        _sliders[5] = Slider(0, 1, 0.94, "wobblePhase", &decoder->wobblePhase);
//
//        for (int i = 0; i < _sliderCount; ++i) {
//            _sliders[i].setBitmap(_output.subBitmap(Vector(1024, i*8), Vector(512, 8)));
//            _sliders[i].draw();
//        }
//        _slider = -1;
//    }
//
//    virtual void draw()
//    {
//        _decoder->decode();
//
//        {
//            Byte* row = _output.data();
//            const Byte* otherRow = _decoder->_output;
//            for (int y = 0; y < 240; ++y) {
//                SRGB* p = reinterpret_cast<SRGB*>(row);
//                const SRGB* op = reinterpret_cast<const SRGB*>(otherRow);
//                for (int x = 0; x < 640; ++x) {
//                    *p = *op;
//                    ++p;
//                    ++op;
//                }
//                row += _output.stride();
//                otherRow += 640*3;
//            }
//        }
//
//        Vector sz = size();
//        if (sz.x > 1536)
//            sz.x = 1536;
//        if (sz.y > 1024)
//            sz.y = 1024;
//
//        Byte* row = data();
//        const Byte* otherRow = _output.data();
//        for (int y = 0; y < sz.y; ++y) {
//            DWORD* p = reinterpret_cast<DWORD*>(row);
//            const SRGB* op = reinterpret_cast<const SRGB*>(otherRow);
//            for (int x = 0; x < sz.x; ++x) {
//                *p = (op->x << 16) | (op->y << 8) | op->z;
//                ++p;
//                ++op;
//            }
//            row += stride();
//            otherRow += _output.stride();
//        }
//    }
//
//    bool buttonDown(Vector position)
//    {
//        Vector p = position - Vector(1024, 0);
//        if (p.inside(Vector(512, _sliderCount*8))) {
//            _dragStart = position;
//            _slider = p.y/8;
//            _dragStartX = _sliders[_slider].currentX();
//            mouseMove(position);
//            return true;
//        }
//        return false;
//    }
//
//    void buttonUp(Vector position)
//    {
//        mouseMove(position);
//        _slider = -1;
//    }
//
//    void mouseMove(Vector position)
//    {
//        if (_slider != -1) {
//            Vector p = position -= _dragStart;
//            _sliders[_slider].slideTo(_dragStartX + p.x - p.y);
//            draw();
//            _window->invalidate();
//        }
//    }
//    Bitmap<SRGB> _output;
//
//    ImageWindow2* _window;
//    NTSCDecoder* _decoder;
//
//    Slider _sliders[19];
//    int _slider;
//    int _sliderCount;
//
//    Vector _dragStart;
//    int _dragStartX;
//};
//
//template<class Base> class CalibrateWindow : public Base
//{
//public:
//    class Params
//    {
//        friend class CalibrateWindow;
//    public:
//        Params(typename Base::Params bp)
//          : _bp(bp)
//        { }
//    private:
//        typename Base::Params _bp;
//    };
//
//    CalibrateWindow() { }
//
//    void create(Params p, NTSCDecoder* decoder)
//    {
//        Base::create(p._bp);
//        _image->setWindow(this, decoder);
//    }
//
//    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
//    {
//        switch (uMsg) {
//            case WM_LBUTTONDOWN:
//                if (_image->buttonDown(vectorFromLParam(lParam)))
//                    SetCapture(_hWnd);
//                break;
//            case WM_LBUTTONUP:
//                ReleaseCapture();
//                _image->buttonUp(vectorFromLParam(lParam));
//                break;
//            case WM_MOUSEMOVE:
//                _image->mouseMove(vectorFromLParam(lParam));
//                break;
//            case WM_KILLFOCUS:
//                ReleaseCapture();
//                break;
//        }
//        return Base::handleMessage(uMsg, wParam, lParam);
//    }
//private:
//    static Vector vectorFromLParam(LPARAM lParam)
//    {
//        return Vector(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
//    }
//};


class Program : public ProgramBase
{
public:
    void run()
    {
        String name = "captured.png";
        int frames = 1;
        if (_arguments.count() >= 2) {
            name = _arguments[1];
            if (_arguments.count() >= 3)
                frames = evaluate<int>(_arguments[2]);
        }
        //String inName = "q:\\input.raw";
        //String inName = "q:\\bottom.raw";
        //String inName = "q:\\6cycle.raw";
        //AutoHandle h = File(inName, true).openRead();
        AutoHandle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);

        int samples = 450*1024*frames;
        int sampleSpaceBefore = 256;
        int sampleSpaceAfter = 256;
        Array<Byte> buffer(sampleSpaceBefore + samples + sampleSpaceAfter);
        Byte* b = &buffer[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i)
            b[i - sampleSpaceBefore] = 0;
        for (int i = 0; i < sampleSpaceAfter; ++i)
            b[i + samples] = 0;
        for (int i = 0; i < 450*frames; ++i)
            h.read(&b[i*1024], 1024);

        Bitmap<SRGB> decoded(Vector(640, 240));

        NTSCCaptureDecoder<SRGB> decoder;
        decoder.setBuffers(b, decoded);

#if 1
        decoder.decode();
        PNGFileFormat<SRGB> png;
        Bitmap<SRGB> output(Vector(640, 480));

        //float z0 = 0;
        //for (int o = 0; o < 480; ++o) {
        //    //float a = 0;
        //    float t = 0;
        //    float fk = o*240.0/480.0;
        //    int k = fk;
        //    fk -= k;
        //    int firstInput = fk - 3;
        //    int lastInput = fk + 3;
        //    Vector3<float> a[640];
        //    for (int x = 0; x < 640; ++x)
        //        a[x] = Vector3<float>(0, 0, 0);
        //    for (int j = firstInput; j <= lastInput; ++j) {
        //        float s = lanczos(j + z0);
        //        int y = j+k;
        //        if (y >= 0 && y < 240) {
        //            for (int x = 0; x < 640; ++x) {
        //                SRGB srgb = decoded[Vector(x, y)];
        //                a[x] += Vector3Cast<float>(srgb)*s;
        //            }
        //            t += s;
        //        }
        //    }
        //    for (int x = 0; x < 640; ++x) {
        //        Vector3<float> c = a[x]/t;
        //        output[Vector(x, o)] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
        //    }
        //    int k1 = (o + 1)*240.0/480.0;
        //    z0 += (k1 - k) - 240.0/480.0;
        //}
        for (int o = 0; o < 478; o += 2) {
            for (int x = 0; x < 640; ++x) {
                output[Vector(x, o)] = decoded[Vector(x, o/2)];
                Vector3<int> a = Vector3Cast<int>(decoded[Vector(x, o/2)]) + Vector3Cast<int>(decoded[Vector(x, o/2 + 1)]);
                output[Vector(x, o+1)] = Vector3Cast<Byte>(a/2);
                //output[Vector(x, o+1)] = decoded[Vector(x, o/2)];
            }
        }
        for (int x = 0; x < 640; ++x) {
            output[Vector(x, 478)] = decoded[Vector(x, 239)];
            output[Vector(x, 479)] = decoded[Vector(x, 239)];
        }

        png.save(output, File(name, true));

        AutoHandle out = File(name + ".raw", true).openWrite();
        out.write(b, 450*1024);
#else
        CalibrateImage image;

        Window::Params wp(&_windows, L"CGA Calibration", Vector(1536, 1024));
        RootWindow2::Params rwp(wp);
        ImageWindow2::Params iwp(rwp, &image);
        typedef CalibrateWindow<ImageWindow2> CalibrateWindow;
        CalibrateWindow::Params cwp(iwp);
        CalibrateWindow window;
        window.create(cwp, &decoder);

        window.show(_nCmdShow);
        pumpMessages();
#endif
    }
};
