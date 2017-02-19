#include "alfe/main.h"
#include "alfe/vectors.h"
#include "alfe/user.h"
#include "alfe/bitmap.h"

typedef Vector3<float> Point;

Point cubeCorners[8] = {
    Point(-1, -1, -1),
    Point(-1, -1,  1),
    Point(-1,  1, -1),
    Point(-1,  1,  1),
    Point( 1, -1, -1),
    Point( 1, -1,  1),
    Point( 1,  1, -1),
    Point( 1,  1,  1)};

class Quad
{
public:
    Quad(int p0, int p1, int p2, int p3, int colour)
      : _colour(colour)
    {
        _points[0] = p0;
        _points[1] = p1;
        _points[2] = p2;
        _points[3] = p3;
    }

    int _points[4];
    int _colour;
};

Quad cubeFaces[6][4] = {
    Quad(0, 4, 6, 2, 1),
    Quad(4, 5, 7, 6, 2),
    Quad(5, 1, 3, 7, 1),
    Quad(1, 0, 2, 3, 2),
    Quad(2, 6, 7, 3, 3),
    Quad(0, 1, 5, 4, 3)
};

class SpanWindow;

class SpanBitmapWindow : public BitmapWindow
{
public:
    void setSpanWindow(SpanWindow* window)
    {
        _spanWindow = window;
    }
    void paint()
    {
        _spanWindow->restart();
    }
    virtual void draw()
    {
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(Vector(320, 200));

        _bitmap.fill(0);


        _bitmap = setNextBitmap(_bitmap);
        invalidate();
    }
private:
    SpanWindow* _spanWindow;
    Bitmap<DWORD> _bitmap;
};

class SpanWindow : public RootWindow
{
public:
    SpanWindow()
    {
        _bitmap.setSpanWindow(this);

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(&_bitmap);
        _animated.setRate(60);
    }
    void restart() { _animated.restart(); }
    void create()
    {
        setText("CGA Span buffer");
        setInnerSize(Vector(320, 200));
        _bitmap.setTopLeft(Vector(0, 0));
        RootWindow::create();
        _animated.start();
    }
private:
    SpanBitmapWindow _bitmap;
    AnimatedWindow _animated;
};

class Program : public WindowProgram<SpanWindow>
{
};