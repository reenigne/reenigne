// Classes for Filter graphs for manipulating streams of samples.
//
// A Filter is an object which can produce data, consume data or both (or, in
// the trivial case, neither).
//
// A Filter that produces data but does not consume it is called a source.
// A Filter that consumes data but does not produce it is called a sink.
//
// Filters communicate through Producer and Consumer objects. A combination of
// a Consumer and a Producer forms a circular buffer internally. These buffers
// always grow (never shrink) and are always a power of two number of samples.
// Therefore it is important to limit the amount of data a Filter produces
// before that data is consumed.
//
// Rather than accessing the buffer directly, a Filter obtains a Buffer object
// from the Producer or Consumer which encapsulates reading and writing
// operations. Calls to Buffer methods (especially the "one sample per call"
// methods) are expected to be inlined for maximum filter performance.
//
// Filters can work in two modes, "push mode" (source driven) or "pull mode"
// (sink driven). Not all filters in a filter graph are required to operate in
// the same mode, and any given filter may operate in both push and pull modes
// as necessary.
//
// In "push mode", a Filter produces data and sends it to its Producer. When
// the Producer's buffer gets too full, connected Consumers notify their
// Filters to consume some data. These Filters may in turn produce more data
// and so on.
//
// In "pull mode" a Filter consumes data from a Consumer. If the Consumer's
// buffer is too empty, the connected Producer notifies its Filter to produce
// some data. That Filter may in turn consume more data and so on.
//
// Filters are responsible for deciding how much data they will produce at once
// and how much data they will consume at once (which is also how much data the
// attached buffer will hold before it is considered "full"). For most purposes
// this should generally be on the order of a kilobyte, depending on the number
// of Filters in the graph.
//
// Because Filters can be connected and disconnected at run time, each Filter
// doesn't know at compile time what it is connected to. In an ideal world it
// would be possible to instantiate the compiler back-end at runtime when
// connections are made to generate ideal code for a given Filter graph.
// However, that technology is not currently practical.
//
// Hence, a virtual function call is required for each process() operation. A
// virtual function call cannot be inlined and interferes with a CPU's
// prediction heuristics, so is relatively expensive - perhaps on the order of
// 40 cycles. For data sampled at tens of MHz passing through tens of filters,
// this quickly becomes the limiting factor if a Filter only processes one
// sample per call. Hence Filters process multiple samples per call.
//
// An application will perform best if all the data required by its inner loop
// resides in L1 cache, which is generally 32Kb, so the average buffer size
// multiplied by the number of Connections should be less than this.
//
// Filters with at most one producer and at most one consumer can be
// implemented using the SimpleSource, SimpleSink and Pipe helper classes.
//
// See also: http://dancinghacker.com/code/dataflow/ which is similar.

#ifndef INCLUDED_FILTERS_H
#define INCLUDED_FILTERS_H

#include "unity/minimum_maximum.h"
#include <vector>
#include <string.h>

// Infrastructure

// Number of samples a filter will produce/consume by default.
static const int defaultFilterCount = 1024;

// Base class for all filters. Filters own their endpoints and supply pointers
// to them. Filters are not copyable because the Connection objects connected
// to a Filter contain pointers to the Filter, and it is not clear what these
// pointers should point to after the copy.
class Filter : Uncopyable
{
public:
    virtual void process() = 0;
};


// Describes a circular buffer that can be read from or written to.
template<class T> class Buffer
{
public:
    Buffer() { }
    Buffer(T* buffer, int position, int mask)
      : _buffer(buffer),
        _position(position),
        _mask(mask)
    { }
    void advance(int count) { _position = offset(count); }
    T& item(int position) { return _buffer[offset(position)]; }
    T& item()
    {
        T& sample = _buffer[_position];
        advance(1);
        return sample;
    }
    template<class F> void items(F& f, int count)
    {
        int n = min(count, _mask + 1 - _position);
        f(_buffer + _position, n);
        f(_buffer, count - n);
        advance(count);
    }
    template<class F> void items(F& f, int position, int count)
    {
        int start = offset(position);
        int n = min(count, _mask + 1 - off);
        f(_buffer + start, n);
        f(_buffer, count - n);
    }
private:
    int offset(int delta) { return (delta + _position)&_mask; }

    T* _buffer;
    int _position;
    int _mask;
};


// Functor to memcpy from a Buffer.
template<class T> class CopyTo
{
public:
    CopyTo(T* destination) : _destination(destination) { }
    void operator()(T* source, int n)
    {
        memcpy(_destination, source, n*sizeof(T));
        _destination += n;
    }
private:
    T* _destination;
};


// Specialization to copy from one Buffer to another.
template<class T> class CopyTo<Buffer<T> >
{
public:
    CopyTo(Buffer<T> destination) : _destination(destination) { }
    void operator()(T* source, int n)
    {
        _destination.items(CopyFrom<T>(source), n);
    }
private:
    Buffer<T> _destination;
};


// Functor to memcpy to a Writer.
template<class T> class CopyFrom
{
public:
    CopyFrom(T* source) : _source(source) { }
    void operator()(T* destination, int n)
    {
        memcpy(destination, _source, n*sizeof(T));
        _source += n;
    }
private:
    T* _source;
};


// Specialization to copy from a one Buffer to another.
template<class T> class CopyFrom<Buffer<T> >
{
public:
    CopyFrom(Buffer<T> source) : _source(source) { }
    void operator()(T* destination, int n)
    {
        _source.items(CopyTo<T>(destination), n);
    }
private:
    Buffer<T> _source;
};


// Functor to zero samples in a Buffer using memset.
template<class T> class Zero
{
public:
    void operator()(T* destination, int n)
    {
        memset(destination, 0, n*sizeof(T));
    }
};


// Base class for filter endpoints. An endpoint is either a producer or a
// consumer.
template<class T> class EndPoint : Uncopyable
{
public:
    EndPoint(Filter* filter) : _filter(filter) { }
    int count() const { return _count; }
protected:
    Buffer<T> buffer() { return Buffer<T>(_buffer, _position, _mask); }
    int offset(int delta) { return (delta + _position)&_mask; }

    Filter* _filter;
    T* _buffer;
    int _mask;
    int _position;
    int _count;
};


template<class T> class Producer;


// An input pin on a filter.
template<class T> class Consumer : public EndPoint<T>
{
public:
    Consumer(Filter* filter, int n = defaultFilterCount)
      : EndPoint(filter),
        _n(n)
    { }
    Buffer<T> reader(int n)
    {
        _producer->ensureData(n);
        return buffer();
    }
    void read(int n) { _count -= n; _position = offset(n); _producer->read(); }
private:
    void push()
    {
        while (_count >= _n)
            _filter->process();
    }

    Producer<T>* _producer;
    int _n;

    friend class Producer<T>;
};


// An output pin on a filter.
template<class T> class Producer : public EndPoint<T>
{
public:
    Producer(Filter* filter)
      : EndPoint(filter),
        _size(2),
        _pulling(false)
    {
        _mask = 1;
        _count = 0;
        _buffer = new T[_size];
        _position = 0;
    }
    ~Producer() { delete[] _buffer; }
    void connect(Consumer<T>* consumer)
    {
        _consumers.push_back(consumer);
        consumer->_producer = this;
        consumer->_buffer = _buffer;
        consumer->_position = _position;
        consumer->_mask = _mask;
        consumer->_count = 0;
    }
    void disconnect(Consumer<T>* consumer)
    {
        int n = _consumers.size();
        int i;
        for (i = 0; i < n; ++i)
            if (_consumers[i] == consumer)
                break;
        for (; i < n - 1; ++i)
            _consumers[i] = _consumers[i + 1];
        _consumers.resize(n - 1);
    }
    Buffer<T> writer(int n)
    {
        // Make sure we have enough space for an additional n items
        int newN = n + _count;
        if (_size < newN) {
            // Double the size of the buffer until it's big enough.
            int newSize = _size;
            while (newSize < newN)
                newSize <<= 1;
            // Since buffers never shrink, this doesn't need to be particularly
            // fast. Just copy all the data to the start of the new buffer.
            T* newBuffer = new T[newSize];
            int start = offset(-_count);
            int n1 = min(_count, _size - start);
            memcpy(newBuffer, _buffer + start, n1*sizeof(T));
            memcpy(newBuffer + n1, _buffer, (_size - n1)*sizeof(T));
            delete[] _buffer;
            _position = _count;
            _size = newSize;
            _buffer = newBuffer;
            for (std::vector<Consumer<T>*>::size_type i = 0; i < _consumers.size(); ++i) {
                Consumer<T>* consumer = _consumers[i];
                consumer->_buffer = _buffer;
                consumer->_position = consumer->offset(-start);
                consumer->_mask = _size - 1;
            }
            _mask = _size - 1;
        }
        return buffer();
    }
    void written(int n)
    {
        _position = offset(n);
        for (std::vector<Consumer<T>*>::size_type i = 0; i < _consumers.size(); ++i)
            _consumers[i]->_count += n;
        read();
        if (!_pulling) {
            // If we're pulling, the consumer will process anyway so we don't
            // do it here.
            for (std::vector<Consumer<T>*>::size_type i = 0; i < _consumers.size(); ++i)
                _consumers[i]->push();
        }
    }
private:
    void ensureData(int n)
    {
        _pulling = true;
        while (_count < n)
            _filter->process();
        _pulling = false;
    }
    void read()
    {
        _count = 0;
        for (std::vector<Consumer<T>*>::size_type i = 0; i < _consumers.size(); ++i)
            _count = max(_count, _consumers[i]->_count);
    }

    std::vector<Consumer<T>*> _consumers;
    int _size;
    bool _pulling;

    friend class Consumer<T>;
};


// Disconnects a connection when it goes out of scope.
template<class T> class Connection : Uncopyable
{
public:
    Connection(Producer* producer, Consumer* consumer)
      : _producer(producer),
        _consumer(consumer)
    {
        producer->connect(consumer);
    }
    ~Connection() { _producer->disconnect(_consumer); }
private:
    Producer<T>* _producer;
    Consumer<T>* _consumer;
};


// Base classes to make filter implementation easier.

// Base class that can be used by a source filter with a single producer.
template<class T> class SingleSource : public Filter
{
public:
    SingleSource(int n = defaultFilterCount)
      : _producer(this),
        _n(n)
    { }
    Producer<T>* producer() { return &_producer; }
protected:
    Producer<T> _producer;
    int _n;
};


// Base class that can be used by a sink filter with a single consumer.
template<class T> class SingleSink : public Filter
{
public:
    SingleSink(int n = defaultFilterCount, Producer<T>* producer = 0)
      : _consumer(this),
        _n(n)
    {
        if (producer != 0)
            producer->connect(consumer());
    }
    Consumer<T>* consumer() { return &_consumer; }
protected:
    Consumer<T> _consumer;
    int _n;
};


// Base class that can be used by a filter with a single producer and a single
// consumer.
template<class ProducedT, class ConsumedT> class Pipe : public Filter
{
public:
    Pipe(int n = defaultFilterCount, Producer<ConsumedT>* producer = 0)
      : _consumer(this),
        _producer(this),
        _n(n)
    {
        if (producer != 0)
            producer->connect(consumer());
    }
    Producer<ProducedT>* producer() { return &_producer; }
    Consumer<ConsumedT>* consumer() { return &_consumer; }
protected:
    Producer<ProducedT> _producer;
    Consumer<ConsumedT> _consumer;
    int _n;
};


// Filter implementations

// A filter that always produces the same sample and never pushes.
template<class T> class ConstantSource : public SingleSource<T>
{
public:
    ConstantSource(T sample, int n = defaultFilterCount)
      : SingleSource(n)
        _sample(sample)
    { }
    void process()
    {
        Buffer<T> writer = _producer.writer(n);
        for (int i = 0; i < n; ++i)
            writer.item() = _sample;
        _producer.written(n);
    }
private:
    T _sample;
};


// Data for a PeriodicSource filter. This is separate from PeriodicSource so
// that several producers can use the same data.
template<class T> class PeriodicSourceData
{
public:
    PeriodicSourceData(int size) : _buffer(size) { }

    // Function for accessing the underlying buffer directly (to fill it, and
    // for use by PeriodicSource).
    T* buffer() { return &_buffer[0]; }

    // For use by PeriodicSource.
    int length() const { return _buffer.size(); }
private:
    std::vector<T> _buffer;
};


// A producer that produces the same information repeatedly (pull only).
template<class T> class PeriodicSource : public SingleSource<T>
{
public:
    PeriodicSource(PeriodicSourceData<T>* data, int offset, int n = defaultFilterCount)
      : SingleSource(n),
        _data(data),
        _offset(offset)
    { }
    void process()
    {
        Buffer<T> writer = _producer.writer(_n);
        int remaining = _n;
        T* buffer = _data->buffer();
        int length = _data->length();
        int n1 = min(_n, length - _offset);
        writer.items(CopyFrom<T>(buffer + _offset), n1);
        remaining -= n1;
        while (remaining > length) {
            writer.items(CopyFrom<T>(buffer), length);
            remaining -= length;
        }
        writer.items(CopyFrom<T>(buffer), remaining);
        _producer.written(_n);
        _offset = (_offset + _n)%length;
    }
private:
    PeriodicSourceData<T>* _data;
    int _offset;
};


// A pipe that just produces exactly what it consumes. This is useful if you
// have multiple consumers that you want to connect to a single producer and
// one or more of the consumers modifies the data.
template<class T> class NopPipe : public Pipe<T, T>
{
public:
    NopPipe(int n = defaultFilterCount, Producer<T>* producer = 0)
      : Pipe(n, producer)
    { }
    void process()
    {
        _producer.writer(_n).items(CopyFrom<Buffer<T> >(_consumer.reader(_n)), _n);
        _consumer.read(n);
        _producer.written(n);
    }
};


// A filter that converts from one type to another (with static_cast<>).
template<class ProducedT, class ConsumedT> class CastPipe : public Pipe<ProducedT, ConsumedT>
{
public:
    CastPipe(int n = defaultFilterCount, Producer<ConsumedT>* producer = 0)
      : Pipe(n, producer)
    { }
    void process()
    {
        Buffer<ConsumedT> reader = _consumer.reader(_n);
        Buffer<ProducedT> writer = _producer.writer(_n);
        for (int i = 0; i < _n; ++i)
            writer.item() = static_cast<ProducedT>(reader.item());
        _consumer.read(_n);
        _producer.written(_n);
    }
};


// A pipe that interpolates using the nearest-neighbor algorithm.
template<class T, class Interpolator = int> class NearestNeighborInterpolator : public Pipe<T, T>
{
public:
    // For every "consumerRate" samples consumed we will produce "producerRate" samples.
    NearestNeighborInterpolator(Interpolator producerRate, Interpolator consumerRate, int n = defaultFilterCount, Interpolator offset = 0, Producer<T>* producer = 0)
      : Pipe(n, producer),
        _producerRate(producerRate),
        _consumerRate(consumerRate),
        _offset(offset)
    { }
    void process()
    {
        // TODO: We can probably speed this up somewhat by copying blocks
        Buffer<T> reader = _consumer.reader(_n);
        Buffer<T> writer = _producer.writer(static_cast<int>((static_cast<Interpolator>(_n)*_producerRate)/_consumerRate) + 1);
        int written = 0;
        for (int i = 0; i < _n; ++i) {
            T sample = reader.item();
            while (_offset >= 0) {
                writer.item() = sample;
                ++written;
                _offset -= _producerRate;
            }
            _offset += _consumerRate;
        }
        _consumer.read(_n);
        _producer.written(written);
    }
private:
    Interpolator _producerRate;
    Interpolator _consumerRate;
    Interpolator _offset;
};

// A pipe that interpolates using the linear interpolation.
template<class T, class Interpolator = int> class LinearInterpolator : public Pipe<T, T>
{
public:
    // For every "consumerRate" samples consumed we will produce "producerRate" samples.
    LinearInterpolator(Interpolator producerRate, Interpolator consumerRate, int n = defaultFilterCount, Interpolator offset = 0, T previous = 0, Producer<T>* producer = 0)
      : Pipe(n, producer),
        _producerRate(producerRate),
        _consumerRate(consumerRate),
        _offset(offset),
        _previous(0)
    { }
    void process()
    {
        Buffer<T> reader = _consumer.reader(_n);
        Buffer<T> writer = _producer.writer(static_cast<int>((static_cast<Interpolator>(_n)*_producerRate)/_consumerRate) + 1);
        int written = 0;
        for (int i = 0; i < _n; ++i) {
            T sample = reader.item();
            while (_offset >= 0) {
                writer.item() = sample + static_cast<T>((static_cast<Interpolator>(_previous - sample)*_offset)/_consumerRate);
                ++written;
                _offset -= _producerRate;
            }
            _offset += _consumerRate;
            _previous = sample;
        }
        _consumer.read(n);
        _producer.written(written);
    }
private:
    Interpolator _producerRate;
    Interpolator _consumerRate;
    Interpolator _offset;
    T _previous;
};

#endif // INCLUDED_FILTERS_H
