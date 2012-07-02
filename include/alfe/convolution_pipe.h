// Pipes that convolve with a kernel function to act as a finite impulse
// response filter, and classes to define such kernel functions.

#include "alfe/main.h"

#ifndef INCLUDED_CONVOLUTION_PIPE_H
#define INCLUDED_CONVOLUTION_PIPE_H

#include "alfe/pipes.h"
#include "alfe/gcd.h"
#include "float.h"

class ProductKernel;

// A convolution kernel is indexed in such a way that the input samples
// correspond to integer input values.
class ConvolutionKernel
{
public:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual double operator()(double x) const = 0;
        virtual double leftExtent() const { return -DBL_MAX; }
        virtual double rightExtent() const { return DBL_MAX; }
    };

    ConvolutionKernel(Implementation* implementation)
      : _implementation(implementation) { }
    double operator()(double x) const { return (implementation->operator())(x); }
    double leftExtent() const { return _implementation->leftExtent(); }
    double rightExtent() const { return _implementation->rightExtent(); }
    ConvolutionKernel& operator*=(const ConvolutionKernel& right)
    {
        _implementation = new ProductKernel(*this, right);
    }
    ConvolutionKernel& operator+=(const ConvolutionKernel& right)
    {
        _implementation = new SumKernel(*this, right);
    }
    ConvolutionKernel& operator-=(const ConvolutionKernel& right)
    {
        _implementation = new SumKernel(*this, -right);
    }
    ConvolutionKernel operator-() const
    {
        return ProductKernel(*this, ConstantKernel());
    }
    ConvolutionKernel operator*(const ConvolutionKernel& right)
    {
        ConvolutionKernel k = *this; k *= right; return k;
    }
    ConvolutionKernel operator+(const ConvolutionKernel& right)
    {
        ConvolutionKernel k = *this; k += right; return k;
    }
    ConvolutionKernel operator-(const ConvolutionKernel& right)
    {
        ConvolutionKernel k = *this; k -= right; return k;
    }
private:
    Reference<Implementation> _implementation;
};

// Sinc filter corresponds to perfect band-limited interpolation - it removes
// all frequencies higher than the sample rate (if x is measured in samples).
class SincFilter : public ConvolutionKernel
{
public:
    SincFilter() : ConvolutionKernel(new Implementation) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
        virtual double operator()(double x) const
        {
            return sin(M_PI*x)/(M_PI*x);
        }
    };
};

class RectangleWindow : public ConvolutionKernel
{
public:
    RectangleWindow(double semiWidth)
      : ConvolutionKernel(new Implementation(semiWidth)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(double semiWidth) : _semiWidth(semiWidth) { }
        virtual double operator()(double x) const { return 1; }
        virtual double leftExtent() const { return -_semiWidth; }
        virtual double rightExtent() const { return _semiWidth; }
    private:
        double _semiWidth;
    };
};

class ScaledFilter : public ConvolutionKernel
{
public:
    ScaledFilter(ConvolutionKernel kernel, double scale)
      : ConvolutionKernel(new Implementation(kernel, scale)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(ConvolutionKernel kernel, double scale)
          : _kernel(kernel), _scale(scale) { }
        virtual double operator()(double x) const
        {
            return _kernel(x / _scale);
        }
        virtual double leftExtent() const
        {
            return _kernel.leftExtent() * _scale;
        }
        virtual double rightExtent() const
        {
            return _kernel.rightExtent() * _scale;
        }
    private:
        ConvolutionKernel _kernel;
        double _scale;
    };
};

class ProductKernel : public ConvolutionKernel
{
public:
    ProductKernel(ConvolutionKernel a, ConvolutionKernel b)
      : ConvolutionKernel(new Implementation(a, b)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(ConvolutionKernel a, ConvolutionKernel b)
          : _a(a), _b(b) { }
        virtual double operator()(double x) const { return _a(x)*_b(x); }
        virtual double leftExtent() const
        {
            return max(_a.leftExtent(), _b.leftExtent());
        }
        virtual double rightExtent() const
        {
            return min(_a.rightExtent(), _b.rightExtent());
        }
    private:
        ConvolutionKernel _a;
        ConvolutionKernel _b;
    };
};

class SumKernel : public ConvolutionKernel
{
public:
    SumKernel(ConvolutionKernel a, ConvolutionKernel b)
      : ConvolutionKernel(new Implementation(a, b)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(ConvolutionKernel a, ConvolutionKernel b)
          : _a(a), _b(b) { }
        virtual double operator()(double x) const { return _a(x)+_b(x); }
        virtual double leftExtent() const
        {
            return min(_a.leftExtent(), _b.leftExtent());
        }
        virtual double rightExtent() const
        {
            return max(_a.rightExtent(), _b.rightExtent());
        }
    private:
        ConvolutionKernel _a;
        ConvolutionKernel _b;
    };
};

class ConstantKernel : public ConvolutionKernel
{
public:
    ConstantKernel(double c) : ConvolutionKernel(new Implementation(c)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(double c) : _c(c) { }
        virtual double operator()(double) const { return c; }
    private:
        double _c;
    };
};

// A convolution pipe which computes kernel coefficients as they are needed,
// which is likely to be very slow. The kernel is measured in output samples
// which is the appropriate default for downsampling. For upsampling, scale
// the
template<class T, class Rate = int> class SmallConvolutionPipe
  : public Pipe<T, T, SmallConvolutionPipe<T, Rate> >
{
public:
    SmallConvolutionPipe(Rate producerRate, Rate consumerRate,
        ConvolutionKernel kernel, int n = defaultSampleCount)
      : Pipe(this, n),
        _kernel(kernel),
        _t(0),
        _r(static_cast<double>(producerRate)/consumerRate),
        _le(kernel.leftExtent()),
        _re(kernel.rightExtent()),
        _maxRead((n + (_re - _le))/_r)
    { }
    void produce(int n)
    {
        Accessor<T> reader = _sink.reader(_maxRead);
        Accessor<T> writer = _source.writer(n);
        int read = 0;
        for (int i = 0; i < n; ++i) {
            T sample = 0;
            int j = read;
            for (double k = _t + _le; k < _re; k += _r) {
                sample += reader.item(j)*_kernel(k);
                ++j;
            }
            writer.item() = sample;

            int r = static_cast<int>((_r + 1.0 - _t)/_r);
            _t += r*_r;
            read += r;
        }
        _sink.read(read);
        _source.written(n);
    }
private:
    ConvolutionKernel _kernel;
    double _t;
    double _r;
    int _maxRead;  // Maximum number of samples that are read at once
    double _le;
    double _re;
};

// A convolution pipe that stores the kernel coefficients it requires. This
// may use a lot of memory if the producing and consuming rates have a high
// lowest common multiple, and it can't be used if the resampling factor
// changes.
template<class T, class Rate = int> class LCMConvolutionPipe
  : public Pipe<T, T, LCMConvolutionPipe<T, Rate> >
{
public:
    // For every "consumerRate" samples consumed we will produce "producerRate"
    // samples.
    LCMConvolutionPipe(Rate producerRate, Rate consumerRate,
        ConvolutionKernel kernel, int n = defaultSampleCount)
      : Pipe(this, n),
        _t(0)
    {
        Rate l = lcm(producerRate, consumerRate);  // 275625000
        int c = l/producerRate;  // 6250
        _delta = l/consumerRate;  // 231
        double le = kernel.leftExtent();
        double re = kernel.rightExtent();
        double extent = re-le;  // 10
        _maxRead = (n + extent)/_r;
        _kernelSize = extent*c;
        // TODO: rearrange kernel coefficients so that they are adjacent in
        // memory for better cache performance
        _kernel.allocate(_kernelSize);
        for (int i = 0; i < _kernelSize; ++i)
            _kernel[i] = kernel(static_cast<double>(i)/c + le);
    }
    void produce(int n)
    {
        Accessor<T> reader = _sink.reader(_maxRead);
        Accessor<T> writer = _source.writer(n);
        int read = 0;
        for (int i = 0; i < n; ++i) {
            T sample = 0;
            int j = read;
            for (int k = _t; k < _kernelSize; k += _delta) {
                sample += reader.item(j)*_kernel[k];
                ++j;
            }
            writer.item() = sample;
            int r = static_cast<int>((c + _delta - _t)/_delta);
            _t += r*_delta;
            read += r;
        }
        _sink.read(read);
        _source.written(n);
    }
private:
    // _kernel is indexed in units of 1/c of an output sample and offset by le
    // output samples
    Array<T> _kernel;
    int _kernelSize;
    int _t;
    int _delta;
    int _t;
};

// A convolution pipe that computes a fixed number of kernel coefficients, and
// uses the closest one to the one required.
template<class T, class Rate = int> class NearestNeighborConvolutionPipe
  : public Pipe<T, T, NearestNeighborConvolutionPipe<T, Rate> >
{
    // TODO
};

// A convolution pipe that computes a fixed number of kernel coefficients, and
// uses linear interpolation to find the others.
template<class T, class Rate = int> class LinearConvolutionPipe
  : public Pipe<T, T, LinearConvolutionPipe<T, Rate> >
{
};


// 985428/44100 == 492714/22050 == 246357/11025 == 82119/3675 == 27373/1225

#endif // INCLUDED_CONVOLUTION_PIPE_H
