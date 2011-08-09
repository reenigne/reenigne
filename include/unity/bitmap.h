#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

#include "unity/vectors.h"
#include <png.h>

template<class Pixel> class Bitmap
{
public:
    Bitmap() : _size(0, 0) { }
    Bitmap(Vector size)
      : _size(size), _stride(size.x*sizeof(Pixel)), _data(size.y*_stride) { }
    void resample(Bitmap* target)
    {
        // TODO
    }
    void load(const File& file)
    {
        FileHandle handle(file);
        handle.openRead();
        Array<Byte> header(8);
        handle.read(&header[0], 8);
        if (png_sig_cmp(&header[0], 0, 8))
            throw Exception(file.messagePath() +
                String(" is not a .png file"));

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
            static_cast<png_voidp>(&handle), userErrorFunction,
            userWarningFunction);

        png_set_read_fn(read_ptr, static_cast<voidp>(&handle), userReadData);

        // TODO
    }
    void save(const File& file)
    {
        png_set_write_fn(write_ptr, static_cast<voidp>(&handle),
            userWriteData, userFlushData);
        // TODO
    }
    Byte* data() const { return &_data[0]; }
    int stride() const { return _stride; }
    Vector size() const { return _size; }
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
        throw Exception(String(error_msg));  // TODO: can we include the filename?
    }
    static void userWarningFunction(png_structp png_ptr,
        png_const_charp error_msg)
    {
        throw Exception(String(error_msg));  // TODO: can we include the filename?
    }

    Vector _size;
    int _stride;
    Array<Byte> _data;
};

#endif // INCLUDED_BITMAP_H
