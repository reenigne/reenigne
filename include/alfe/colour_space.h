#include "alfe/main.h"

#ifndef INCLUDED_COLOUR_SPACE_H
#define INCLUDED_COLOUR_SPACE_H

#include "alfe/vectors.h"

typedef Vector3<double> Colour;
typedef Vector3<UInt8> SRGB;

class ColourSpaceBody
{
public:
    virtual Colour fromSrgb(const Colour& srgb) = 0;
    virtual Colour toSrgb(const Colour& colour) = 0;
    virtual Colour fromRgb(const Colour& rgb) = 0;
    virtual Colour toRgb(const Colour& colour) = 0;
};

template<class T> class ColourSpaceT;
typedef ColourSpaceT<void> ColourSpace;

// CIELUV L*u*v* conversions from
// http://en.wikipedia.org/wiki/CIELUV_color_space
template<class T> class LUVColourSpaceBodyT : public ColourSpaceBody
{
public:
    Colour fromSrgb(const Colour& srgb)
    {
        return fromRgb(ColourSpace::rgb().fromSrgb(srgb));
    }
    Colour toSrgb(const Colour& luv)
    {
        return ColourSpace::rgb().toSrgb(toRgb(luv));
    }
    Colour fromRgb(const Colour& rgb)
    {
        Colour xyz = ColourSpace::xyz().fromRgb(rgb);
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
    Colour toRgb(const Colour& luv)
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
            return SRGB(0, 0, 0);
        double x = y*(9*uu)/(4*vv);
        double z = y*(12 - 3*uu - 20*vv)/(4*vv);
        return ColourSpace::xyz().toRgb(Colour(x, y, z));
    }
private:
    LUVColourSpaceBodyT() { }
    friend class ColourSpaceT<T>;
};

typedef LUVColourSpaceBodyT<void>  LUVColourSpaceBody;

// CIELAB L*a*b* conversions from http://en.wikipedia.org/wiki/Lab_color_space
template<class T> class LABColourSpaceBodyT : public ColourSpaceBody
{
public:
    Colour fromSrgb(const Colour& srgb)
    {
        return fromRgb(ColourSpace::rgb().fromSrgb(srgb));
    }
    Colour toSrgb(const Colour& lab)
    {
        return ColourSpace::rgb().toSrgb(toRgb(lab));
    }
    Colour fromRgb(const Colour& rgb)
    {
        Colour xyz = ColourSpace::xyz().fromRgb(rgb);
        double y = labFromXyzHelper(xyz.y);
        return Colour(
            116.0*y - 16.0,
            500.0*(labFromXyzHelper(xyz.x) - y),
            200.0*(y - labFromXyzHelper(xyz.z)));
    }
    Colour toRgb(const Colour& lab)
    {
        double y = (lab.x + 16.0)/116.0;
        return ColourSpace::xyz().toRgb(Colour(
            xyzFromLabHelper(y + lab.y/500.0),
            xyzFromLabHelper(y),
            xyzFromLabHelper(y - lab.z/200.0)));
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
    LABColourSpaceBodyT() { }
    friend class ColourSpaceT<T>;
};

typedef LABColourSpaceBodyT<void>  LABColourSpaceBody;

template<class T> class SRGBColourSpaceBodyT : public ColourSpaceBody
{
public:
    Colour fromSrgb(const Colour& srgb) { return srgb; }
    Colour toSrgb(const Colour& srgb) { return srgb; }
    Colour fromRgb(const Colour& rgb)
    {
        return ColourSpace::rgb().toSrgb(rgb);
    }
    Colour toRgb(const Colour& srgb)
    {
        return ColourSpace::rgb().fromSrgb(srgb);
    }
private:
    SRGBColourSpaceBodyT() { }
    friend class ColourSpaceT<T>;
};

typedef SRGBColourSpaceBodyT<void> SRGBColourSpaceBody;

// sRGB conversions from http://en.wikipedia.org/wiki/SRGB
class RGBColourSpaceBody : public ColourSpaceBody
{
public:
    Colour fromSrgb(const Colour& srgb)
    {
        //return Colour(_lFromS[clamp(0, static_cast<int>(srgb.x+100), 255+200)],
        //    _lFromS[clamp(0, static_cast<int>(srgb.y+100), 255+200)],
        //    _lFromS[clamp(0, static_cast<int>(srgb.z+100), 255+200)]);
        return Colour(
            rgbFromSrgbHelper(srgb.x),
            rgbFromSrgbHelper(srgb.y),
            rgbFromSrgbHelper(srgb.z));
    }
    Colour toSrgb(const Colour& rgb)
    {
        //return Colour(_sFromL[clamp(0, static_cast<int>(rgb.x*582.0+100), 582+200)],
        //    _sFromL[clamp(0, static_cast<int>(rgb.y*582.0+100), 582+200)],
        //    _sFromL[clamp(0, static_cast<int>(rgb.z*582.0+100), 582+200)]);
        return Colour(
            srgbFromRgbHelper(rgb.x),
            srgbFromRgbHelper(rgb.y),
            srgbFromRgbHelper(rgb.z));
    }
    Colour fromRgb(const Colour& rgb) { return rgb; }
    Colour toRgb(const Colour& rgb) { return rgb; }
private:
    static double srgbFromRgbHelper(double t)
    {
        return 256.0*(t <= 0.0031308 ? 12.92*t :
                (1 + 0.055)*pow(t, 1.0/2.4) - 0.055);
    }
    static double rgbFromSrgbHelper(double t)
    {
        t /= 256.0;
        return t <= 0.04045 ? t/12.92 : pow((t + 0.055)/(1 + 0.055), 2.4);
    }

    //RGBColourSpaceBody()
    //{
    //    for (int s = 0; s < 256+200; ++s)
    //        _lFromS[s] = rgbFromSrgbHelper(s-100);
    //    for (int l = 0; l < 583+200; ++l)
    //        _sFromL[l] = srgbFromRgbHelper(static_cast<double>(l-100)/582.0);
    //}

    //double _sFromL[583+200];
    //double _lFromS[256+200];

    friend class ColourSpaceT<void>;
};

template<class T> class XYZColourSpaceBodyT : public ColourSpaceBody
{
public:
    Colour fromSrgb(const Colour& srgb)
    {
        return fromRgb(ColourSpace::rgb().fromSrgb(srgb));
    }
    Colour toSrgb(const Colour& xyz)
    {
        return ColourSpace::rgb().toSrgb(toRgb(xyz));
    }
    Colour fromRgb(const Colour& rgb)
    {
        return Colour(
            0.4124*rgb.x + 0.3576*rgb.y + 0.1805*rgb.z,
            0.2126*rgb.x + 0.7152*rgb.y + 0.0722*rgb.z,
            0.0193*rgb.x + 0.1192*rgb.y + 0.9505*rgb.z);
    }
    Colour toRgb(const Colour& xyz)
    {
        return Colour(
             3.2410*xyz.x - 1.5374*xyz.y - 0.4986*xyz.z,
            -0.9692*xyz.x + 1.8760*xyz.y + 0.0416*xyz.z,
             0.0556*xyz.x - 0.2040*xyz.y + 1.0570*xyz.z);
    }
private:
    XYZColourSpaceBodyT() { }
    friend class ColourSpaceT<T>;
};

typedef XYZColourSpaceBodyT<void>  XYZColourSpaceBody;

template<class T> class ColourSpaceT : public ConstHandle
{
public:
    ColourSpaceT() { }
    static ColourSpace luv() { return ColourSpace(&_luv); }
    static ColourSpace lab() { return ColourSpace(&_lab); }
    static ColourSpace srgb() { return ColourSpace(&_srgb); }
    static ColourSpace rgb() { return ColourSpace(&_rgb); }
    static ColourSpace xyz() { return ColourSpace(&_xyz); }
    Colour fromSrgb(const Colour& srgb)
    {
        return _body->fromSrgb(srgb);
    }
    Colour toSrgb(const Colour& colour)
    {
        return _body->toSrgb(colour);
    }
    Colour fromRgb(const Colour& rgb)
    {
        return _body->fromRgb(rgb);
    }
    Colour toRgb(const Colour& colour)
    {
        return _body->toRgb(colour);
    }
    SRGB toSrgb24(const Colour& colour)
    {
        Colour c = _body->toSrgb(colour);
        return SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
    }
private:
    ColourSpaceT(ColourSpaceBody* body) : _body(body) { }
    ColourSpaceBody* _body;
    static LUVColourSpaceBody _luv;
    static LABColourSpaceBody _lab;
    static SRGBColourSpaceBody _srgb;
    static RGBColourSpaceBody _rgb;
    static XYZColourSpaceBody _xyz;
};

LUVColourSpaceBody ColourSpace::_luv;
LABColourSpaceBody ColourSpace::_lab;
SRGBColourSpaceBody ColourSpace::_srgb;
RGBColourSpaceBody ColourSpace::_rgb;
XYZColourSpaceBody ColourSpace::_xyz;

#endif // INCLUDED_COLOUR_SPACE_H
