#include "alfe/main.h"
#include <stdio.h>                  
#define _USE_MATH_DEFINES
#include <math.h>

// We have a periodic rectangular function f(x):
//   f(x) = {1 if frac(x) < duty
//          {0 otherwise
// Let fl(x) be the output of a filter that band limits to a frequency of 2:
//   fl(x) = a + b*sin(x*tau) + c*cos(x*tau) + d*sin(x*2*tau)
//
// a =   integral(0, 1, f(x)*dx) = duty
// b = 2*integral(0, 1, f(x)*sin(x*tau)*dx) = 2*integral(0, duty, sin(x*tau)*dx) = 2*(1-cos(x*tau))/tau
// c = 2*integral(0, 1, f(x)*cos(x*tau)*dx) = 2*integral(0, duty, cos(x*tau)*dx) = 2*sin(duty*tau)/tau
// d = 2*integral(0, 1, f(x)*sin(x*2*tau)*dx) = 2*integral(0, duty, sin(x*4*pi)*dx) = 2*(1-cos(2*tau*duty))/(2*tau)
//
// 

// We want to band-limit this function so that it contains no frequencies
// higher than 2, then sample it at 4 points.
// fl[k] = 

class Program : public ProgramBase
{
protected:
    // sinc(z) = sin(z)/z
    // Si(x) = integral(0, x, dz * sinc(z))
    // 
    // sincp(1) = sin(pi)/pi = 0
    // sincp(y) = sinc(pi*y)
    // z = pi*y
    // dz/dy = pi
    // dy = dz/pi
    // Si(x) = integral(0, x, dy * pi * sinc(pi*y))
    //
    //double Si(double x)
    //{
    //    double r = 0;
    //    double s = 1;
    //    double v = x;
    //    double x2 = x*x;
    //    double d = 1;
    //    for (int k = 1;; k += 2) {
    //        r += s*v/k;
    //        v *= x2/((k+1)*(k+2));
    //        if (v < 1e-6)
    //            break;
    //        s = -s;
    //    }
    //    return r;
    //}
    //double Sip(double x)
    //{
    //    return M_PI*Si(M_PI*x);
    //}
   
    void run()
    {
        double tau = 2*M_PI;
        double duty = 0.5;
        double a = duty;
        double b = 2.0*(1.0-cos(duty*tau))/tau;
        double c = 2.0*sin(duty*tau)/tau;
        double d = 2.0*(1.0-cos(duty*2*tau))/(2*tau);

        int phases[6] = {270, 135, 180, 0, 315, 90};

        for (int phase = 0; phase < 6; ++phase) {
            for (int quadrant = 0; quadrant < 4; ++quadrant) {
                double x = phases[phase]/360.0 + quadrant/4.0;
                double v = a + b*sin(x*tau) + c*cos(x*tau) + d*sin(x*2*tau);
                printf("%lf ",v);
            }
            printf("\n");
        }
    }
};