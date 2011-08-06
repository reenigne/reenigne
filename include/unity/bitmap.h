#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

#include "unity/vectors.h"

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
        // TODO
    }
    void save(const File& file)
    {
        // TODO
    }
    Byte* data() const { return &_data[0]; }
    int stride() const { return _stride; }
    Vector size() const { return _size; }
private:
    Vector _size;
    int _stride;
    Array<Byte> _data;
};

#endif // INCLUDED_BITMAP_H
