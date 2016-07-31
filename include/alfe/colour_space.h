#include "alfe/main.h"

#ifndef INCLUDED_COLOUR_SPACE_H
#define INCLUDED_COLOUR_SPACE_H

#include "alfe/vectors.h"

typedef Vector3<float> Colour;
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
        float r = x + 15f*y + 3f*z;
        float u;
        float v;
        if (r < 1.0fe-5) {
            u = 13.0f*l*(4.0f - 0.2105f);
            v = 13.0f*l*(9.0f/15.0f - 0.4737f);
        }
        else {
            u = 13.0f*l*(4.0f*x/r - 0.2105f);
            v = 13.0f*l*(9.0f*y/r - 0.4737f);
        }
        return Colour(l, u, v);
    }
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
        if (vv < 1.0fe-5)
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
    Colour fromRgb(const Colour& rgb)
    {
        Colour xyz = ColourSpace::xyz().fromRgb(rgb);
        float y = labFromXyzHelper(xyz.y);
        return Colour(
            116.0f*y - 16.0f,
            500.0f*(labFromXyzHelper(xyz.x) - y),
            200.0f*(y - labFromXyzHelper(xyz.z)));
    }
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

    float labFromXyzHelper(float t)
    {
        static const float d = 6.0f/29.0f;
        return t > d*d*d ? pow(t, 1.0f/3.0f) : t/(3.0f*d*d) + 4.0f/29.0f;
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

    //RGBColourSpaceBody()
    //{
    //    for (int s = 0; s < 256+200; ++s)
    //        _lFromS[s] = rgbFromSrgbHelper(s-100);
    //    for (int l = 0; l < 583+200; ++l)
    //        _sFromL[l] = srgbFromRgbHelper(static_cast<float>(l-100)/582.0);
    //}

    //float _sFromL[583+200];
    //float _lFromS[256+200];

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
             3.2410f*xyz.x - 1.5374f*xyz.y - 0.4986f*xyz.z,
            -0.9692f*xyz.x + 1.8760f*xyz.y + 0.0416f*xyz.z,
             0.0556f*xyz.x - 0.2040f*xyz.y + 1.0570f*xyz.z);
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
