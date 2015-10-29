#include "alfe/main.h"

#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

#include "alfe/colour_space.h"
#include "alfe/vectors.h"
#include "alfe/body_with_array.h"

template<class Pixel> class Bitmap;

template<class Pixel> class BitmapFileFormat : public Handle
{
public:
    Bitmap<Pixel> load(const File& file)
    {
        return body()->load(file);
    }
    void save(Bitmap<Pixel>& bitmap, const File& file) const
    {
        return body()->save(bitmap, file);
    }
protected:
    class Body : public Handle::Body
    {
    public:
        virtual void save(Bitmap<Pixel>& bitmap, const File& file) = 0;
        virtual Bitmap<Pixel> load(const File& file) = 0;
    };
    BitmapFileFormat(Body* body) : Handle(body) { }
private:
    friend class Bitmap<Pixel>;
};

template<class T> class RawFileFormatTemplate;
typedef RawFileFormatTemplate<SRGB> RawFileFormat;

template<class T> class RawFileFormatTemplate : public BitmapFileFormat<T>
{
public:
    RawFileFormatTemplate(Vector size)
      : BitmapFileFormat(new Body(size)) { }
private:
    class Body : public BitmapFileFormat::Body
    {
    public:
        Body(Vector size) : _size(size) { }
        // The bitmap needs to be 8-bit sRGB data for this to work.
        virtual void save(Bitmap<T>& bitmap, const File& file)
        {
            FileStream stream = file.openWrite();
            Byte* data = bitmap.data();
            int stride = bitmap.stride();
            Vector size = bitmap.size();
            for (int y = 0; y < size.y; ++y) {
                stream.write(static_cast<void*>(data), size.x*sizeof(T));
                data += stride;
            }
        }
        // This will put 8-bit sRGB data in the bitmap.
        virtual Bitmap<T> load(const File& file)
        {
            FileStream stream = file.openRead();
            Bitmap<SRGB> bitmap(_size);
            Byte* data = bitmap.data();
            int stride = bitmap.stride();
            for (int y = 0; y < _size.y; ++y) {
                stream.read(static_cast<void*>(data), _size.x*sizeof(T));
                data += stride;
            }
            return bitmap;
        }
    private:
        Vector _size;
    };
};

// A Bitmap is a value class encapsulating a 2D image. Its width, height,
// stride and pixel format are immutable but the pixels themselves are not.
template<class Pixel> class Bitmap : public Handle
{
    class Body : public BodyWithArray<Body, Byte>
    {
    public:
        Body() { }
        Byte* topLeft() { return pointer(); }
    };
public:
    Bitmap() : _size(0, 0) { }
    Bitmap(Vector size)
    {
        _stride = size.x*sizeof(Pixel);
        _size = size;
        Handle::operator=(Body::create(_stride * size.y));
        _topLeft = body()->topLeft();
    }

    // Convert from one pixel format to another.
    template<class TargetPixel, class Converter> void convert(
        Bitmap<TargetPixel>& target, Converter converter)
    {
        Byte* row = data();
        Byte* targetRow = target.data();
        for (int y = 0; y < _size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            TargetPixel* tp = reinterpret_cast<TargetPixel*>(targetRow);
            for (int x = 0; x < _size.x; ++x) {
                *tp = converter.convert(*p);
                ++p;
                ++tp;
            }
            row += _stride;
            targetRow += target.stride();
        }
    }

    void load(const BitmapFileFormat<Pixel>& format, const File& file)
    {
        *this = format.load(file);
    }

    void save(const BitmapFileFormat<Pixel>& format, const File& file)
    {
        format.save(*this, file);
    }
    Byte* data() { return _topLeft; }
    const Byte* data() const { return _topLeft; }
    int stride() const { return _stride; }
    Vector size() const { return _size; }
    Pixel* row(int y)
    {
        return reinterpret_cast<Pixel*>(_topLeft + y*_stride);
    }
    Pixel& operator[](Vector position) { return row(position.y)[position.x]; }

    // A sub-bitmap of a bitmap is a pointer into the same set of data, so
    // drawing on a subBitmap will also draw on the parent bitmap, and any
    // other overlapping sub-bitmaps. This can be used in conjunction with
    // fill() to draw rectangles. To avoid this behavior, use
    // subBitmap().clone().
    Bitmap subBitmap(Vector topLeft, Vector size)
    {
        return Bitmap(body(),
            _topLeft + topLeft.x*sizeof(Pixel) + topLeft.y*_stride, size,
            _stride);
    }

    Bitmap clone() const
    {
        Bitmap c(_size);
        copyTo(c);
        return c;
    }

    void fill(const Pixel& pixel)
    {
        Byte* row = data();
        for (int y = 0; y < _size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            for (int x = 0; x < _size.x; ++x) {
                *p = pixel;
                ++p;
            }
            row += _stride;
        }
    }

    // Copy with pixel format conversion but no resizing. Bitmaps must be the
    // same dimensions.
    template<class OtherPixel> void copyFrom(const Bitmap<OtherPixel>& other)
    {
        Byte* row = data();
        const Byte* otherRow = other.data();
        for (int y = 0; y < _size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            const OtherPixel* op =
                reinterpret_cast<const OtherPixel*>(otherRow);
            for (int x = 0; x < _size.x; ++x) {
                *p = *op;
                ++p;
                ++op;
            }
            row += _stride;
            otherRow += other._stride;
        }
    }
    template<class OtherPixel> void copyTo(Bitmap<OtherPixel>& other) const
    {
        other.copyFrom(*this);
    }

private:
    Bitmap(const Body* body, Byte* topLeft, Vector size, int stride)
      : Handle(body), _topLeft(topLeft), _size(size), _stride(stride)
    { }

    Vector _size;
    Byte* _topLeft;
    int _stride;
};

#endif // INCLUDED_BITMAP_H
