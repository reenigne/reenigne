#include <stdio.h>
#define _USE_MATH_DEFINES 1
#include <math.h>
#include "alfe/minimum_maximum.h"

//static const int width = 80;
//static const int height = 100;
//
//Point points[width*height];
//
//class Point
//{
//public:
//    bool used() { return _used; }
//    void setNext(int next)
//    {
//        _next = next;
//        points[_next]._previous = _number;
//        _used = true;
//    }
//
//    int _number;
//    int _next;
//    int _previous;
//    bool _used;
//};

int main()
{
    for (int i = 0; i < 1024; ++i) {
        printf("%3i, ", static_cast<int>(40*sin(i*M_PI*2/1024) + 39.5));
        //if (i % 16 == 15)
        //    printf("\n");
    }
    //for (int i = 0; i < width*height; ++i) {
    //    points[i]._number = i;
    //    points[i]._next = (i + 1) % (width*height);
    //    points[i]._previous = (i + width*height - 1) % (width*height);
    //    points[i]._used = false;
    //}

    //for (int i = 0; i < width*height; ++i) {
    //    printf("%4i, ", points[i]._next);
    //    if (i % 8 == 7)
    //        printf("\n");
    //}
}
