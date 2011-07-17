#ifndef INCLUDED_PERCEPTUAL_H
#define INCLUDED_PERCEPTUAL_H

#include "unity/vectors.h"

// CIELAB L*a*b* conversions from http://en.wikipedia.org/wiki/Lab_color_space

double xyzFromLabHelper(double t)
{
    static const double d = 6.0/29.0;
    return t > d ? pow(t, 3.0) : (t - 4.0/29.0)*3.0*d*d;
}

Vector3<double> xyzFromLab(const Vector3<double>& lab)
{
    double y = (lab.x + 16.0)/116.0;
    return Vector3<double>(
        xyzFromLabHelper(y + lab.y/500.0),
        xyzFromLabHelper(y),
        xyzFromLabHelper(y - lab.z/200.0));
}

double labFromXyzHelper(double t)
{
    static const double d = 6.0/29.0;
    return t > d*d*d ? pow(t, 1/3.0) : t/(3.0*d*d) + 4.0/29.0;
}

Vector3<double> labFromXyz(const Vector3<double>& xyz)
{
    double y = labFromXyzHelper(xyz.y);
    return Vector3<double>(
        116.0*y - 16.0,
        500.0*(labFromXyzHelper(xyz.x) - y),
        200.0*(y - labFromXyzHelper(xyz.z)));
}


// CIELUV L*u*v* conversions from
// http://en.wikipedia.org/wiki/CIELUV_color_space

Vector3<double> luvFromXyz(const Vector3<double>& xyz)
{
    double x = xyz.x;
    double y = xyz.y;
    double z = xyz.z;
    static const double d = 3.0/29.0;
    static const double d2 = d*2.0;
    double l;
    if (y <= d2*d2*d2)
        l = y/(d*d*d);
    else
        l = 116.0*pow(y, 1.0/3.0) - 16.0;
    double r = x + 15*y + 3*z;
    double u;
    double v;
    if (r < 1e-5) {
        u = 13.0*l*(4 - 0.2105);
        v = 13.0*l*(9.0/15.0 - 0.4737);
    }
    else {
        u = 13.0*l*(4*x/r - 0.2105);
        v = 13.0*l*(9*y/r - 0.4737);
    }
    return Vector3<double>(l, u, v);
}

Vector3<double> xyzFromLuv(const Vector3<double>& xyz)
{
    double l = xyz.x;
    double u = xyz.y;
    double v = xyz.z;
    double uu = u/(13*l) + 0.2105;
    double vv = v/(13*l) + 0.4737;
    double y;
    static const double d = 3.0/29.0;
    if (l <= 8)
        y = l*d*d*d;
    else {
        y = (l + 16)/116;
        y = y*y*y;
    }
    if (vv < 1e-5)
        return Vector3<double>(0, 0, 0);
    double x = y*(9*uu)/(4*vv);
    double z = y*(12 - 3*uu - 20*vv)/(4*vv);
    return Vector3<double>(x, y, z);
}


// sRGB conversions from http://en.wikipedia.org/wiki/SRGB

Vector3<double> xyzFromRgb(const Vector3<double>& rgb)
{
    return Vector3<double>(
        0.4124*rgb.x + 0.3576*rgb.y + 0.1805*rgb.z,
        0.2126*rgb.x + 0.7152*rgb.y + 0.0722*rgb.z,
        0.0193*rgb.x + 0.1192*rgb.y + 0.9505*rgb.z);
}

Vector3<double> rgbFromXyz(const Vector3<double>& xyz)
{
    return Vector3<double>(
         3.2410*xyz.x - 1.5374*xyz.y - 0.4986*xyz.z,
        -0.9692*xyz.x + 1.8760*xyz.y + 0.0416*xyz.z,
         0.0556*xyz.x - 0.2040*xyz.y + 1.0570*xyz.z);
}

double srgbFromRgbHelper(double t)
{
    return clamp(0.0,
        256.0*(t <= 0.0031308 ? 12.92*t : (1 + 0.055)*pow(t, 1.0/2.4) - 0.055),
        256.0);
}

Vector3<double> srgbFromRgb(const Vector3<double>& rgb)
{
    return Vector3<double>(
        srgbFromRgbHelper(rgb.x),
        srgbFromRgbHelper(rgb.y),
        srgbFromRgbHelper(rgb.z));
}

double rgbFromSrgbHelper(double t)
{
    t /= 256.0;
    return t <= 0.04045 ? t/12.92 : pow((t + 0.055)/(1 + 0.055), 2.4);
}

Vector3<double> rgbFromSrgb(const Vector3<double>& srgb)
{
    return Vector3<double>(
        rgbFromSrgbHelper(srgb.x),
        rgbFromSrgbHelper(srgb.y),
        rgbFromSrgbHelper(srgb.z));
}


// Putting it all together

Vector3<double> luvFromSrgb(const Vector3<double>& srgb)
{
    return luvFromXyz(xyzFromRgb(rgbFromSrgb(srgb)));
}

Vector3<double> srgbFromLuv(const Vector3<double>& lab)
{
    return srgbFromRgb(rgbFromXyz(xyzFromLuv(lab)));
}

Vector3<double> labFromSrgb(const Vector3<double>& srgb)
{
    return labFromXyz(xyzFromRgb(rgbFromSrgb(srgb)));
}

Vector3<double> srgbFromLab(const Vector3<double>& lab)
{
    return srgbFromRgb(rgbFromXyz(xyzFromLab(lab)));
}

double colourDistance2(const Vector3<double>& a, const Vector3<double>& b,
    bool luv = true)
{
    Vector3<double> aP;
    Vector3<double> bP;
    if (luv) {
        aP = luvFromSrgb(a);
        bP = luvFromSrgb(b);
    }
    else {
        aP = labFromSrgb(a);
        bP = labFromSrgb(b);
    }
    aP -= bP;
    return (aP - bP).modulus2();
}

#endif // INCLUDED_PERCEPTUAL_H
