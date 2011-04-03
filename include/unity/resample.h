#ifndef INCLUDED_RESAMPLE_H
#define INCLUDED_RESAMPLE_H

#include "unity/filters.h"
#include "unity/lcm.h"

// A pipe that interpolates using band-limited interpolation.
template<class T, class class Rate = int> class BandLimitedInterpolator : public Pipe<T, T>
{
public:
    // For every "consumerRate" samples consumed we will produce "producerRate" samples.
    BandLimitedInterpolator(Rate producerRate, Rate consumerRate, int n = defaultFilterCount, Rate offset = 0, Producer<T>* producer = 0)
      : Pipe(n, producer),
        _producerRate(producerRate),
        _consumerRate(consumerRate),
        _offset(offset),
        _previous(0)
    {

    }
    void process()
    {
        Buffer<T> reader = _consumer.reader(_n);
        Buffer<T> writer = _producer.writer(static_cast<int>((static_cast<Interpolator>(_n)*_producerRate)/_consumerRate) + 1);
//        int written = 0;
//        for (int i = 0; i < _n; ++i) {
//            T sample = reader.item();
//            while (_offset >= 0) {
//                writer.item() = sample + static_cast<T>((static_cast<Interpolator>(_previous - sample)*_offset)/_consumerRate);
//                ++written;
//                _offset -= _producerRate;
//            }
//            _offset += _consumerRate;
//            _previous = sample;
//        }
        _consumer.read(n);
        _producer.written(written);
    }
private:
    Rate _producerRate;
    Rate _consumerRate;
    Rate _offset;
    Array<T> _kernel;
    T _previous;

};

#endif // INCLUDED_RESAMPLE_H
