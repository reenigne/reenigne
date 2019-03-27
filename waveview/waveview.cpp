#include "alfe/main.h"

class WaveViewThread : public ThreadTask
{
};

class WaveViewWindow : public RootWindow
{
public:
    WaveViewWindow()
    {
        setText("Wave viewer");
        add(&_bitmap);
        add(&_animated);
        _animated.setDrawWindow(this);
        _animated.setRate(60);
    }
    void create()
    {
        RootWindow::create();
        _animated.start();
    }
private:
    BitmapWindow _bitmap;
    AnimatedWindow _animated;
};

class Program : public WindowProgram<WaveViewWindow>
{
public:
    void run()
    {

        WindowProgram::run();
    }
};