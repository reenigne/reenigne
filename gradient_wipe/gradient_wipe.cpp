#include "alfe/main.h"

class GradientTask : public ThreadTask
{
public:
    GradientTask(Array<Byte>* gradient) : _gradient(gradient), _count(0) { }
    void run()
    {
        while (!cancelling()) {
            int i = rand() % 8000;
            int j = rand() % 8000;
            int metricUnswapped = metric(i, j);
            swap(i, j);
            int metricSwapped = metric(i, j);
            if (metricSwapped > metricUnswapped)
                swap(i, j);
            ++_count;
        }
    }
    int count() { return _count; }
private:
    void swap(int i, int j)
    {
        Byte t = (*_gradient)[i];
        (*_gradient)[i] = (*_gradient)[j];
        (*_gradient)[j] = t;
    }
    int metric(int i, int j)
    {
        return metricForPoint(i) + metricForPoint(j);
    }
    int metricForPoint(int i)
    {
        int x = i % 80;
        int y = i / 80;
        int h = secondDerivative(x, y, (x + 1)%80, y, (x + 79)%80, y);
        int v = secondDerivative(x, y, x, (y + 1)%100, x, (y + 99)%100);
        return h*h + v*v;
    }
    int secondDerivative(int x1, int y1, int x0, int y0, int x2, int y2)
    {
        return getByte(x2, y2) + getByte(x0, y0) - 2*getByte(x1, y1);
    }
    int getByte(int x, int y) { return (*_gradient)[y*80 + x]; }

    Array<Byte>* _gradient;
    int _count;
};

class GradientWipeWindow : public RootWindow
{
public:
    GradientWipeWindow() : _task(&_gradientWipe)
    {
        _outputSize = Vector(80, 100);
        _gradientWipe.allocate(8000);
        for (int i = 0; i < 8000; ++i)
            _gradientWipe[i] = (i*256)/8000;
        for (int i = 0; i < 8000 - 3; ++i) {
            int j = (rand() % (8000 - i)) + i;
            Byte t = _gradientWipe[i];
            _gradientWipe[i] = _gradientWipe[j];
            _gradientWipe[j] = t;
        }

        add(&_bitmapWindow);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(60);
    }
    ~GradientWipeWindow() { _task.join(); }
    void create()
    {
        setText("Gradient wipe");
        setInnerSize(_outputSize);
        _bitmapWindow.setTopLeft(Vector(0, 0));
        _bitmapWindow.setInnerSize(_outputSize);

        RootWindow::create();
        _animated.start();
        _task.restart();
    }
    virtual void draw()
    {
        _bitmap.ensure(_outputSize);
        Byte* outputRow = _bitmap.data();
        for (int y = 0; y < _outputSize.y; ++y) {
            DWORD* output = reinterpret_cast<DWORD*>(outputRow);
            for (int x = 0; x < _outputSize.x; ++x) {
                Byte b = _gradientWipe[y*80 + x];
                *output = (b << 16) | (b << 8) | b;
                ++output;
            }
            outputRow += _bitmap.stride();
        }
        _bitmap = _bitmapWindow.setNextBitmap(_bitmap);

        _animated.restart();
        printf("%i\n",_task.count());
    }
private:
    AnimatedWindow _animated;
    BitmapWindow _bitmapWindow;
    Vector _outputSize;
    GradientTask _task;
    Bitmap<DWORD> _bitmap;

    Array<Byte> _gradientWipe;
};


class Program : public WindowProgram<GradientWipeWindow>
{
};