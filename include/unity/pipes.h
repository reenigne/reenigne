// Classes for Filter graphs for manipulating streams of samples.
//
// A Filter is an object which can produce data, consume data or both (or, in
// the trivial case, neither).
//
// A Filter that produces data but does not consume is called a Source.
// A Filter that consumes data but does not produce is called a Sink.
// A Filter that both produces and consumes is called a Pipe.
//
// Filters communicate through Source and Sink objects. A combination of a
// Source and a Sink forms a circular buffer internally. These buffers always
// grow (never shrink) and are always a power of two number of samples.
// Therefore it is important to limit the amount of data a Filter produces
// before that data is consumed.
//
// Rather than accessing the buffer directly, a Filter obtains an Accessor
// object from the Source or Sink which encapsulates reading and writing
// operations. Calls to Accessor methods (especially the "one sample per call"
// methods) are expected to be inlined for maximum filter performance.
//
// Filters can work in two modes, "push mode" (Source driven) or "pull mode"
// (Sink driven). Not all filters in a filter graph are required to operate in
// the same mode, and any given filter may operate in both push and pull modes
// as necessary.
//
// In "push mode", a Filter produces data via one or more Source objects. When
// a Source's buffer gets too full, the connected Sink is notified to consume
// some data. These Filters that these Sinks are connected to may in turn
// produce more data and so on.
//
// In "pull mode" a Filter consumes data via one or more Sink objects. When a
// Sink's buffer is too empty, the connected Source is notified to produce some
// data. That Filter may in turn consume more data and so on.
//
// Filters are responsible for deciding how much data they will produce at once
// and how much data they will consume at once (which is also how much data the
// attached buffer will hold before it is considered "full"), though a hint is
// passed in to produce() (the number of samples required) and to consume()
// (the number of samples available). For most purposes this should generally
// be on the order of a kilobyte, depending on the number of Filters in the
// graph.
//
// Because Filters can be connected and disconnected at run time, each Filter
// doesn't know at compile time what it is connected to. In an ideal world it
// would be possible to instantiate the compiler back-end at runtime when
// connections are made to generate ideal code for a given Filter graph.
// However, that technology is not currently practical.
//
// Hence, a virtual function call is required for each produce() or consume()
// operation. A virtual function call cannot be inlined and interferes with a
// CPU's prediction heuristics, so is relatively expensive - perhaps on the
// order of 40 cycles. For data sampled at tens of MHz passing through tens of
// filters, this quickly becomes the limiting factor if a Filter only
// processes one sample per call. Hence Filters process multiple samples per
// call.
//
// An application will perform best if all the data required by its inner loop
// resides in L1 cache, which is generally 32Kb, so the average buffer size
// multiplied by the number of Connections should be less than this.
//
// A Pipe with a single Source and a single Sink can be implemented using the
// Pipe helper class.
//
// Sources can specify 
//
// See also: http://dancinghacker.com/code/dataflow/ which is similar.

#ifndef INCLUDED_PIPES_H
#define INCLUDED_PIPES_H

#include "unity/minimum_maximum.h"
#include "unity/uncopyable.h"
#include "unity/thread.h"
#include "unity/file.h"
#include <vector>
#include <string.h>

// Infrastructure

// Number of samples a Sink will accumulate by default before consume() is
// called.
static const int defaultSampleCount = 1024;

// Base class for all filters. Filters are not copyable because connected
// filters will have pointers to them or their members.
class Filter : Uncopyable { };


// Describes a circular buffer that can be read from or written to.
template<class T> class Accessor
{
public:
    Accessor() { }
    Accessor(T* buffer, int position, int mask)
      : _buffer(buffer),
        _position(position),
        _mask(mask)
    { }
    void advance(int count) { _position = offset(count); }
    T& item(int position) { return _data[offset(position)]; }
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


// Functor to memcpy from an Accessor.
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


// Specialization to copy from one Accessor to another.
template<class T> class CopyTo<Accessor<T> >
{
public:
    CopyTo(Accessor<T> destination) : _destination(destination) { }
    void operator()(T* source, int n)
    {
        _destination.items(CopyFrom<T>(source), n);
    }
private:
    Accessor<T> _destination;
};


// Functor to memcpy to an Accessor.
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


// Specialization to copy from a one Accessor to another.
template<class T> class CopyFrom<Accessor<T> >
{
public:
    CopyFrom(Accessor<T> source) : _source(source) { }
    void operator()(T* destination, int n)
    {
        _source.items(CopyTo<T>(destination), n);
    }
private:
    Accessor<T> _source;
};


// Functor to zero samples in an Accessor using memset.
template<class T> class Zero
{
public:
    void operator()(T* destination, int n)
    {
        memset(destination, 0, n*sizeof(T));
    }
};


// Functor to read samples from a FileHandle.
template<class T> class ReadFrom
{
public:
    ReadFrom(FileHandle* handle) : _handle(handle) { }
    void operator()(T* destination, int n)
    {
        _handle->read(destination, n);
    }
private:
    FileHandle* _handle;
};


// Functor to write samples to a FileHandle.
template<class T> class WriteTo
{
public:
    WriteTo(FileHandle* handle) : _handle(handle) { }
    void operator()(T* source, int n)
    {
        _handle->write(source, n);
    }
private:
    FileHandle* _handle;
};


// Base class for filter endpoints. An EndPoint is either a Source or a Sink.
template<class T> class EndPoint : public Filter
{
protected:
    Accessor<T> accessor() { return Accessor<T>(_buffer, _position, _mask); }
    int offset(int delta) { return (delta + _position)&_mask; }

    T* _buffer;
    int _mask;
    int _position;
    int _count;
    int _size;
};


template<class T> class Source;

// A Sink is the means by which a Filter consumes data. Filters with a single
// Sink and no Source can just inherit from Sink, others should include it as
// a member. The argument to the constructor should be increased if
// defaultSampleCount is too few samples for consume() to do anything with,
// or decreased if it's so many that latency will be too high.
template<class T> class Sink : public EndPoint<T>
{
public:
    Sink(int n = defaultSampleCount) : _n(n), _samplesToEnd(-1) { }
    void connect(Source<T>* source)
    {
        if (_source == source)
            return;
        _source = source;
        _buffer = source->_buffer;
        _count = source->_count;
        _size = source->_size;
        _mask = source->_mask;
        _position = (source->_position + _size - _count) & _mask;
        source->connect(this);
    }
    virtual void consume(int n) = 0;
    virtual void finish() { }   // How a Source can tell a Sink that its data stream is coming to an end
    Accessor<T> reader(int n) { _source->ensureData(n); return accessor(); }
    void read(int n)
    {
        _count -= n;
        _position = offset(n);
        _source->_count -= n;
        if (_samplesToEnd > 0) {
            _samplesToEnd -= n;
            if (_samplesToEnd <= 0)
                finish();
        }
    }
    void consume()
    {
        while (_count >= _n)
            consume(_count);
    }
private:
    void ending()
    {
        _samplesToEnd = _count;
    }
    Source<T>* _source;
    int _n;
    int _samplesToEnd;

    friend class Source<T>;
};


// A Source is the means by which a Filter produces data. Filters with a
// single Source and no Sink can just inherit from Source, others should
// include it as a member.
template<class T> class Source : public EndPoint<T>
{
public:
    Source()
    {
        _position = 0;
        _size = 1;
        _count = 0;
        _mask = 0;
        _buffer = new T[1];
    }
    ~Source() { delete[] _buffer; }
    void connect(Sink<T>* sink)
    {
        if (_sink == sink)
            return;
        _sink = sink;
        sink->connect(this);
    }
    virtual void produce(int n) = 0;
    Accessor<T> writer(int n)
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
            _mask = _size - 1;
            _sink->_buffer = _buffer;
            _sink->_size = _size;
            _sink->_mask = _mask;
            _sink->_position = 0;
        }
        return accessor();
    }
    void written(int n)
    {
        _position = offset(n);
        _count += n;
        _sink->_count += n;
        // If we're pulling, the consumer will process anyway so we don't
        // do it here.
        if (!_pulling)
            _sink->consume();
    }
    void ensureData(int n)
    {
        _pulling = true;
        while (_count < n)
            produce(n - _count);
        _pulling = false;
    }
    void finish() { _sink->ending(); }
private:
    Sink<T>* _sink;
    bool _pulling;

    friend class Sink<T>;
};


// Base classes to make filter implementation easier.

// Base class that can be used by a filter with a single Source and a single
// Sink.
template<class ConsumedT, class ProducedT, class P> class Pipe
    : public Filter
{
public:
    Pipe(P* p, int n = defaultSampleCount)
      : _source(p), _sink(p, n), _finished(false)
    { }
    Source<ProducedT>* source() { return &_source; }
    Sink<ConsumedT>* sink() { return &_sink; }
    virtual void produce(int n) { consume(n); }
    virtual void consume(int n) { produce(n); }
    void finish() { _finished = true; }
protected:
    class PipeSource : public Source<ProducedT>
    {
    public:
        PipeSource(P* p) : _p(p) { }
        void produce(int n) { _p->produce(n); }
    private:
        P* _p;
    };
    class PipeSink : public Sink<ConsumedT>
    {
    public:
        PipeSink(P* p, int n) : Sink(n), _p(p) { }
        void consume(int n) { _p->consume(n); }
        void finish() { _p->finish(); }
    private:
        P* _p;
    };
    PipeSource _source;
    PipeSink _sink;
    bool _finished;
};


// Filter implementations

// A Sink that does nothing with its data. Useful when you have a Filter with
// multiple Sources but don't need the data from all of them. Push only.
template<class T> class BitBucketSink : public Sink<T>
{
public:
    void consume(int n) { reader(n); read(n); }
};


// A Source that always produces the same sample. Pull only. Never finishes.
template<class T> class ConstantSource : public Source<T>
{
public:
    ConstantSource(T sample) : _sample(sample) { }
    void produce(int n)
    {
        Accessor<T> w = writer(n);
        for (int i = 0; i < n; ++i)
            w.write(_sample);
        written(n);
    }
private:
    T _sample;
};


// Data for a PeriodicSource filter. This is separate from PeriodicSource so
// that several producers can use the same data.
// TODO: initialize from File.
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


// A Source that produces the same information repeatedly. Pull only. Never finishes.
template<class T> class PeriodicSource : public Source<T>
{
public:
    PeriodicSource(PeriodicSourceData<T>* data, int offset)
      : _data(data),
        _offset(offset)
    { }
    void produce(int n)
    {
        int length = _data->length();
        T* buffer = _data->buffer();
        if (n < length - _offset) {
            writer(n).items(CopyFrom<T>(buffer + _offset), n);
            _offset += n;
        }
        else {
            n = length - _offset;
            writer(n).items(CopyFrom<T>(buffer + _offset), n);
            _offset = 0;
        }
        written(n);
    }
private:
    PeriodicSourceData<T>* _data;
    int _offset;
};


// A pipe that just takes samples from its Sink and sends them directly to its
// Source. This is useful as a pump: external code can call the produce()
// method directly to cause data to be pulled via the Sink and then pushed via
// the Source.
template<class T> class NopPipe : public Pipe<T, T, NopPipe<T> >
{
public:
    void produce(int n)
    {
        _source.writer(n).items(CopyFrom<Accessor<T> >(_sink.reader(n)), n);
        _sink.read(n);
        _source.written(n);
        if (_finished)
            _source.finish();
    }
};


// A filter that converts from one type to another (with static_cast<>).
template<class ProducedT, class ConsumedT> class CastPipe
  : public Pipe<ProducedT, ConsumedT, CastPipe<ProducedT, ConsumedT> >
{
public:
    void produce(int n)
    {
        Accessor<ConsumedT> reader = _sink.reader(n);
        Accessor<ProducedT> writer = _source.writer(n);
        for (int i = 0; i < n; ++i)
            writer.item() = static_cast<ProducedT>(reader.item());
        _sink.read(n);
        _source.written(n);
        if (_finished)
            _source.finish();
    }
};


// A pipe that interpolates using the nearest-neighbor algorithm.
template<class T, class Rate = int> class NearestNeighborInterpolator
  : public Pipe<T, T, NearestNeighborInterpolator<T, Rate> >
{
public:
    // For every "consumerRate" samples consumed we will produce "producerRate" samples.
    NearestNeighborInterpolator(Rate producerRate, Rate consumerRate, Rate offset = 0)
      : _producerRate(producerRate),
        _consumerRate(consumerRate),
        _offset(offset)
    { }
    void produce(int n)
    {
        // TODO: We can probably speed this up somewhat by copying blocks
        Accessor<T> reader = _sink.reader(n);
        Accessor<T> writer = _source.writer(static_cast<int>((static_cast<Rate>(n)*_producerRate)/_consumerRate) + 1);
        int written = 0;
        for (int i = 0; i < n; ++i) {
            T sample = reader.item();
            while (_offset >= 0) {
                writer.item() = sample;
                ++written;
                _offset -= _producerRate;
            }
            _offset += _consumerRate;
        }
        _sink.read(n);
        _source.written(written);
        if (_finished)
            _source.finish();
    }
private:
    Rate _producerRate;
    Rate _consumerRate;
    Rate _offset;
};


// A pipe that interpolates using the linear interpolation. TODO: modify this so it downsamples as well.
template<class T, class Rate = int> class LinearInterpolator
  : public Pipe<T, T, LinearInterpolator<T, Rate> >
{
public:
    // For every "consumerRate" samples consumed we will produce "producerRate" samples.
    LinearInterpolator(Rate producerRate, Rate consumerRate, Rate offset = 0, T previous = 0)
      : _producerRate(producerRate),
        _consumerRate(consumerRate),
        _offset(offset),
        _previous(0)
    { }
    void produce(int n)
    {
        Accessor<T> reader = _sink.reader(n);
        Accessor<T> writer = _source.writer(static_cast<int>(
            (static_cast<Rate>(n)*_producerRate)/_consumerRate) + 1);
        int written = 0;
        for (int i = 0; i < n; ++i) {
            T sample = reader.item();
            while (_offset >= 0) {
                writer.item() = sample + static_cast<T>((static_cast<Rate>(_previous - sample)*_offset)/_consumerRate);
                ++written;
                _offset -= _producerRate;
            }
            _offset += _consumerRate;
            _previous = sample;
        }
        _sink.read(n);
        _source.written(written);
        // TODO: Should we avoid calling finish() until we're done with the
        // last sample that came from before finish() was called?
        // Should we be keeping track of the number of post-finish samples?
        if (_finished)
            _source.finish();
    }
private:
    Rate _producerRate;
    Rate _consumerRate;
    Rate _offset;
    T _previous;
};


// A pipe that neither pushes or pulls. If you try to push to it without
// pulling, it continues to accumulate data until it runs out of memory. If
// you try to pull from it without pushing, it blocks until data is pushed.
template<class T, class C = Tank<T> > class Tank : public Pipe<T, T, C>
{
public:
    Tank()
      : _buffer(new T[1]),
        _size(1),
        _count(0),
        _mask(0),
        _readPosition(0),
        _writePosition(0)
    { }
    void produce(int n)
    {
        while (_count < n)
            _event.wait();
        Lock lock(&_mutex);
        _source.writer(n).items(CopyFrom<Accessor<T> >(reader()), n);
        _readPosition = (_readPosition + n) & _mask;
        _count -= n;
        _source.written(n);
        if (_finished)
            _source.finish();
    }
    void consume(int n)
    {
        Lock lock(&_mutex);
        // Make sure we have enough space for an additional n items
        int newN = n + _count;
        if (_size < newN) {
            // Double the size of the buffer until it's big enough.
            int newSize = _size;
            while (newSize < newN)
                newSize <<= 1;
            T* newBuffer = new T[newSize];
            int start = offset(-_count);
            int n1 = min(_count, _size - start);
            memcpy(newBuffer, _buffer + start, n1*sizeof(T));
            memcpy(newBuffer + n1, _buffer, (_size - n1)*sizeof(T));
            delete[] _buffer;
            _writePosition = _count;
            _readPosition = 0;
            _size = newSize;
            _buffer = newBuffer;
            _mask = _size - 1;
        }
        writer().items(CopyFrom<Accessor<T> >(_sink.reader(n)), n);
        _sink.read(n);
        _writePosition = (_writePosition + n) & _mask;
        _count += n;
        _event.signal();
    }
private:
    Mutex _mutex;
    Event _event;

    Accessor<T> reader() { return Accessor<T>(_buffer, _readPosition, _mask); }
    Accessor<T> writer() { return Accessor<T>(_buffer, _writePosition, _mask); }

    T* _buffer;
    int _size;
    volatile int _count;
    int _mask;
    int _readPosition;
    int _writePosition;
};


// A pipe that can be both pulled from and pushed to, and which adjusts the
// rate of a connected interpolator to match the pull rate to the push rate.
// The "timeConstant" parameter must be tuned. If it is too small, the
// resampling rate will fluctuate wildly as samples are pushed and pulled. If
// it is too large, it will take too long to adjust to changes in the push or
// pull rates, leading to high latencies or stalls waiting for the connected
// Source to push (PushPullPipe will never push or pull itself). "timeConstant"
// is measured in samples consumed.
template<class T, class Interpolator> class PushPullPipe
  : public Tank<T, PushPullPipe<T, Interpolator> >
{
public:
    PushPullPipe(int timeConstant, Interpolator* interpolator)
      : _timeConstant(timeConstant),
        _interpolator(interpolator)
    { }
    void produce(int n)
    {
        _produced += n*_rate;
        updateRate();
        Tank::produce(n);
    }
    void consume(int n)
    {
        double c = exp(-n/_timeConstant);
        _produced *= c;
        _consumed *= c;
        _consumed += n;
        updateRate();
        Tank::consume(n);
    }
private:
    void updateRate()
    {
        // TODO: Adjust slightly so that we speed up if we have a large number of samples
        _rate = _produced/_consumed;
        _interpolator->setRate(_rate);
    }

    int _timeConstant;
    Interpolator* _interpolator;
    double _produced;
    double _consumed;
    double _rate;
};

#endif // INCLUDED_PIPES_H
