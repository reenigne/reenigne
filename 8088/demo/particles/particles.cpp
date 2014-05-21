#include "alfe/main.h"

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

static const int pointCount = 16192;
static const int width = 80;
static const int height = 200;

class Program : public ProgramBase
{
public:
    void run()
    {
        shuffle();
        SInt16* factors;

        // Compute amount of space needed
        UInt32 f = 0x10000L;
        int n = 0;
        UInt16 ff = 0x110;
        do {
            f = (f << 8) / ff;
            if (f < 0x28f)
                break;
            ++n;
        } while (true);
        int m = 0;
        f = 0x10000L;
        do {
            f = (f * ff) >> 8;
            if (f >= 0x640000L)
                break;
            ++m;
        } while (true);
        factors = (SInt16*)malloc((1+n+m)*sizeof(SInt16));
        if (factors == 0) {
            printf("Not enough memory for factors array!\n");
            exit(1);
        }

        // Fill in factors array
        f = 0x10000L;
        int i = n;
        while (i >= 0) {
            factors[i--] = (SInt16)(f >> 8);
            f = (f << 8) / ff;
        }
        f = 0x10000L;
        i = n + 1;
        while (i < 1+n+m) {
            f = (f * ff) >> 8;
            factors[i++] = (SInt16)(f >> 8);
        }
        n = n + m + 1;

        // Compute trails
        usedList = 0xffff;
        do {
            point = freeList;
            if (point == 0xffff)
                break;
            SInt16 ox = (xCoordinate(point) - 40)<<8;
            SInt16 oy = (yCoordinate(point) - 100)<<8;
            for (int i = 0; i < n; ++i) {
                SInt16 x = (short)(((long)factors[i]*(long)ox) >> 8);
                SInt16 y = (short)(((long)factors[i]*(long)oy) >> 8);
                UInt16 p = pointFromCoordinates((x >> 8) + 40, (y >> 8) + 100);
                ++*(UInt8 far*)MK_FP(0xb800, p);
                if (x >= 0x2800 || x < -0x2800 || y >= 0x6400 || y < -0x6400)
                    break;
                place(p);
            }
        } while (true);
        free(factors);

    }
private:
    bool isVisible(int point)
    {
        return (point & 0x1fff) < width*height/2;
    }

    void shuffle()
    {
        UInt16 visible = 0;
        UInt16 p;
        // Fill pointsPrev with the indexes of visible points
        for (p = 0; p < pointCount; ++p)
            if (isVisible(p)) {
            pointsPrev[visible] = p;
            ++visible;
            }
        // Shuffle pointsPrev
        for (p = 0; p < visible - 1; ++p) {
            UInt16 q = rand() % (visible - p) + p;
            if (q != p) {
                UInt16 t = pointsPrev[p];
                pointsPrev[p] = pointsPrev[q];
                pointsPrev[q] = t;
            }
        }
        // Make a singly linked list from the index array.
        freeList = pointsPrev[0];
        for (p = 0; p < visible - 1; ++p)
            pointsNext[pointsPrev[p]] = pointsPrev[p + 1];
        pointsNext[pointsPrev[visible - 1]] = freeList;
        // Make the list doubly linked
        for (p = 0; p < pointCount; ++p)
            if (isVisible(p))
                pointsPrev[pointsNext[p]] = p;

        //    printf("Visible = %i\n",visible);
        //    for (p = 0; p < pointCount; ++p)
        //        if (isVisible(p)) {
        //            if (pointsPrev[pointsNext[p]] != p) {
        //                printf("pointsPrev[pointsNext[0x%04x]] = 0x%04x, pointsNext[0x%04x] = 0x%04x\n",p,pointsPrev[pointsNext[p]],p,pointsNext[p]);
        //                exit(1);
        //            }
        //            if (pointsNext[pointsPrev[p]] != p) {
        //                printf("pointsNext[pointsPrev[0x%04x]] = 0x%04x, pointsPrev[0x%04x] = 0x%04x\n",p,pointsNext[pointsPrev[p]],p,pointsPrev[p]);
        //                exit(1);
        //            }
        //        }
    }

};

//int main()
//{
//    for (int i = 0; i < 1024; ++i) {
//        printf("%3i, ", static_cast<int>(40*sin(i*tau/1024) + 39.5));
//        //if (i % 16 == 15)
//        //    printf("\n");
//    }
//    //for (int i = 0; i < width*height; ++i) {
//    //    points[i]._number = i;
//    //    points[i]._next = (i + 1) % (width*height);
//    //    points[i]._previous = (i + width*height - 1) % (width*height);
//    //    points[i]._used = false;
//    //}
//
//    //for (int i = 0; i < width*height; ++i) {
//    //    printf("%4i, ", points[i]._next);
//    //    if (i % 8 == 7)
//    //        printf("\n");
//    //}
//}
