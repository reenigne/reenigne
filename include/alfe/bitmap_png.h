#include "alfe/main.h"

#ifndef INCLUDED_BITMAP_PNG_H
#define INCLUDED_BITMAP_PNG_H

#include <png.h>
#include "alfe/bitmap.h"

// Currently T needs to be DWORD (0x00RRGGBB) or SRGB (0xRR, 0xGG, 0xBB).
template<class T> class PNGFileFormat : public BitmapFileFormat<T>
{
public:
    PNGFileFormat() : BitmapFileFormat(new Body) { }
private:
    class Body : public BitmapFileFormat::Body
    {
    public:
        virtual void save(Bitmap<T>& bitmap, const File& file)
        {
            FileStream stream = file.openWrite();
            PNGWrite write(&stream);
            write.write(bitmap);
        }
        virtual Bitmap<T> load(const File& file)
        {
            FileStream stream = file.openRead();
            Array<Byte> header(8);
            stream.read(&header[0], 8);
            if (png_sig_cmp(&header[0], 0, 8))
                throw Exception(file.path() + " is not a .png file");
            return PNGRead(&stream).read();
        }
    private:
        static void userReadData(png_structp png_ptr, png_bytep data,
            png_size_t length)
        {
            FileStream* stream =
                static_cast<FileStream*>(png_get_io_ptr(png_ptr));
            stream->read(static_cast<Byte*>(data), length);
        }
        static void userWriteData(png_structp png_ptr, png_bytep data,
            png_size_t length)
        {
            FileStream* stream =
                static_cast<FileStream*>(png_get_io_ptr(png_ptr));
            stream->write(static_cast<void*>(data), length);
        }
        static void userFlushData(png_structp png_ptr) { }
        static void userErrorFunction(png_structp png_ptr,
            png_const_charp error_msg)
        {
            FileStream* stream =
                static_cast<FileStream*>(png_get_error_ptr(png_ptr));
            throw Exception("Error reading: " + stream->file().path() + ": " +
                error_msg);
        }
        static void userWarningFunction(png_structp png_ptr,
            png_const_charp error_msg)
        {
            FileStream* stream =
                static_cast<FileStream*>(png_get_error_ptr(png_ptr));
            throw Exception("Error reading: " + stream->file().path() + ": " +
                error_msg);
        }

        class PNGRead
        {
        public:
            PNGRead(FileStream* stream) : _stream(stream)
            {
                _png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                    static_cast<png_voidp>(stream), userErrorFunction,
                    userWarningFunction);
                if (_png_ptr == 0)
                    throw Exception("Error creating PNG read structure");
            }
            Bitmap<T> read()
            {
                _info_ptr = png_create_info_struct(_png_ptr);
                if (_info_ptr == 0)
                    throw Exception("Error creating PNG info structure");
                png_set_read_fn(_png_ptr, static_cast<png_voidp>(_stream),
                    userReadData);
                png_set_sig_bytes(_png_ptr, 8);
                png_read_png(_png_ptr, _info_ptr,
                    PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_16, 0);
                _row_pointers = png_get_rows(_png_ptr, _info_ptr);
                Vector size(png_get_image_width(_png_ptr, _info_ptr),
                    png_get_image_height(_png_ptr, _info_ptr));
                Bitmap<T> bitmap(size);
                doCopy<T>(bitmap);
                return bitmap;
            }
            ~PNGRead()
            {
                png_destroy_read_struct(&_png_ptr, &_info_ptr, 0);
            }
        private:
            template<class T2> void doCopy(Bitmap<T2> bitmap)
            {
                throw Exception();
            }
            template<> void doCopy<SRGB>(Bitmap<SRGB> bitmap)
            {
                Byte* data = bitmap.data();
                int stride = bitmap.stride();
                Vector size = bitmap.size();
                for (int y = 0; y < size.y; ++y) {
                    png_bytep row = _row_pointers[y];
                    memcpy(reinterpret_cast<SRGB*>(data), row, size.x*3);
                    data += stride;
                }
            }
            template<> void doCopy<DWORD>(Bitmap<DWORD> bitmap)
            {
                Byte* data = bitmap.data();
                int stride = bitmap.stride();
                Vector size = bitmap.size();
                for (int y = 0; y < size.y; ++y) {
                    DWORD* line = reinterpret_cast<DWORD*>(data);
                    png_bytep row = _row_pointers[y];
                    png_byte* input = row;
                    for (int x = 0; x < size.x; ++x) {
                        png_byte r = *input;
                        ++input;
                        png_byte g = *input;
                        ++input;
                        png_byte b = *input;
                        ++input;
                        *line = (r << 16) | (g << 8) | b;
                        ++line;
                    }
                    data += stride;
                }
            }

            png_structp _png_ptr;
            png_infop _info_ptr;
            png_bytep* _row_pointers;
            FileStream* _stream;
        };

        class PNGWrite
        {
        public:
            PNGWrite(FileStream* stream) : _stream(stream)
            {
                _png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                    static_cast<png_voidp>(stream), userErrorFunction,
                    userWarningFunction);
                if (_png_ptr == 0)
                    throw Exception("Error creating PNG write structure");
            }
            void write(Bitmap<T>& bitmap)
            {
                _info_ptr = png_create_info_struct(_png_ptr);
                if (_info_ptr == 0)
                    throw Exception("Error creating PNG info structure");
                png_set_write_fn(_png_ptr, static_cast<png_voidp>(_stream),
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
                doWrite<T>();
            }
            ~PNGWrite()
            {
                png_destroy_write_struct(&_png_ptr, &_info_ptr);
            }
        private:
            template<class T2> void doWrite()
            {
                throw Exception();
            }
            template<> void doWrite<SRGB>()
            {
                png_write_png(_png_ptr, _info_ptr, PNG_TRANSFORM_IDENTITY,
                    NULL);
            }
            template<> void doWrite<DWORD>()
            {
                png_write_png(_png_ptr, _info_ptr,
                    PNG_TRANSFORM_BGR | PNG_TRANSFORM_STRIP_FILLER_AFTER,
                    NULL);
            }
            png_structp _png_ptr;
            png_infop _info_ptr;
            png_bytep* _row_pointers;
            FileStream* _stream;
        };
    };
};

#endif // INCLUDED_BITMAP_PNG_H
