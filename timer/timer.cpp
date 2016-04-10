#include "alfe/main.h"
#include <mmsystem.h>

class Program : public ProgramBase
{
public:
    DWORD msFromWallClock(int hour, int minute, int second = 0, int ms = 0)
    {
        return (((hour*60) + minute)*60 + second)*1000 + ms;
    }
    void beep()
    {
        PlaySound(L"C:\\t\\timer.wav", NULL, SND_FILENAME);
    }
    void run()
    {
        static const int secondsToWindow = 10;
        static const int stareSeconds = 20;
        do {
            SYSTEMTIME t;
            GetLocalTime(&t);
            DWORD ms = msFromWallClock(t.wHour, t.wMinute, t.wSecond,
                t.wMilliseconds);
            ms -= msFromWallClock(t.wHour, t.wMinute - t.wMinute % 20);
            if (ms > msFromWallClock(0, 15) ||
                ms < msFromWallClock(0, 5)) {
                beep();
                Sleep(secondsToWindow*1000);
                beep();
                Sleep(stareSeconds*1000);
                beep();
                GetLocalTime(&t);
                ms = msFromWallClock(t.wHour, t.wMinute, t.wSecond,
                    t.wMilliseconds);
                if (ms > msFromWallClock(17, 55))
                    ms = msFromWallClock(24 + 9, 0) - ms;
                else {
                    ms -= msFromWallClock(t.wHour, t.wMinute - t.wMinute % 20);
                    if (t.wMinute % 20 < 10)
                        ms = msFromWallClock(0, 10) - ms;
                    else
                        ms = msFromWallClock(0, 30) - ms;
                }
            }
            else {
                beep();
                GetLocalTime(&t);
                ms = msFromWallClock(0, 20) - msFromWallClock(0,
                    t.wMinute % 20, t.wSecond, t.wMilliseconds);
            }
            Sleep(ms);
        } while (true);
    }
};