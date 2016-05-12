#include "alfe/main.h"

#ifndef INCLUDED_TIMER_H
#define INCLUDED_TIMER_H

#include <MMSystem.h>

class Timer
{
public:
    Timer()
    {
        QueryPerformanceCounter(&_startTime);
    }
    void output(String caption)
    {
        LARGE_INTEGER time;
        QueryPerformanceCounter(&time);
        time.QuadPart -= _startTime.QuadPart;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        console.write(caption + ": " + decimal(static_cast<int>(
            (time.QuadPart*1000000)/frequency.QuadPart)) +
            " microseconds\n");
        //printf("%lf us\n",time.QuadPart*1000000.0/frequency.QuadPart);
    }
private:
    LARGE_INTEGER _startTime;
};

//class Timer
//{
//public:
//    Timer()
//    {
//        _ms = timeGetTime();
//    }
//    void output(String caption)
//    {
//        DWORD ms = timeGetTime();
//        printf("%i ms\n",ms - _ms);
//    }
//private:
//    DWORD _ms;
//};

#endif // INCLUDED_TIMER_H
