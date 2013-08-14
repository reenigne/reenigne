#include "alfe/main.h"

#ifndef INCLUDED_CIRCULAR_BUFFER_H
#define INCLUDED_CIRCULAR_BUFFER_H

template<class T> class CircularBuffer
{
public:
    CircularBuffer()
      : _buffer(0), _readPosition(0), _writePosition(0), _mask(1), _count(0),
        _size(0)
    { }
    ~CircularBuffer() {  if (_buffer != 0) delete[] _buffer; }
    void write(const T& t)
    {
        add(1);
        _buffer[writeOffset(0)] = t;
        added(1);
    }
    const T& read(int n = 0) const
    {
        return _buffer[readOffset(n)];
    }
    int readBoundary() const { return _size - _readPosition; }
    int writeBoundary() const { return _size - _writePosition; }
    T* readPointer() const { return _buffer + _readPosition; }
    T* writePointer() const { return _buffer + _writePosition; }
    T* lowPointer() const { return _buffer; }
    void copyIn(T* source, int first, int n)
    {
        int start = writeOffset(first);
        int n1 = min(n, _size - start);
        memcpy(_buffer + start, source, n1*sizeof(T));
        memcpy(_buffer, source + n1, (n - n1)*sizeof(T));
    }
    void copyOut(T* destination, int first, int n)
    {
        int start = readOffset(first);
        int n1 = min(n, _size - start);
        memcpy(destination, _buffer + start, n1*sizeof(T));
        memcpy(destination + n1, _buffer, (n - n1)*sizeof(T));
    }

    int count() const { return _count; }

    void add(int n)
    {
        // Make sure we have enough space for an additional n items
        int newN = n + _count;
        if (_size < newN) {
            // Double the size of the buffer until it's big enough.
            int newSize = _size;
            if (newSize == 0)
                newSize = 1;
            while (newSize < newN)
                newSize <<= 1;
            // Since buffers never shrink, this doesn't need to be particularly
            // fast. Just copy all the data to the start of the new buffer.
            T* newBuffer = new T[newSize];
            if (_buffer != 0) {
                copyOut(newBuffer, 0, _count);
                delete[] _buffer;
            }
            _writePosition = _count;
            _readPosition = 0;
            _size = newSize;
            _buffer = newBuffer;
            _mask = _size - 1;
        }
    }
    void added(int n)
    {
        _count += n;
        _writePosition = writeOffset(n);
    }
    void remove(int n)
    {
        _count -= n;
        _readPosition = readOffset(n);
    }
private:
    int readOffset(int delta) const { return (delta + _readPosition)&_mask; }
    int writeOffset(int delta) const { return (delta + _writePosition)&_mask; }

    T* _buffer;
    int _readPosition;
    int _writePosition;
    int _mask;
    int _count;
    int _size;
};

#endif // INCLUDED_CIRCULAR_BUFFER_H
