#include "alfe/main.h"

#ifndef INCLUDED_BITMAP_PNG_H
#define INCLUDED_BITMAP_PNG_H

#include <png.h>
#include "alfe/bitmap.h"

class PNGFileFormat : public BitmapFileFormat<SRGB>
{
public:
    PNGFileFormat() : BitmapFileFormat(new Implementation) { }
private:
    class Implementation : public BitmapFileFormat::Implementation
    {
    public:
        // The bitmap needs to be 8-bit sRGB data for this to work.
        virtual void save(Bitmap<SRGB>& bitmap, const File& file)
        {
            FileHandle handle = file.openWrite();
            PNGWrite write(&handle);
            write.write(bitmap);
        }
        // This will put 8-bit sRGB data in the bitmap.
        virtual Bitmap<SRGB> load(const File& file)
        {
            FileHandle handle = file.openRead();
            Array<Byte> header(8);
            handle.read(&header[0], 8);
            if (png_sig_cmp(&header[0], 0, 8))
                throw Exception(file.path() + " is not a .png file");
            return PNGRead(&handle).read();
        }
    private:
        static void userReadData(png_structp png_ptr, png_bytep data,
            png_size_t length)
        {
            FileHandle* handle =
                static_cast<FileHandle*>(png_get_io_ptr(png_ptr));
            handle->read(static_cast<Byte*>(data), length);
        }
        static void userWriteData(png_structp png_ptr, png_bytep data,
            png_size_t length)
        {
            FileHandle* handle =
                static_cast<FileHandle*>(png_get_io_ptr(png_ptr));
            handle->write(static_cast<void*>(data), length);
        }
        static void userFlushData(png_structp png_ptr) { }
        static void userErrorFunction(png_structp png_ptr,
            png_const_charp error_msg)
        {
            FileHandle* handle =
                static_cast<FileHandle*>(png_get_error_ptr(png_ptr));
            throw Exception("Error reading: " + handle->file().path() + ": " +
                error_msg);
        }
        static void userWarningFunction(png_structp png_ptr,
            png_const_charp error_msg)
        {
            FileHandle* handle =
                static_cast<FileHandle*>(png_get_error_ptr(png_ptr));
            throw Exception("Error reading: " + handle->file().path() + ": " +
                error_msg);
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
            Bitmap<SRGB> read()
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
                Bitmap<SRGB> bitmap(size);
                Byte* data = bitmap.data();
                int stride = bitmap.stride();
                for (int y = 0; y < size.y; ++y) {
                    SRGB* line = reinterpret_cast<SRGB*>(data);
                    png_bytep row = _row_pointers[y];
                    for (int x = 0; x < size.x; ++x) {
                        png_bytep p = &row[x*3];
                        *line = SRGB(p[0], p[1], p[2]);
                        ++line;
                    }
                    data += stride;
                }
                return bitmap;
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
                    throw Exception("Error creating PNG write structure");
            }
            void write(Bitmap<SRGB>& bitmap)
            {
                _info_ptr = png_create_info_struct(_png_ptr);
                if (_info_ptr == 0)
                    throw Exception("Error creating PNG info structure");
                png_set_write_fn(_png_ptr, static_cast<png_voidp>(_handle),
                    userWriteData, userFlushData);
                Vector size = bitmap.size();
                png_set_IHDR(_png_ptr, _info_ptr, size.x, size.y, 8,
                    PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                    PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
                Array<Byte*> rows(size.y);
                Byte* data = bitmap.data();
                int stride = bitmap.stride();
                for (int y = 0; y < size.y; ++y) {
                    rows[y] = data;
                    data += stride;
                }
                png_set_rows(_png_ptr, _info_ptr,
                    static_cast<png_bytepp>(&rows[0]));
                png_write_png(_png_ptr, _info_ptr, PNG_TRANSFORM_IDENTITY,
                    NULL);
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
    };
};

#endif // INCLUDED_BITMAP_PNG_H
