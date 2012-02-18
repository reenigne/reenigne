#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

#include "unity/vectors.h"
#include "unity/reference_counted_array.h"
#include "unity/file.h"
#include <png.h>

// A Bitmap is a value class encapsulating a 2D image. Its width, height,
// stride and pixel format are immutable but the pixels themselves are not.
template<class Pixel> class Bitmap
{
    class Data : public ReferenceCountedArray<Data, Byte>
    {
    public:
        Byte& operator[] (int i)
        {
            return ReferenceCountedArray<Data, Byte>::operator[](i);
        }
        Byte operator[] (int i) const
        {
            return ReferenceCountedArray<Data, Byte>::operator[](i);
        }

        int _stride;
    };
public:
    Bitmap() : _size(0, 0) { }
    Bitmap(Vector size) { create(size, size.x*sizeof(Pixel)); }

    bool valid() const { return _data.valid(); }

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
            scaleTarget = 1.0f;
            scale =
                static_cast<float>(targetSize.x)/static_cast<float>(_size.x);
        }
        else {
            // Downsampling. The band-limit resolution is the same as the
            // target resolution.
            scaleTarget =
                static_cast<float>(_size.x)/static_cast<float>(targetSize.x);
            scale = 1.0f;
        }

        int s = stride();
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
            row += s;
            targetRow += intermediateStride;
        }

        resampleVertically(&intermediate[0], target);
    }

    // Copy this bitmap to target with subpixel resampling.
    template<class OtherPixel> void subPixelResample(
        Bitmap<OtherPixel>& target, bool tripleResolution) const
    {
        Vector targetSize = target.size();
        Byte* row = data();
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
            scaleTarget = 1.0f;
            scale =
                static_cast<float>(targetSize.x)/static_cast<float>(_size.x);
        }
        else {
            // Downsampling. The band-limit resolution is the same as the
            // target resolution.
            scaleTarget =
                static_cast<float>(_size.x)/static_cast<float>(targetSize.x);
            scale = 1.0f;
        }
        float subPixel = scaleTarget/3.0f;
        if (tripleResolution) {
            scaleTarget *= 3.0f;
            scale *= 3.0f;
        }
        int s = stride();
        for (int y = 0; y < _size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
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
            row += s;
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
        int s = stride();
        int targetStride = target.stride();
        for (int y = 0; y < _size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            TargetPixel* tp = reinterpret_cast<TargetPixel*>(targetRow);
            for (int x = 0; x < _size.x; ++x) {
                *tp = converter.convert(*p);
                ++p;
                ++tp;
            }
            row += s;
            targetRow += targetStride;
        }
    }

    // This will put 8-bit sRGB data in the bitmap.
    void load(const File& file)
    {
        FileHandle handle(file);
        handle.openRead();
        Array<Byte> header(8);
        handle.read(&header[0], 8);
        if (png_sig_cmp(&header[0], 0, 8))
            throw Exception(file.messagePath() + " is not a .png file");
        PNGRead read(&handle);
        read.read(this);
    }

    // The bitmap needs to be 8-bit sRGB data for this to work.
    void save(const File& file)
    {
        FileHandle handle(file);
        handle.openWrite();
        PNGWrite write(&handle);
        write.write(this);
    }
    Byte* data() { return _topLeft; }
    const Byte* data() const { return _topLeft; }
    int stride() const { return _data->_stride; }
    Vector size() const { return _size; }
    Pixel* row(int y)
    {
        return reinterpret_cast<Pixel*>(_topLeft + y*stride());
    }
    Pixel& pixel(Vector position) { return row(position.y)[position.x]; }

    // A sub-bitmap of a bitmap is a pointer into the same set of data, so
    // drawing on a subBitmap will also draw on the parent bitmap, and any
    // other overlapping sub-bitmaps. This can be used in conjunction with
    // fill() to draw rectangles. To avoid this behavior, use
    // subBitmap().clone().
    Bitmap subBitmap(Vector topLeft, Vector size)
    {
        return Bitmap(_data,
            _topLeft + topLeft.x*sizeof(Pixel) + topLeft.y*stride(), size);
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
        int s = stride();
        for (int y = 0; y < size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            for (int x = 0; x < size.x; ++x) {
                *p = pixel;
                ++p;
            }
            row += s;
        }
    }

    // Copy with pixel format conversion but no resizing. Bitmaps must be the
    // same dimensions.
    template<class OtherPixel> void copyFrom(const Bitmap<OtherPixel>& other)
    {
        Byte* row = data();
        int s = stride();
        const Byte* otherRow = other.data();
        int os = other.stride();
        for (int y = 0; y < size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            OtherPixel* op = reinterpret_cast<OtherPixel*>(otherRow);
            for (int x = 0; x < size.x; ++x) {
                *p = *op;
                ++p;
                ++op;
            }
            row += s;
            otherRow += os;
        }
    }
    template<class OtherPixel> void copyTo(Bitmap<OtherPixel>& other) const
    {
        other.copyFrom(*this);
    }

private:
    Bitmap(const Reference<Data>& data, Byte* topLeft, Vector size)
      : _data(data), _topLeft(topLeft), _size(size) { }

    void create(Vector size, int stride)
    {
        _data = ReferenceCountedArray<Data, Byte>::create(stride * size.y);
        _data->_stride = stride;
        _topLeft = &(*_data)[0];
    }

    static void userReadData(png_structp png_ptr, png_bytep data,
        png_size_t length)
    {
        FileHandle* handle = static_cast<FileHandle*>(png_get_io_ptr(png_ptr));
        handle->read(static_cast<void*>(data), length);
    }
    static void userWriteData(png_structp png_ptr, png_bytep data,
        png_size_t length)
    {
        FileHandle* handle = static_cast<FileHandle*>(png_get_io_ptr(png_ptr));
        handle->write(static_cast<void*>(data), length);
    }
    static void userFlushData(png_structp png_ptr) { }
    static void userErrorFunction(png_structp png_ptr,
        png_const_charp error_msg)
    {
        FileHandle* handle =
            static_cast<FileHandle*>(png_get_error_ptr(png_ptr));
        throw Exception("Error reading: " + handle->name() + ": " + error_msg);
    }
    static void userWarningFunction(png_structp png_ptr,
        png_const_charp error_msg)
    {
        FileHandle* handle =
            static_cast<FileHandle*>(png_get_error_ptr(png_ptr));
        throw Exception("Error reading: " + handle->name() + ": " + error_msg);
    }

    class PNGRead
    {
    public:
        PNGRead(FileHandle* handle) : _handle(handle)
        {
            _png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                static_cast<png_voidp>(handle), userErrorFunction,
                userWarningFunction);
            if (_png_ptr == 0)
                throw Exception("Error creating PNG read structure");
        }
        void read(Bitmap* bitmap)
        {
            _info_ptr = png_create_info_struct(_png_ptr);
            if (_info_ptr == 0)
                throw Exception("Error creating PNG info structure");
            png_set_read_fn(_png_ptr, static_cast<png_voidp>(_handle),
                userReadData);
            png_set_sig_bytes(_png_ptr, 8);
            png_read_png(_png_ptr, _info_ptr, PNG_TRANSFORM_IDENTITY, 0);
            _row_pointers = png_get_rows(_png_ptr, _info_ptr);
            Vector size(png_get_image_width(_png_ptr, _info_ptr),
                png_get_image_height(_png_ptr, _info_ptr));
            *bitmap = Bitmap(size);
            Byte* data = bitmap->data();
            int stride = bitmap->stride();
            for (int y = 0; y < size.y; ++y) {
                Pixel* line = reinterpret_cast<Pixel*>(data);
                png_bytep row = _row_pointers[y];
                for (int x = 0; x < size.x; ++x) {
                    png_bytep p = &row[x*3];
                    *line = Pixel(p[0], p[1], p[2]);
                    ++line;
                }
                data += stride;
            }
        }
        ~PNGRead()
        {
            png_destroy_read_struct(&_png_ptr, &_info_ptr, 0);
        }
    private:
        png_structp _png_ptr;
        png_infop _info_ptr;
        png_bytep* _row_pointers;
        FileHandle* _handle;
    };

    class PNGWrite
    {
    public:
        PNGWrite(FileHandle* handle) : _handle(handle)
        {
            _png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                static_cast<png_voidp>(handle), userErrorFunction,
                userWarningFunction);
            if (_png_ptr == 0)
                throw Exception(String("Error creating PNG write structure"));
        }
        void write(Bitmap* bitmap)
        {
            _info_ptr = png_create_info_struct(_png_ptr);
            if (_info_ptr == 0)
                throw Exception(String("Error creating PNG info structure"));
            png_set_write_fn(_png_ptr, static_cast<png_voidp>(_handle),
                userWriteData, userFlushData);
            Vector size = bitmap->size();
            png_set_IHDR(_png_ptr, _info_ptr, size.x, size.y, 8,
                PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            Array<Byte*> rows(size.y);
            Byte* data = bitmap->data();
            for (int y = 0; y < size.y; ++y) {
                rows[y] = data;
                data += bitmap->stride();
            }
            png_set_rows(_png_ptr, _info_ptr,
                static_cast<png_bytepp>(&rows[0]));
            png_write_png(_png_ptr, _info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
        }
        ~PNGWrite()
        {
            png_destroy_write_struct(&_png_ptr, &_info_ptr);
        }
    private:
        png_structp _png_ptr;
        png_infop _info_ptr;
        png_bytep* _row_pointers;
        FileHandle* _handle;
    };

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
            scaleTarget = 1.0f;
            scale =
                static_cast<float>(targetSize.y)/static_cast<float>(_size.y);
        }
        else {
            // Downsampling
            scaleTarget =
                static_cast<float>(_size.y)/static_cast<float>(targetSize.y);
            scale = 1.0f;
        }
        Byte* targetRow = target.data();
        int targetStride = target.stride();
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
            targetRow += targetStride;
        }
    }

    static float sinc(float z)
    {
        if (z == 0.0f)
            return 1.0f;
        return sin(z)/z;
    }

    Vector _size;
    Reference<Data> _data;
    Byte* _topLeft;
};

#endif // INCLUDED_BITMAP_H
