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
        PNGRead read(&handle);
        read.init();

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
        FileHandle* handle = png_get_error_ptr(png_ptr);
        throw Exception(String("Error reading: ") + handle->name() +
            colonSpace + String(error_msg));
    }
    static void userWarningFunction(png_structp png_ptr,
        png_const_charp error_msg)
    {
        FileHandle* handle = png_get_error_ptr(png_ptr);
        throw Exception(String("Error reading: ") + handle->name() +
            colonSpace + String(error_msg));
    }

    class PNGRead
    {
    public:
        PNGRead(FileHandle* handle)
        {
            _png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                static_cast<png_voidp>(handle), userErrorFunction,
                userWarningFunction);
            if (png_ptr == 0)
                throw Exception(String("Error creating PNG read structure"));
        }
        void init()
        {
            _info_ptr = png_create_info_struct(_png_ptr);
            if (_info_ptr == 0)
                throw Exception(String("Error creating PNG info structure"));
            png_set_read_fn(_png_ptr, static_cast<voidp>(handle),
                userReadData);
            png_set_sig_bytes(_png_ptr, 8);


        }
        ~PNGRead()
        {
            png_destroy_read_struct(&_png_ptr, &_info_ptr, &_end_info);
        }
    private:
        png_structp _png_ptr;
        png_infop _info_ptr;
    };

    Vector _size;
    int _stride;
    Array<Byte> _data;
};

#endif // INCLUDED_BITMAP_H
