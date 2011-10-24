#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

#include "unity/vectors.h"
#include <png.h>

template<class Pixel> class Bitmap
{
public:
    Bitmap() : _size(0, 0) { }
    Bitmap(Vector size) { setSize(size); }

    // Copy this bitmap to target with resampling.
    void resample(Bitmap* target)
    {
        Vector targetSize = target->size();
        Byte* row = data();
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

        for (int y = 0; y < _size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
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
            row += stride();
            targetRow += intermediateStride;
        }

        resampleVertically(&intermediate[0], target);
    }

    // Copy this bitmap to target with subpixel resampling.
    void subPixelResample(Bitmap* target, bool tripleResolution)
    {
        Vector targetSize = target->size();
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
            row += stride();
            targetRow += intermediateStride;
        }

        resampleVertically(&intermediate[0], target);
    }

    // Convert from one pixel format to another.
    template<class TargetPixel, class Converter> void convert(
        Bitmap<TargetPixel>* target, Converter converter)
    {
        target->setSize(_size);
        Byte* row = data();
        Byte* targetRow = target->data();
        for (int y = 0; y < _size.y; ++y) {
            Pixel* p = reinterpret_cast<Pixel*>(row);
            TargetPixel* tp = reinterpret_cast<TargetPixel*>(targetRow);
            for (int x = 0; x < _size.x; ++x) {
                *tp = converter.convert(*p);
                ++p;
                ++tp;
            }
            row += stride();
            targetRow += target->stride();
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
            throw Exception(file.messagePath() +
                String(" is not a .png file"));
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
    Byte* data() { return &_data[0]; }
    int stride() const { return _stride; }
    Vector size() const { return _size; }
    void setSize(Vector size)
    {
        _size = size;
        _stride = size.x*sizeof(Pixel);
        _data.allocate(size.y*_stride);
    }
private:
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
        throw Exception(String("Error reading: ") + handle->name() +
            colonSpace + String(error_msg));
    }
    static void userWarningFunction(png_structp png_ptr,
        png_const_charp error_msg)
    {
        FileHandle* handle =
            static_cast<FileHandle*>(png_get_error_ptr(png_ptr));
        throw Exception(String("Error reading: ") + handle->name() +
            colonSpace + String(error_msg));
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
                throw Exception(String("Error creating PNG read structure"));
        }
        void read(Bitmap* bitmap)
        {
            _info_ptr = png_create_info_struct(_png_ptr);
            if (_info_ptr == 0)
                throw Exception(String("Error creating PNG info structure"));
            png_set_read_fn(_png_ptr, static_cast<png_voidp>(_handle),
                userReadData);
            png_set_sig_bytes(_png_ptr, 8);
            png_read_png(_png_ptr, _info_ptr, PNG_TRANSFORM_IDENTITY, 0);
            _row_pointers = png_get_rows(_png_ptr, _info_ptr);
            Vector size(png_get_image_width(_png_ptr, _info_ptr),
                png_get_image_height(_png_ptr, _info_ptr));
            bitmap->setSize(size);
            Byte* data = bitmap->data();
            for (int y = 0; y < size.y; ++y) {
                Pixel* line = reinterpret_cast<Pixel*>(data);
                png_bytep row = _row_pointers[y];
                for (int x = 0; x < size.x; ++x) {
                    png_bytep p = &row[x*3];
                    *line = Pixel(p[0], p[1], p[2]);
                    ++line;
                }
                data += bitmap->stride();
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

    void resampleVertically(Byte* intermediate, Bitmap* target)
    {
        Vector targetSize = target->size();
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
        Byte* targetRow = target->data();
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
            Pixel* targetP = reinterpret_cast<Pixel*>(targetRow);
            c = cRow;
            t = tRow;
            for (int x = 0; x < targetSize.x; ++x) {
                *targetP = (*c)/(*t);
                ++c;
                ++t;
                ++targetP;
            }
            targetRow += target->stride();
        }
    }

    static float sinc(float z)
    {
        if (z == 0.0f)
            return 1.0f;
        return sin(z)/z;
    }

    Vector _size;
    int _stride;
    Array<Byte> _data;
};

#endif // INCLUDED_BITMAP_H
