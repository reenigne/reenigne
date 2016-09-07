#include "alfe/main.h"

#ifndef INCLUDED_COLOUR_SPACE_H
#define INCLUDED_COLOUR_SPACE_H

#include "alfe/vectors.h"

typedef Vector3<float> Colour;
typedef Vector3<UInt8> SRGB;

Colour labFromXyz(Colour xyz)
{
    static const Colour xyzWhite(95.047, 100, 108.883);
    auto helper = [=](float t)
    {
        static const float d = 6.0f/29.0f;
        static const float d3 = d*d*d;
        static const float a = 1.0f/(3.0f*d*d);
        static const float b = 4.0f/29.0f;
        if (t > d3)
            return pow(t, 1/3.0f);
        return a*t + b;
    };
    Colour c = xyz/xyzWhite;
    float y = helper(c.y);
    return Colour(116.0f*y - 16.0f, 500.0f*(helper(c.x) - y),
        200.0f*(y - helper(c.z)));
}

Colour luvFromXyz(Colour xyz)
{
    float x = xyz.x;
    float y = xyz.y;
    float z = xyz.z;
    static const float d = 3.0f/29.0f;
    static const float d2 = d*2.0f;
    float l;
    if (y <= d2*d2*d2)
        l = y/(d*d*d);
    else
        l = 116.0f*pow(y, 1.0f/3.0f) - 16.0f;
    float r = x + 15*y + 3*z;
    float u;
    float v;
    if (r < 1.0e-5) {
        u = 13.0f*l*(4.0f - 0.2105f);
        v = 13.0f*l*(9.0f/15.0f - 0.4737f);
    }
    else {
        u = 13.0f*l*(4.0f*x/r - 0.2105f);
        v = 13.0f*l*(9.0f*y/r - 0.4737f);
    }
    return Colour(l, u, v);
}

Colour xyzFromRgb(Colour rgb)
{
    return Colour(rgb.x*41.24f + rgb.y*35.76f + rgb.z*18.05f,
        rgb.x*21.26f + rgb.y*71.52f + rgb.z*7.22f,
        rgb.x*1.93f + rgb.y*11.92f + rgb.z*95.05f);
}

Colour labFromRgb(Colour rgb) { return labFromXyz(xyzFromRgb(rgb)); }
Colour luvFromRgb(Colour rgb) { return luvFromXyz(xyzFromRgb(rgb)); }

float deltaE2CIEDE2000(Colour rgb1, Colour rgb2)
{
    Colour lab1 = labFromRgb(rgb1);
    Colour lab2 = labFromRgb(rgb2);
    float c1 = sqrt(lab1.y*lab1.y + lab1.z*lab1.z);
    float c2 = sqrt(lab2.y*lab2.y + lab2.z*lab2.z);
    float meanC = (c1 + c2)/2.0f;
    float c3 = meanC*meanC*meanC;
    float c7 = c3*c3*meanC;
    static const float twentyFive7 = 25.0f*25.0f*25.0f*25.0f*25.0f*25.0f*25.0f;
    float d = sqrt(c7/(c7 + twentyFive7));
    float e = 1 + (1 - d)/2.0f;
    float a1p = lab1.y*e;
    float a2p = lab2.y*e;
    float c1p = sqrt(a1p*a1p + lab1.z*lab1.z);
    float c2p = sqrt(a2p*a2p + lab2.z*lab2.z);
    float meanCp = (c1p + c2p)/2.0f;
    static const float tauf = static_cast<float>(tau);
    static const float degrees = 360.0f/tauf;
    static const float pi = tauf/2;
    float meanHp = (atan2(-lab1.z - lab2.z, -a1p - a2p) + pi);
    float deltaHp = 2*sqrt(c1p*c2p)*
        sin(atan2(lab2.z*a1p - a2p*lab1.z, a2p*a1p + lab2.z*lab1.z)/2);
    static const float p1 = -30/degrees;
    static const float p3 = 6/degrees;
    static const float p4 = -63/degrees;
    float t = 1 - 0.17f*cos(meanHp + p1) + 0.24f*cos(2*meanHp)
        + 0.32f*cos(3*meanHp + p3) - 0.20f*cos(4*meanHp + p4);
    float meanL = (lab1.x + lab2.x)/2 - 50;
    float meanL2 = meanL*meanL;
    float sL = 1 + 0.015f*meanL2/sqrt(20 + meanL2);
    float f = (meanHp*degrees - 275)/25;
    static const float g = 60/degrees;
    float deltaL = lab2.x - lab1.x;
    float deltaCp = c2p - c1p;
    float l = deltaL/sL;
    float sC = 1 + 0.045f*meanCp;
    float c = deltaCp/sC;
    float sH = 1 + 0.015f*meanCp*t;
    float h = deltaHp/sH;
    return l*l + c*c + h*h + -2*d*sin(g*exp(-f*f))*deltaCp*deltaHp/(sC*sH);
}

float deltaE2Luv(Colour rgb1, Colour rgb2)
{
    return (luvFromRgb(rgb1) - luvFromRgb(rgb2)).modulus2();
}

float deltaE2CIE76(Colour rgb1, Colour rgb2)
{
    return (labFromRgb(rgb1) - labFromRgb(rgb2)).modulus2();
}

float deltaE2CIE94(Colour rgb1, Colour rgb2)
{
    Colour lab1 = labFromRgb(rgb1);
    Colour lab2 = labFromRgb(rgb2);
    float deltaL = lab1.x - lab2.x;
    float c1 = sqrt(lab1.y*lab1.y + lab1.z*lab1.z);
    float deltaC = c1 - sqrt(lab2.y*lab2.y + lab2.z*lab2.z);
    float deltaa = lab1.y - lab2.y;
    float deltab = lab1.z - lab2.z;
    float c = deltaC/(1 + 0.045f*c1);
    float sH = 1 + 0.015f*c1;
    return deltaL*deltaL + c*c +
        (deltaa*deltaa + deltab*deltab - deltaC*deltaC)/(sH*sH);
}

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
    Colour fromRgb(const Colour& rgb) { return luvFromRgb(rgb); }
    Colour toRgb(const Colour& luv)
    {
        float l = luv.x;
        float u = luv.y;
        float v = luv.z;
        float uu = u/(13.0f*l) + 0.2105f;
        float vv = v/(13.0f*l) + 0.4737f;
        float y;
        static const float d = 3.0f/29.0f;
        if (l <= 8.0f)
            y = l*d*d*d;
        else {
            y = (l + 16.0f)/116.0f;
            y = y*y*y;
        }
        if (vv < 1.0e-5)
            return SRGB(0, 0, 0);
        float x = y*(9.0f*uu)/(4.0f*vv);
        float z = y*(12.0f - 3.0f*uu - 20.0f*vv)/(4.0f*vv);
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
    Colour fromRgb(const Colour& rgb) { return labFromRgb(rgb); }
    Colour toRgb(const Colour& lab)
    {
        float y = (lab.x + 16.0f)/116.0f;
        return ColourSpace::xyz().toRgb(Colour(
            xyzFromLabHelper(y + lab.y/500.0f),
            xyzFromLabHelper(y),
            xyzFromLabHelper(y - lab.z/200.0f)));
    }
private:
    float xyzFromLabHelper(float t)
    {
        static const float d = 6.0f/29.0f;
        return t > d ? pow(t, 3.0f) : (t - 4.0f/29.0f)*3.0f*d*d;
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
        return Colour(
            rgbFromSrgbHelper(srgb.x),
            rgbFromSrgbHelper(srgb.y),
            rgbFromSrgbHelper(srgb.z));
    }
    Colour toSrgb(const Colour& rgb)
    {
        return Colour(
            srgbFromRgbHelper(rgb.x),
            srgbFromRgbHelper(rgb.y),
            srgbFromRgbHelper(rgb.z));
    }
    Colour fromRgb(const Colour& rgb) { return rgb; }
    Colour toRgb(const Colour& rgb) { return rgb; }
private:
    static float srgbFromRgbHelper(float t)
    {
        return 256.0f*(t <= 0.0031308f ? 12.92f*t :
                (1.0f + 0.055f)*pow(t, 1.0f/2.4f) - 0.055f);
    }
    static float rgbFromSrgbHelper(float t)
    {
        t /= 256.0f;
        return t <= 0.04045f ? t/12.92f : pow((t + 0.055f)/(1 + 0.055f), 2.4f);
    }

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
            0.4124f*rgb.x + 0.3576f*rgb.y + 0.1805f*rgb.z,
            0.2126f*rgb.x + 0.7152f*rgb.y + 0.0722f*rgb.z,
            0.0193f*rgb.x + 0.1192f*rgb.y + 0.9505f*rgb.z);
    }
    Colour toRgb(const Colour& xyz)
    {
        return Colour(
             3.2406f*xyz.x - 1.5372f*xyz.y - 0.4986f*xyz.z,
            -0.9689f*xyz.x + 1.8758f*xyz.y + 0.0415f*xyz.z,
             0.0557f*xyz.x - 0.2040f*xyz.y + 1.0570f*xyz.z);
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
