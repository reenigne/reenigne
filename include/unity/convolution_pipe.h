#ifndef INCLUDED_CONVOLUTION_PIPE_H
#define INCLUDED_CONVOLUTION_PIPE_H

#include "unity/filters.h"
#include "unity/lcm.h"
#include "unity/reference_counted.h"
#include "float.h"

class ConvolutionKernel
{
public:
    ConvolutionKernel(Implementation* implementation) : _implementation(implementation) { }
    double operator()(double x) const { return (*_implementation)(x); }
    double leftExtent() const { return _implementation->leftExtent(); }
    double rightExtent() const { return _implementation->rightExtent(); }
    ConvolutionKernel& operator*=(const ConvolutionKernel& right) { _implementation = new ProductKernel(*this, right); }
    ConvolutionKernel& operator+=(const ConvolutionKernel& right) { _implementation = new SumKernel(*this, right); }
    ConvolutionKernel& operator-=(const ConvolutionKernel& right) { _implementation = new SumKernel(*this, -right); }
    ConvolutionKernel operator-() const { return new NegatedKernel(*this); }
    ConvolutionKernel operator*(const ConvolutionKernel& right) { ConvolutionKernel k = *this; k *= right; return k; }
    ConvolutionKernel operator+(const ConvolutionKernel& right) { ConvolutionKernel k = *this; k += right; return k; }
    ConvolutionKernel operator-(const ConvolutionKernel& right) { ConvolutionKernel k = *this; k -= right; return k; }

    class Implementation : public ReferenceCounted
    {
    public:
        virtual double operator()(double x) const = 0;
        virtual double leftExtent() const { return -DBL_MAX; }
        virtual double rightExtent() const { return DBL_MAX; }
    };
private:
    Reference<Implementation> _implementation;
};

class SincFilter : public ConvolutionKernel
{
public:
    SincFilter() : ConvolutionKernel(new Implementation) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
        virtual double operator()(double offset) const { return sin(x)/x; }
    };
};

class RectangleWindow : public ConvolutionKernel
{
public:
    RectangleWindow(double semiWidth) : ConvolutionKernel(new Implementation(semiWidth)) { }
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
    ScaledFilter(ConvolutionKernel kernel, double scale) : ConvolutionKernel(new Implementation(kernel, scale)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(ConvolutionKernel kernel, double scale) : _kernel(kernel), _scale(scale) { }
        virtual double operator()(double x) const { return _kernel(x / _scale); }
        virtual double leftExtent() const { return _kernel.leftExtent() * _scale; }
        virtual double rightExtent() const { return _kernel.rightExtent() * _scale; }
    private:
        ConvolutionKernel _kernel;
        double _scale;
    };
};

class ProductKernel : public ConvolutionKernel
{
public:
    ProductKernel(ConvolutionKernel a, ConvolutionKernel b) : ConvolutionKernel(new Implementation(a, b)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(ConvolutionKernel a, ConvolutionKernel b) : _a(a), _b(b) { }
        virtual double operator()(double x) const { return _a(x)*_b(x); }
        virtual double leftExtent() const { return max(_a.leftExtent(), _b.leftExtent()); }
        virtual double rightExtent() const { return min(_a.rightExtent(), _b.rightExtent()); }
    private:
        ConvolutionKernel _a;
        ConvolutionKernel _b;
    };
};

class SumKernel : public ConvolutionKernel
{
public:
    SumKernel(ConvolutionKernel a, ConvolutionKernel b) : ConvolutionKernel(new Implementation(a, b)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(ConvolutionKernel a, ConvolutionKernel b) : _a(a), _b(b) { }
        virtual double operator()(double x) const { return _a(x)+_b(x); }
        virtual double leftExtent() const { return min(_a.leftExtent(), _b.leftExtent()); }
        virtual double rightExtent() const { return max(_a.rightExtent(), _b.rightExtent()); }
    private:
        ConvolutionKernel _a;
        ConvolutionKernel _b;
    };
};

class NegatedKernel : public ConvolutionKernel
{
public:
    NegatedKernel(ConvolutionKernel a) : ConvolutionKernel(new Implementation(a, b)) { }
    class Implementation : public ConvolutionKernel::Implementation
    {
    public:
        Implementation(ConvolutionKernel a) : _a(a) { }
        virtual double operator()(double x) const { return -_a(x); }
        virtual double leftExtent() const { return _a.leftExtent(); }
        virtual double rightExtent() const { return _a.rightExtent(); }
    private:
        ConvolutionKernel _a;
    };
};

// A pipe that produces output by convolving the input.
template<class T, class Rate = int> class ConvolutionPipe : public Pipe<T, T>
{
public:
    // For every "consumerRate" samples consumed we will produce "producerRate" samples.
    BandLimitedInterpolator(Rate producerRate, Rate consumerRate, ConvolutionKernel kernel, int n = defaultFilterCount, Producer<T>* producer = 0)
      : Pipe(n, producer),
        _offset(0)
    {
        Rate l = lcm(producerRate, consumerRate);  // 275625000
        int c = l/producerRate;  // 6250
        int p = l/consumerRate;  // 231
        double extent = kernel.rightExtent() - kernel.leftExtent();  // 10



        // TODO: initialize a, b, f, _offset and _delta
        _kernelSize = (_kernel.rightExtent() - _kernel.leftExtent())*f;
        _kernel.allocate(_kernelSize);
        for (int i = 0; i < _kernelSize; ++i)
            _kernel[i] = kernel(i*a+b);
    }
    void process()
    {
        Buffer<T> reader = _consumer.reader(_n);
        Buffer<T> writer = _producer.writer(static_cast<int>((static_cast<Rate>(_n)*_producerRate)/_consumerRate) + 1);
        int read = 0;
        for (int i = 0; i < _n; ++i) {
            T sample = 0;
            int j = 0;
            for (int k = _offset; k < _kernelSize; k += _delta) {
                sample += reader.item(j)*_kernel[_offset];
                ++j;
            }
            writer.item() = sample;
        }
        // TODO: Figure out "read" and "_offset" in terms of "_producerRate" and "_consumerRate"
        _consumer.read(read);
        _producer.written(_n);
    }
private:
    Array<T> _kernel;
    int _kernelSize;
    int _offset;
    int _delta;
};

#endif // INCLUDED_CONVOLUTION_PIPE_H
