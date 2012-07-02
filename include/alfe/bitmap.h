#include "alfe/main.h"

#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

#include "alfe/colour_space.h"
#include "alfe/vectors.h"
#include "alfe/reference_counted_array.h"

template<class Pixel> class Bitmap;

template<class Pixel> class BitmapFileFormat
{
public:
    Bitmap<Pixel> load(const File& file)
    {
        return _implementation->load(file);
    }
    void save(Bitmap<Pixel>& bitmap, const File& file)
    {
        return _implementation->save(file);
    }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual void save(Bitmap<Pixel>& bitmap, const File& file) = 0;
        virtual Bitmap<Pixel> load(const File& file) = 0;
    };
    BitmapFileFormat(Implementation* implementation)
      : _implementation(implementation) { }
private:
    Reference<Implementation> _implementation;
    friend class Bitmap<Pixel>;
};

template<class T> class RawFileFormatTemplate;
typedef RawFileFormatTemplate<SRGB> RawFileFormat;

template<class T> class RawFileFormatTemplate : public BitmapFileFormat<T>
{
public:
    RawFileFormatTemplate(Vector size)
      : BitmapFileFormat(new Implementation(size)) { }
private:
    class Implementation : public BitmapFileFormat::Implementation
    {
    public:
        Implementation(Vector size) : _size(size) { }
        // The bitmap needs to be 8-bit sRGB data for this to work.
        virtual void save(Bitmap<T>& bitmap, const File& file)
        {
            FileHandle handle = file.openWrite();
            Byte* data = bitmap.data();
            int stride = bitmap.stride();
            Vector size = bitmap.size();
            for (int y = 0; y < size.y; ++y) {
                handle.write(static_cast<void*>(data), size.x*sizeof(T));
                data += stride;
            }
        }
        // This will put 8-bit sRGB data in the bitmap.
        virtual Bitmap<T> load(const File& file)
        {
            FileHandle handle = file.openRead();
            Bitmap<SRGB> bitmap(_size);
            Byte* data = bitmap.data();
            int stride = bitmap.stride();
            for (int y = 0; y < _size.y; ++y) {
                handle.read(static_cast<void*>(data), _size.x*sizeof(T));
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
template<class Pixel> class Bitmap
{
    class Implementation : public ReferenceCountedArray<Implementation, Byte>
    {
    public:
        Implementation() { }
        Implementation(int bytes) : _implementation(create(bytes)) { }
    };
public:
    Bitmap() : _size(0, 0) { }
    Bitmap(Vector size)
    {
        _stride = size.x*sizeof(Pixel);
        _size = size;
        _implementation = Implementation(_stride * size.y);
        _topLeft = _data.pointer();
    }

    bool valid() const { return _implementation.valid(); }

    // Copy this bitmap to target with resampling.
    template<class OtherPixel> void resample(Bitmap<OtherPixel>& target) const
    {
        Vector targetSize = target.size();
        const Byte* row = data();
        float scaleTarget;
        float scale;

        // Resample horizontally
        int intermediateStride = targetSize.x * sizeof(Pixel);
        Array<Byte> intermediate(intermediateStride * _size.y);
        Byte* targetRow = &intermediate[0];
        if (targetSize.x > _size.x) {
            // Upsampling. The band-limit resolution is the same as the source
            // resolution.
            scaleTarget =
                static_cast<float>(_size.x)/static_cast<float>(targetSize.x);
            scale = 1.0f;
        }
        else {
            // Downsampling. The band-limit resolution is the same as the
            // target resolution.
            scaleTarget = 1.0f;
            scale =
                static_cast<float>(targetSize.x)/static_cast<float>(_size.x);
        }

        for (int y = 0; y < _size.y; ++y) {
            const Pixel* p = reinterpret_cast<const Pixel*>(row);
            Pixel* targetP = reinterpret_cast<Pixel*>(targetRow);
            for (int xTarget = 0; xTarget < targetSize.x; ++xTarget) {
                Pixel c(0, 0, 0);
                float t = 0;
                float z0 = xTarget*scaleTarget;
                for (int x = 0; x < _size.x; ++x) {
                    float s = sinc(x*scale - z0);
                    t += s;
                    c += p[x]*s;
                }
                *targetP = c/t;
                ++targetP;
            }
            row += _stride;
            targetRow += intermediateStride;
        }

        resampleVertically(&intermediate[0], target);
    }

    // Copy this bitmap to target with subpixel resampling.
    template<class OtherPixel> void subPixelResample(
        Bitmap<OtherPixel>& target, bool tripleResolution) const
    {
        Vector targetSize = target.size();
        const Byte* row = data();
        float scaleTarget;
        float scale;

        // Resample horizontally
        int intermediateStride = targetSize.x * sizeof(Pixel);
        Array<Byte> intermediate(intermediateStride * _size.y);
        Byte* targetRow = &intermediate[0];
        int targetWidth = targetSize.x;
        if (tripleResolution)
            targetWidth *= 3;
        if (targetWidth > _size.x) {
            // Upsampling. The band-limit resolution is the same as the source
            // resolution.
            scaleTarget =
                static_cast<float>(_size.x)/static_cast<float>(targetSize.x);
            scale = 1.0f;
        }
        else {
            // Downsampling. The band-limit resolution is the same as the
            // target resolution.
            scaleTarget = 1.0f;
            scale =
                static_cast<float>(targetSize.x)/static_cast<float>(_size.x);
        }
        float subPixel = scaleTarget/3.0f;
        if (tripleResolution) {
            scaleTarget *= 3.0f;
            scale *= 3.0f;
        }
        for (int y = 0; y < _size.y; ++y) {
            const Pixel* p = reinterpret_cast<const Pixel*>(row);
            Pixel* targetP = reinterpret_cast<Pixel*>(targetRow);
            for (int xTarget = 0; xTarget < targetSize.x; ++xTarget) {
                Pixel c(0, 0, 0);
                Pixel t(0, 0, 0);
                float z0 = xTarget*scaleTarget;
                for (int x = 0; x < _size.x; ++x) {
                    float xs = x*scale;
                    float z = xs - z0;
                    float s = sinc(z);
                    t.x += s;
                    c.x += p[x].x*s;

                    z -= subPixel;
                    s = sinc(z);
                    t.y += s;
                    c.y += p[x].y*s;

                    z -= subPixel;
                    s = sinc(z);
                    t.z += s;
                    c.z += p[x].z*s;
                }
                *targetP = c/t;
                ++targetP;
            }
            row += _stride;
            targetRow += intermediateStride;
        }

        resampleVertically(&intermediate[0], target);
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
        return Bitmap(_implementation,
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
        for (int y = 0; y < size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            OtherPixel* op = reinterpret_cast<OtherPixel*>(otherRow);
            for (int x = 0; x < size.x; ++x) {
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
    Bitmap(const Reference<Implementation>& data, Byte* topLeft, Vector size,
        int stride)
      : _implementation(implementation), _topLeft(topLeft), _size(size),
        _stride(stride)
    { }

    template<class OtherPixel> void resampleVertically(Byte* intermediate,
        Bitmap<OtherPixel>& target) const
    {
        Vector targetSize = target.size();
        int intermediateStride = targetSize.x * sizeof(Pixel);
        Array<Byte> cRowArray(intermediateStride);
        Array<float> tRowArray(targetSize.x);
        Pixel* cRow = reinterpret_cast<Pixel*>(&cRowArray[0]);
        float* tRow = &tRowArray[0];
        float scaleTarget;
        float scale;
        if (targetSize.y > _size.y) {
            // Upsampling
            scaleTarget =
                static_cast<float>(_size.y)/static_cast<float>(targetSize.y);
            scale = 1.0f;
        }
        else {
            // Downsampling
            scaleTarget = 1.0f;
            scale =
                static_cast<float>(targetSize.y)/static_cast<float>(_size.y);
        }
        Byte* targetRow = target.data();
        for (int yTarget = 0; yTarget < targetSize.y; ++yTarget) {
            Pixel* c = cRow;
            float* t = tRow;
            for (int x = 0; x < targetSize.x; ++x) {
                *c = Pixel(0, 0, 0);
                ++c;
                *t = 0;
                ++t;
            }
            Byte* row = intermediate;
            float z0 = yTarget*scaleTarget;
            for (int y = 0; y < _size.y; ++y) {
                c = cRow;
                t = tRow;
                float s = sinc(y*scale - z0);
                Pixel* p = reinterpret_cast<Pixel*>(row);
                for (int x = 0; x < targetSize.x; ++x) {
                    *t += s;
                    *c += s*(*p);
                    ++t;
                    ++c;
                    ++p;
                }
                row += intermediateStride;
            }
            OtherPixel* targetP = reinterpret_cast<OtherPixel*>(targetRow);
            c = cRow;
            t = tRow;
            for (int x = 0; x < targetSize.x; ++x) {
                *targetP = (*c)/(*t);
                ++c;
                ++t;
                ++targetP;
            }
            targetRow += target.stride();
        }
    }

    static float sinc(float z)
    {
        if (z == 0.0f)
            return 1.0f;
        z *= M_PI;
        return sin(z)/z;
    }

    Vector _size;
    Reference<Implementation> _implementation;
    Byte* _topLeft;
    int _stride;
};

#endif // INCLUDED_BITMAP_H
