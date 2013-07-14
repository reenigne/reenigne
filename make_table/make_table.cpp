#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    int cordic(int beta, int n)
    {
        if (beta < -512 || beta > 512) {
            int v;
            if (beta < 0)
                v = cordic(beta + 1024, n);
            else
                v = cordic(beta - 1024, n);
            return -v;
        }
        static const double angles[] = {
            0.78539816339745, 0.46364760900081, 0.24497866312686,
            0.12435499454676, 0.06241880999596, 0.03123983343027,
            0.01562372862048, 0.00781234106010, 0.00390623013197, 
            0.00195312251648, 0.00097656218956, 0.00048828121119,
            0.00024414062015, 0.00012207031189, 0.00006103515617,
            0.00003051757812, 0.00001525878906, 0.00000762939453,
            0.00000381469727, 0.00000190734863, 0.00000095367432,
            0.00000047683716, 0.00000023841858, 0.00000011920929,
            0.00000005960464, 0.00000002980232, 0.00000001490116,
            0.00000000745058};
        static const double Kvalues[] = {
            0.70710678118655, 0.63245553203368, 0.61357199107790,
            0.60883391251775, 0.60764825625617, 0.60735177014130,
            0.60727764409353, 0.60725911229889, 0.60725447933256,
            0.60725332108988, 0.60725303152913, 0.60725295913894,
            0.60725294104140, 0.60725293651701, 0.60725293538591,
            0.60725293510314, 0.60725293503245, 0.60725293501477,
            0.60725293501035, 0.60725293500925, 0.60725293500897,
            0.60725293500890, 0.60725293500889, 0.60725293500888};
        int Kn = Kvalues[min(n, 23)]*0x100 + 0.5;
        int vx = 0x100;
        int vy = 0;
        int poweroftwo = 8;
        int angle = angles[0]*2048/tau + 0.5;
        for (int j = 0; j < n; ++j) {
            int sigma;
            if (beta < 0)
                sigma = -0x100;
            else
                sigma = 0x100;
            int vxn = vx - ((sigma*vy)>>poweroftwo);
            vy = vy + ((sigma*vx)>>poweroftwo);
            vx = vxn;
            beta -= (sigma*angle)>>8;
            ++poweroftwo;
            if (j + 1 > 27)
                angle /= 2;
            else
                angle = angles[j+1]*2048/tau + 0.5;
        }
        return (vy*Kn)>>8;
    }

    void run()
    {
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

        for (int i = 0; i < 2560; ++i) {
            if ((i & 7) == 0)
                printf("  ");
            int fp = static_cast<int>(256*sin(i*tau/2048) + 256.5) - 256;
            printf("0x%04x, ", fp & 0xffff);
            //int c = cordic(i, 9);
            //printf("0x%04x, ", c & 0xffff);
            if ((i & 7) == 7)
                printf("\n");
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
};