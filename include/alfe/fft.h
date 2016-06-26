#include "alfe/main.h"
#include "alfe/complex.h"

#ifndef INCLUDED_FFT_H
#define INCLUDED_FFT_H

#include "fftw3.h"

template<class T> struct FFTW;

template<> struct FFTW<float>
{
    typedef float Real;
    typedef fftwf_complex Complex;
    typedef fftwf_plan Plan;

    static void execute(Plan p) { fftwf_execute(p); }
    static Plan plan_dft_r2c_1d(int n, Real* in, Complex* out, unsigned flags)
    {
        return fftwf_plan_dft_r2c_1d(n, in, out, flags);
    }
    static Plan plan_dft_c2r_1d(int n, Complex* in, Real* out, unsigned flags)
    {
        return fftwf_plan_dft_c2r_1d(n, in, out, flags);
    }
    static void execute_dft_r2c(Plan p, Real* in, Complex* out)
    {
        fftwf_execute_dft_r2c(p, in, out);
    }
    static void execute_dft_c2r(Plan p, Complex* in, Real* out)
    {
        fftwf_execute_dft_c2r(p, in, out);
    }
    static void destroy_plan(Plan p) { fftwf_destroy_plan(p); }
    static void cleanup(void) { fftwf_cleanup(); }
    static int export_wisdom_to_filename(const char *filename)
    {
        return fftwf_export_wisdom_to_filename(filename);
    }
    static int import_wisdom_from_filename(const char *filename)
    {
        return fftwf_import_wisdom_from_filename(filename);
    }
    static void print_plan(Plan p) { fftwf_print_plan(p); }
    static float* alloc_real(size_t n) { return fftwf_alloc_real(n); }
    static Complex* alloc_complex(size_t n) { return fftwf_alloc_complex(n); }
    static void free(void* p) { fftwf_free(p); }
    static void flops(Plan p, double* add, double* mul, double* fmas)
    {
        return fftwf_flops(p, add, mul, fmas);
    }
};

template<class T> class FFTWArray : public ConstHandle
{
protected:
    FFTWArray() { }
    FFTWArray(const ConstHandle other) : ConstHandle(other) { }
    struct Body : public ConstHandle::Body
    {
        Body(void* data, int n) : _data(data), _n(n) { }
        ~Body() { FFTW<T>::free(_data); }
        void* _data;
        int _n;
    };

    void* data() const { return body()->_data; }
    int count() const { return body() == 0 ? 0 : body()->_n; }
private:
    const Body* body() const { return as<Body>(); }
};

template<class T> class FFTWRealArray : public FFTWArray<T>
{
public:
    FFTWRealArray() { }
    FFTWRealArray(int n)
      : FFTWArray<T>(create<Body>(FFTW<T>::alloc_real(n), n)) { }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    void ensure(int n) { if (count() < n) *this = FFTWRealArray(n); }
private:
    T* data() const { return reinterpret_cast<T*>(FFTWArray<T>::data()); }
};

template<class T> class FFTWComplexArray : public FFTWArray<T>
{
public:
    FFTWComplexArray() { }
    FFTWComplexArray(int n)
      : FFTWArray<T>(create<Body>(FFTW<T>::alloc_complex(n), n)) { }
    typename Complex<T>& operator[](int i)
    {
        return reinterpret_cast<Complex<T>*>(data())[i];
    }
    const typename Complex<T>& operator[](int i) const
    {
        return reinterpret_cast<Complex<T>*>(data())[i];
    }
    void ensure(int n) { if (count() < n) *this = FFTWComplexArray(n); }
    typename FFTW<T>::Complex* data() const
    {
        return reinterpret_cast<FFTW<T>::Complex*>(FFTWArray<T>::data());
    }
};

template<class T> class FFTWPlan : public ConstHandle
{
public:
    FFTWPlan() { }
    void execute() { FFTW<T>::execute(plan()); }
    void print()
    {
        FFTW<T>::print_plan(plan());
        double add,mul,fmas;
        FFTW<T>::flops(plan(), &add, &mul, &fmas);
        printf("\n%lf adds, %lf muls, %lf fmas\n", add, mul, fmas);
    }
protected:
    FFTWPlan(typename FFTW<T>::Plan plan) : ConstHandle(create<Body>(plan)) { }
    typename FFTW<T>::Plan plan() { return as<Body>()->_plan; }
private:
    struct Body : public ConstHandle::Body
    {
        Body(typename FFTW<T>::Plan plan) : _plan(plan) { }
        ~Body() { FFTW<T>::destroy_plan(_plan); }
        typename FFTW<T>::Plan _plan;
    };
};

template<class T> class FFTWPlanDFTR2C1D : public FFTWPlan<T>
{
public:
    FFTWPlanDFTR2C1D() { }
    FFTWPlanDFTR2C1D(int n, int rigor)
      : FFTWPlanDFTR2C1D(n, FFTWRealArray(n), FFTWComplexArray(n/2 + 1), rigor)
    { }
    FFTWPlanDFTR2C1D(int n, FFTWRealArray<T> in, FFTWComplexArray<T> out,
        int rigor)
      : FFTWPlan(FFTW<T>::plan_dft_r2c_1d(n, &in[0], out.data(), rigor)) { }
    void execute() { FFTWPlan<T>::execute(); }
    void execute(FFTWRealArray<T> in, FFTWComplexArray<T> out)
    {
        FFTW<T>::execute_dft_r2c(plan(), &in[0], out.data());
    }
};

template<class T> class FFTWPlanDFTC2R1D : public FFTWPlan<T>
{
public:
    FFTWPlanDFTC2R1D() { }
    FFTWPlanDFTC2R1D(int n, int rigor)
      : FFTWPlanDFTC2R1D(n, FFTWComplexArray(n/2 + 1), FFTWRealArray(n), rigor)
    { }
    FFTWPlanDFTC2R1D(int n, FFTWComplexArray<T> in, FFTWRealArray<T> out,
        int rigor)
      : FFTWPlan(FFTW<T>::plan_dft_c2r_1d(n, in.data(), &out[0], rigor)) { }
    void execute() { FFTWPlan<T>::execute(); }
    void execute(FFTWComplexArray<T> in, FFTWRealArray<T> out)
    {
        FFTW<T>::execute_dft_c2r(plan(), in.data(), &out[0]);
    }
};

template<class T> class FFTWWisdom
{
public:
    FFTWWisdom(File wisdom) : _wisdom(wisdom)
    {
        NullTerminatedString data(_wisdom.path());
        FFTW<T>::import_wisdom_from_filename(data);
    }
    ~FFTWWisdom()
    {
        NullTerminatedString data(_wisdom.path());
        FFTW<T>::export_wisdom_to_filename(data);
        FFTW<T>::cleanup();
    }
private:
    File _wisdom;
};

#endif // INCLUDED_FFT_H
