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
// Pipe helper classes.
//
// See also: http://dancinghacker.com/code/dataflow/ which is similar.

#ifndef INCLUDED_PIPES_H
#define INCLUDED_PIPES_H

#include "unity/minimum_maximum.h"
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
    Accessor(T* data, int position, int mask)
      : _data(data),
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
    CopyTo(Buffer<T> destination) : _destination(destination) { }
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
    CopyFrom(Buffer<T> source) : _source(source) { }
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


// Base class for filter endpoints. An EndPoint is either a Source or a Sink.
template<class T> class EndPoint : public Filter
{
protected:
    Accessor<T> buffer() { return Buffer<T>(_buffer, _position, _mask); }
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
// or if it's so many that latency will be too high.
template<class T> class Sink : public EndPoint
{
public:
    Sink(int n = defaultSampleCount) : _n(n) { }
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
    Accessor<T> reader(int n) { _source->ensureData(n); return buffer(); }
    void read(int n)
    {
        _count -= n;
        _position = offset(n);
        _source->_count -= n;
    }
    void consume()
    {
        while (_count >= _n)
            consume(_count);
    }
private:
    Source<T>* _source;
    int _n;
};


// A Source is the means by which a Filter produces data. Filters with a
// single Source and no Sink can just inherit from Source, others should
// include it as a member.
template<class T> class Source : public EndPoint
{
public:
    Source()
      : _buffer(new T[1]),
        _mask(0),
        _count(0),
        _size(1),
        _position(0)
    { }
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

            // Disconnect and reconnect to refresh the Sink's data.
            Sink<T>* sink = _sink;
            connect(0);
            connect(sink);
        }

        return buffer();
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
            produce();
        _pulling = false;
    }
private:
    Sink<T>* _sink;
    bool _pulling;
};


// Base classes to make filter implementation easier.

// Base class that can be used by a filter with a single Source and a single
// Sink.
template<class ProducedT, class ConsumedT, class P> class Pipe : public Filter
{
    Pipe(int n = defaultSampleCount) : _source(this), _sink(this, n) { }
    Source<T>* source() { return &_source; }
    Sink<T>* sink() { return &_sink; }
    void consume(int n) { produce(n); }
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
    private:
        P* _p;
    };
    PipeSource _source;
    PipeSink _sink;
};


// Filter implementations

// A Sink that does nothing with its data. Useful when you have a Filter with
// multiple Sources but don't need the data from all of them. Push only.
template<class T> class BitBucketSink : public Sink<T>
{
public:
    void consume(int n) { reader(n); read(n); }
};


// A Source that always produces the same sample. Pull only.
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


// A Source that produces the same information repeatedly. Pull only.
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
        _source.writer(n).items(CopyFrom<Buffer<T> >(_sink.reader(n)), n);
        _sink.read(n);
        _source.written(n);
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
    }
    void consume(int n) { produce(n); }
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
    LinearInterpolator(Rate producerRate, Rate consumerRate,Rate offset = 0, T previous = 0)
      : _producerRate(producerRate),
        _consumerRate(consumerRate),
        _offset(offset),
        _previous(0)
    { }
    void produce(int n)
    {
        Accessor<T> reader = _sink.reader(n);
        Accessor<T> writer = _source.writer(static_cast<int>((static_cast<Rate>(n)*_producerRate)/_consumerRate) + 1);
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
    }
private:
    Rate _producerRate;
    Rate _consumerRate;
    Rate _offset;
    T _previous;
};


// A pipe that can be both pulled from and pushed to, and which resamples its
// data to match the pull and push rates. The "timeConstant" parameter must be
// tuned. If it is too small, the resampling rate will fluctuate wildly as
// samples are pushed and pulled. If it is too large, it will take too long to
// adjust to changes in the push or pull rates, leading to high latencies or
// stalls waiting for the connected Source to push (PushPullPipe will never
// push or pull itself). "timeConstant" is measured in samples consumed.
// TODO: this needs to be thread-safe, as generally one thread will be pushing
// and another pulling.
template<class T, class Interpolator> class PushPullPipe
  : public Pipe<T, T, PushPullPipe<T, Interpolator> >
{
public:
    PushPullPipe(int timeConstant, Interpolator* interpolator)
      : _timeConstant(timeConstant),
        _interpolator(interpolator),
        _interpolatorSource(this),
        _interpolatorSink(this)
    {
        _interpolatorSink.connect(interpolator->source());
        _interpolatorSource.connect(interpolator->sink());
    }
    void produce(int n)
    {
        // TODO: account for n samples being pulled from us

    }
    void consume(int n)
    {
        // TODO: account for n samples being pushed to us

    }
    void interpolatorProduce(int n)
    {
        // TODO: account for the interpolator pulling n samples from us
    }
    void interpolatorConsume(int n)
    {
        // TODO: account for the interpolator pushing n samples to us
    }
private:
    class InterpolatorSource : public Source<T>
    {
    public:
        InterpolatorSource(PushPullPipe* p) : _p(p) { }
        void produce(int n) { _p->interpolatorProduce(n); }
    private:
        PushPullPipe* _p;
    };
    class InterpolatorSink : public Sink<T>
    {
    public:
        InterpolatorSink(PushPullPipe* p) : _p(p) { }
        void consume(int n) { _p->interpolatorConsume(n); }
    private:
        PushPullPipe* _p;
    };
    InterpolatorSource _interpolatorSource;
    InterpolatorSink _interpolatorSink;

    int _timeConstant;
    Interpolator* _interpolator;
};

#endif // INCLUDED_PIPES_H
