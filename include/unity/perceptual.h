#ifndef INCLUDED_PERCEPTUAL_H
#define INCLUDED_PERCEPTUAL_H

#include "unity/vectors.h"

typedef Vector3<double> Colour;
typedef Vector3<UInt8> SRGB;

class PerceptualModelImplementation
{
public:
    virtual Colour perceptualFromXyz(const Colour& xyz) = 0;
    virtual Colour xyzFromPerceptual(const Colour& perceptual) = 0;
private:
};

// CIELUV L*u*v* conversions from
// http://en.wikipedia.org/wiki/CIELUV_color_space
class LUVPerceptualModelImplementation : public PerceptualModelImplementation
{
public:
    virtual Colour perceptualFromXyz(const Colour& xyz)
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
        return Colour(l, u, v);
    }

    virtual Colour xyzFromPerceptual(const Colour& luv)
    {
        double l = luv.x;
        double u = luv.y;
        double v = luv.z;
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
            return Colour(0, 0, 0);
        double x = y*(9*uu)/(4*vv);
        double z = y*(12 - 3*uu - 20*vv)/(4*vv);
        return Colour(x, y, z);
    }
private:
    LUVPerceptualModelImplementation() { }
    friend class PerceptualModel;
};

// CIELAB L*a*b* conversions from http://en.wikipedia.org/wiki/Lab_color_space
class LABPerceptualModelImplementation : public PerceptualModelImplementation
{
public:
    virtual Colour perceptualFromXyz(const Colour& xyz)
    {
        double y = labFromXyzHelper(xyz.y);
        return Colour(
            116.0*y - 16.0,
            500.0*(labFromXyzHelper(xyz.x) - y),
            200.0*(y - labFromXyzHelper(xyz.z)));
    }
    virtual Colour xyzFromPerceptual(const Colour& lab)
    {
        double y = (lab.x + 16.0)/116.0;
        return Colour(
            xyzFromLabHelper(y + lab.y/500.0),
            xyzFromLabHelper(y),
            xyzFromLabHelper(y - lab.z/200.0));
    }
private:
    double xyzFromLabHelper(double t)
    {
        static const double d = 6.0/29.0;
        return t > d ? pow(t, 3.0) : (t - 4.0/29.0)*3.0*d*d;
    }

    double labFromXyzHelper(double t)
    {
        static const double d = 6.0/29.0;
        return t > d*d*d ? pow(t, 1/3.0) : t/(3.0*d*d) + 4.0/29.0;
    }
    LABPerceptualModelImplementation() { }
    friend class PerceptualModel;
};

class PerceptualModel
{
public:
    PerceptualModel() { }
    static PerceptualModel luv() { return PerceptualModel(&_luv); }
    static PerceptualModel lab() { return PerceptualModel(&_lab); }
    Colour perceptualFromSrgb(const SRGB& srgb)
    {
        return
            _implementation->perceptualFromXyz(xyzFromRgb(rgbFromSrgb(srgb)));
    }
    Vector3<UInt8> srgbFromPerceptual(const Colour& perceptual)
    {
        return srgbFromRgb(
            rgbFromXyz(_implementation->xyzFromPerceptual(perceptual)));
    }
    // sRGB conversions from http://en.wikipedia.org/wiki/SRGB
    Colour xyzFromRgb(Colour& rgb)
    {
        return Colour(
            0.4124*rgb.x + 0.3576*rgb.y + 0.1805*rgb.z,
            0.2126*rgb.x + 0.7152*rgb.y + 0.0722*rgb.z,
            0.0193*rgb.x + 0.1192*rgb.y + 0.9505*rgb.z);
    }
    Colour rgbFromXyz(const Colour& xyz)
    {
        return Colour(
             3.2410*xyz.x - 1.5374*xyz.y - 0.4986*xyz.z,
            -0.9692*xyz.x + 1.8760*xyz.y + 0.0416*xyz.z,
             0.0556*xyz.x - 0.2040*xyz.y + 1.0570*xyz.z);
    }
    SRGB srgbFromRgb(const Colour& rgb)
    {
        return SRGB(
            srgbFromRgbHelper(rgb.x),
            srgbFromRgbHelper(rgb.y),
            srgbFromRgbHelper(rgb.z));
    }
    Colour rgbFromSrgb(const SRGB& srgb)
    {
        return Colour(
            rgbFromSrgbHelper(srgb.x),
            rgbFromSrgbHelper(srgb.y),
            rgbFromSrgbHelper(srgb.z));
    }

private:
    UInt8 srgbFromRgbHelper(double t)
    {
        return clamp(0,
            static_cast<int>(256.0*(t <= 0.0031308 ? 12.92*t : 
                (1 + 0.055)*pow(t, 1.0/2.4) - 0.055)),
            255);
    }

    double rgbFromSrgbHelper(double t)
    {
        t /= 256.0;
        return t <= 0.04045 ? t/12.92 : pow((t + 0.055)/(1 + 0.055), 2.4);
    }

    PerceptualModel(PerceptualModelImplementation* implementation)
      : _implementation(implementation) { }
    PerceptualModelImplementation* _implementation;
    static LUVPerceptualModelImplementation _luv;
    static LABPerceptualModelImplementation _lab;
};

LUVPerceptualModelImplementation PerceptualModel::_luv;
LABPerceptualModelImplementation PerceptualModel::_lab;

#endif // INCLUDED_PERCEPTUAL_H
