// Fixed-point arbitrary-precision arithmetic classes for fractal generation
// purposes.
//
// The least significant words are stored at lower addresses.

#define _USE_MATH_DEFINES
#include <math.h>

typedef unsigned int Digit;
typedef int SignedDigit;
static const int logBitsPerDigit = 5; // 5 for 32-bit Digit, 6 for 64-bit Digit
static const int intBits = 12;  // Matches double precision at -2.
static const int bitsPerDigit = 1 << logBitsPerDigit;
static const Digit minusOne = static_cast<Digit>(-1);
static const int bitsPerHalfDigit = 1 << (logBitsPerDigit - 1);
static const Digit lowBits = (1 << bitsPerHalfDigit) - 1;

// r = v. Aliasing ok.
void copy(Digit* r, const Digit* v, int n)
{
    while (n-- > 0)
        *(r++) = *(v++);
}

// r == 0
bool isZero(const Digit* v, int n)
{
    while (n-- > 0)
        if (*(v++) != 0)
            return false;
    return true;
}

// v < 0
bool lessThanZero(const Digit* v, int n)
{
    return (v[n - 1] & (1 << (bitsPerDigit - 1))) != 0;
}

// a < b. Aliasing ok.
bool lessThan(const Digit* a, const Digit* b, int n)
{
    --n;
    SignedDigit aHigh = static_cast<SignedDigit>(a[n]);
    SignedDigit bHigh = static_cast<SignedDigit>(b[n]);
    if (aHigh < bHigh)
        return true;
    if (aHigh > bHigh)
        return false;
    while (n-- > 0) {
        if (a[n] < b[n])
            return true;
        if (a[n] > b[n])
            return false;
    }
    return false;
}

// r = v with precision change. Aliasing ok.
void copy(Digit* r, int rn, const Digit* v, int vn)
{
    if (rn > vn) {
        r += rn;
        v += vn;
        while (vn-- > 0) {
            *(--r) = *(--v);
            --rn;
        }
        while (rn-- > 0)
            *(--r) = 0;
    }
    else {
        v += vn - rn;
        while (rn-- > 0)
            *(r++) = *(v++);
    }
}

// r = 0
void zero(Digit* r, int n)
{
    while (n-- > 0)
        *(r++) = 0;
}

// Equivalent of x86 ADC instruction (carry:r = a + b + carry)
void adc(Digit& r, Digit a, Digit b, Digit& carry)
{
    Digit s = a + b;
    Digit carry1 = (s < a ? 1 : 0);
    r = s + carry;
    carry = carry1 | (r < s ? 1 : 0);
}

// r = a + b. Aliasing ok.
void add(Digit* r, const Digit* a, const Digit* b, int n)
{
    Digit c = 0;
    while (n-- > 0)
        adc(*(r++), *(a++), *(b++), c);
}

void print(const Digit* v, int n)
{
    for (int i = n - 1; i >= 0; --i)
        printf("%08x ", v[i]);
}

//void add(Digit* r, const Digit* a, const Digit* b, int n)
//{
////    print(a, n); printf("+ "); print(b, n); printf("= ");
//    __asm {
//        pxor mm0, mm0
//        mov eax, a
//        mov ebx, b
//        mov edx, r
//        mov ecx, n
//        lea eax, [eax+ecx*4]
//        lea ebx, [ebx+ecx*4]
//        lea edx, [edx+ecx*4]
//        neg ecx
//looptop:
//        movd mm1, [eax+ecx*4]
//        movd mm2, [ebx+ecx*4]
//        paddq mm1, mm2
//        paddq mm0, mm1
//        movd [edx+ecx*4], mm0
//        psrlq mm0, 32
//        add ecx, 1
//        jnz looptop
//        emms
//    }
////    print(r, n); printf("\n");
//}


// Equivalent of x86 SBB instruction (carry:r = a - (b + carry))
void sbb(Digit& r, Digit a, Digit b, Digit& carry)
{
    Digit s = a - b;
    Digit carry1 = (s > a ? 1 : 0);
    r = s - carry;
    carry = carry1 | (r > s ? 1 : 0);
}

// r = a - b. Aliasing ok.
void sub(Digit* r, const Digit* a, const Digit* b, int n)
{
    Digit c = 0;
    while (n-- > 0)
        sbb(*(r++), *(a++), *(b++), c);
}

//void sub(Digit* r, const Digit* a, const Digit* b, int n)
//{
////    print(a, n); printf("+ "); print(b, n); printf("= ");
//    __asm {
//        pxor mm0, mm0
//        mov eax, a
//        mov ebx, b
//        mov edx, r
//        mov ecx, n
//        lea eax, [eax+ecx*4]
//        lea ebx, [ebx+ecx*4]
//        lea edx, [edx+ecx*4]
//        neg ecx
//looptop:
//        movd mm1, [eax+ecx*4]
//        movd mm2, [ebx+ecx*4]
//        psubq mm1, mm2
//        psubq mm1, mm0
//        movd [edx+ecx*4], mm1
//        psrlq mm1, 63
//        add ecx, 1
//        jnz looptop
//        emms
//    }
////    print(r, n); printf("\n");
//}

//void print(double v)
//{
//    if (v < 0) {
//        printf("-");
//        v = -v;
//    }
//    v /= 1024/16;
//    for (int i = 0; i < 16; ++i) {
//        int vv = (int)v;
//        printf("%01x", vv);
//        v -= vv;
//        v *= 16;
//    }
//    printf(" ");
//}
//
// r = v*2^exponent.
void fixedFromDouble(Digit* r, double v, int exponent, int n)
{
    int ve;
    v = frexp(v, &ve);
    exponent += ve + bitsPerDigit - intBits;
    int i = n - 1;
    for (; i >= 0 && exponent < 0; --i) {
        r[i] = v < 0 ? minusOne : 0;
        exponent += bitsPerDigit;
    }
    if (i < 0)
        return;
    v = ldexp(v, exponent);
    SignedDigit vv = static_cast<SignedDigit>(v);
    if (static_cast<double>(vv) > v)
        --vv;  // Round towards -infinity
    r[i--] = static_cast<Digit>(vv);
    v -= vv;
    for (; i >= 0; --i) {
        v = ldexp(v, bitsPerDigit);
        Digit vv = static_cast<Digit>(v);
        r[i] = vv;
        v -= vv;
    }
}

// Returns v*2^-exponent.
double doubleFromFixed(Digit* v, int exponent, int n)
{
    exponent = intBits - bitsPerDigit - exponent;
    Digit sign = lessThanZero(v, n) ? minusOne : 0;

    int i = n - 1;
    for (; i >= 1; --i) {
        if (v[i] != sign)
            break;
        exponent -= bitsPerDigit;
    }
    if (i < n - 1) {
        ++i;
        exponent += bitsPerDigit;
    }
    SignedDigit vv = static_cast<SignedDigit>(v[i]);
    double r = ldexp(static_cast<double>(vv), exponent);
    exponent -= bitsPerDigit;
    --i;
    for (; i >= 0; --i) {
        r += ldexp(static_cast<double>(v[i]), exponent);
        exponent -= bitsPerDigit;
    }
    return r;
}

// r = v << shift. Aliasing ok.
void shiftLeft(Digit* r, const Digit* v, int n, int shift)
{
    //Digit* r0 = r;
    //if (n >= 3) {
    //    print(v, n); printf("<<%i = ",shift);
    //}
    int shiftBits = shift & (bitsPerDigit - 1);
    int shiftDigits = shift >> logBitsPerDigit;
    int rightShiftBits = bitsPerDigit - shiftBits;
    r += n;
    int nn = n - shiftDigits;
    v += nn;
    Digit vv = *(--v);
    while (--nn > 0) {
        Digit vvv = vv << shiftBits;
        vv = *(--v);
        *(--r) = vvv | (vv >> rightShiftBits);
    }
    *(--r) = vv << shiftBits;
    while (shiftDigits-- > 0)
        *(--r) = 0;
    //if (r != r0)
    //    printf("Bad shift!\n");
    //if (n >= 3) {
    //    print(r0, n);
    //    printf("\n");
    //}
}

// r = -v. Aliasing ok.
void negate(Digit* r, const Digit* v, int n)
{
//    printf("-"); print(v, n); printf("= "); int on = n;
    Digit c = 1;
    while (n-- > 0)
        adc(*(r++), ~*(v++), 0, c);
//    print(r - on, on); printf("\n");
}

// r = abs(v). Aliasing ok.
void abs(Digit* r, const Digit* v, int n)
{
    if (lessThanZero(v, n))
        negate(r, v, n);
    else
        copy(r, v, n);
}

//// Equivalent of x86 MUL instruction (hr:lr = a*b)
//void mul(Digit& lr, Digit& hr, Digit a, Digit b)
//{
//    Digit ah = a >> bitsPerHalfDigit;
//    Digit al = a & lowBits;
//    Digit bh = b >> bitsPerHalfDigit;
//    Digit bl = b & lowBits;
//    lr = al * bl;
//    Digit m = ah * bl + (lr >> bitsPerHalfDigit); // Can't carry
//    Digit m2 = al * bh;
//    hr = ah * bh;
//    m += m2;
//    if (m < m2) // Carry?
//      hr += lowBits + 1;
//    hr += m >> bitsPerHalfDigit;
//    lr &= lowBits;
//    lr += m << bitsPerHalfDigit;
//}

void mul(Digit& lr, Digit& hr, Digit a, Digit b)
{
    Digit l, h;
    __asm {
        mov eax, a
        mul b
        mov h, edx
        mov l, eax
    }
    hr = h;
    lr = l;
}

// Helper function for multiply: r += a * b. Aliasing forbidden. an is the
// number of digits in a and rn is the number of padding digits to add on.
void multiply(Digit* r, int rn, const Digit* a, int an, Digit b)
{
    //print(r, rn + an);
    //printf("+ ");
    //print(a, an);
    //printf("* %08x = ",b);
    //Digit* r0 = r;

    Digit high, low, carry = 0;
    for (int i = 0; i < an; ++i) {
        mul(low, high, *(a++), b);
        low += carry;
        carry = high + (low < carry ? 1 : 0);
        Digit rd = *r;
        low += rd;
        carry += (low < rd ? 1 : 0);
        *(r++) = low;
    }
    for (int i = 0; i < rn && carry != 0; ++i) {
        Digit rd = *r;
        low = carry + rd;
        carry = (low < rd ? 1 : 0);
        *(r++) = low;
    }

    //print(r0, rn + an);
    //printf("\n");
}

// r = a * b. Aliasing ok. 
// scratch must point to at least n*4 Digits. This buffer cannot be shared
// across threads.
void multiply(Digit* r, const Digit* a, const Digit* b, Digit* scratch, int n,
    int integerBits = intBits)
{
    int n2 = n*2;

//    printf("%i ",integerBits); print(a, n); printf("* "); print(b, n); printf("= ");

    Digit* av = scratch + n2;
    Digit* bv = av + n;
    bool aNegative = false;
    bool bNegative = false;
    if (lessThanZero(a, n)) {
        aNegative = true;
        negate(av, a, n);
//        printf("-");
    }
    else
        copy(av, a, n);
    if (lessThanZero(b, n)) {
        bNegative = true;
        negate(bv, b, n);
//        printf("-");
    }
    else
        copy(bv, b, n);
//    print(av, n); printf("* "); print(bv, n); printf("= ");

    // 1) Implement full n*n->2n integer multiplication
    zero(scratch, n2);
    for (int i = 0; i < n; ++i)
        multiply(scratch + i, n - i, av, n, bv[i]);
//    print(scratch, 2*n); printf("= ");

    // 2) Correct for signs
    // Now, scratch = a * b but what we wanted was (a - a')*(b - b')
    // where a' is lessThanZero(a, n) ? radix^n : 0
    //       b' is lessThanZero(b, n) ? radix^n : 0
    // a'*b' is either 0 or radix^(2n) which truncates to 0 anyway, so we 
    // can just subtract a*radix^n and/or b*radix^n as necessary
    //if (lessThanZero(a, n))
    //    sub(scratch + n, scratch + n, b, n);
    //if (lessThanZero(b, n))
    //    sub(scratch + n, scratch + n, a, n);
    //print(scratch, n*2); printf(") ");

    // 3) Shift the result for radix point correction
    int shift = (n << logBitsPerDigit) - integerBits;  // == fractionalBits
    Digit* v = scratch;
    int shiftBits = shift & (bitsPerDigit - 1);
    int shiftDigits = shift >> logBitsPerDigit;
    int leftShiftBits = bitsPerDigit - shiftBits;
    v += shiftDigits;
    Digit vv = *(v++);
    Digit carry = ((vv & (1 << (shiftBits - 1))) != 0 ? 1 : 0);
    Digit* rr = r;
    for (int i = 0; i < n; ++i) {
        Digit vvv = vv >> shiftBits;
        vv = *(v++);
        *(rr++) = vvv | (vv << leftShiftBits);
    }

//    print(r - n, n); printf("= ");

    rr = r;
    int nn = n;
    while (nn-- > 0 && carry != 0) {
        adc(*rr, *rr, 0, carry);
        ++rr;
    }
//    print(r - n, n); printf("= ");

    if (aNegative != bNegative)
        negate(r, r, n);

//    print(r - n, n);
//    printf("\n");

    // 4) Avoid unnecessary multiplications
    //    How many digits of scratch can we avoid computing?
    //    
    // 5) optimize
}

// This class encapsulates an sequence of digits.
class DigitBuffer
{
public:
    DigitBuffer() : _n(0) { }

    void ensureLength(int n)
    {
        if (n > _n) {
            Digit* newDigits = new Digit[n];
            for (int i = 0; i < _n; ++i)
                newDigits[i] = _digits[i];
            reset();
            _digits = newDigits;
            _n = n;
        }
    }

    ~DigitBuffer() { reset(); }

    operator Digit*() { return _digits; }
    operator const Digit*() const { return _digits; }

    int n() const { return _n; }
private:
    void reset()
    {
        if (_n > 0)
            delete[] _digits;
    }

    int _n;
    Digit* _digits;
};


//
//// r = v >> shift. Aliasing ok.
//void shiftRight(Digit* r, const Digit* v, int n, int shift)
//{
//    // Increasing word order
//    int shiftBits = shift & (bitsPerDigit - 1);
//    int shiftDigits = shift >> logBitsPerDigit;
//    int leftShiftBits = bitsPerDigit - shiftBits;
//    v += shiftDigits;
//    int nn = n - (shiftDigits + 1);
//    Digit vv = *v;
//    while (--nn) {
//        Digit vvv = vv >> shiftBits;
//        vv = *(v++);
//        *(r++) = vvv | (vv << leftShiftBits);
//    }
//    Digit sign = lessThanZero(v, n) ? minusOne : 0;
//    *(r++) = vv | (sign << leftShiftBits);
//    while (--shiftWords > 0)
//        *(r++) = sign;
//}
//
//// r = ~v. Aliasing ok.
//void bitwiseNot(Digit* r, const Digit* v, int n)
//{
//    while (n-- > 0)
//        *(r++) = ~*(v++);
//}
//
//// r = a & b. Aliasing ok.
//void bitwiseAnd(Digit* r, const Digit* a, const Digit* b, int n)
//{
//    while (n-- > 0)
//        *(r++) = *(a++) & *(b++);
//}
//
//// r = a | b. Aliasing ok.
//void bitwiseOr(Digit* r, const Digit* a, const Digit* b, int n)
//{
//    while (n-- > 0)
//        *(r++) = *(a++) | *(b++);
//}
//
//// r = a ^ b. Aliasing ok.
//void bitwiseXor(Digit* r, const Digit* a, const Digit* b, int n)
//{
//    while (n-- > 0) {
//        *(r++) = *(a++) ^ *(b++);
//}
//
//// a == b. Aliasing ok.
//bool equalTo(const Digit* a, const Digit* b, int n)
//{
//    while (n-- > 0)
//        if (*(a++) != *(b++))
//            return false;
//    return true;
//}
//
//// Can we represent r with fewer digits of precision?
//bool overPrecise(const Digit* v)
//{
//    return *v == 0;
//}
//
//// r = 2^exponent
//void powerOfTwo(Digit* r, int n, int integerBits, int exponent)
//{
//    zero(r, n);
//    int bit = exponent + (n << logBitsPerDigit) - integerBits;
//    r[bit >> logBitsPerWord] = 1 << (bit & (bitsPerWord - 1));
//}
//
//// r = v*v
//void square(Digit* r, const Digit* v, int n)
//{
//    // TODO. See if we can implement this more efficiently than just
//    // multiplying v with itself, since this is important for the Mandelbrot
//    // set. We should only need n*(n + 1)/2 multiplications rather than the
//    // usual n*n.
//}
//
//// Equivalent of x86 DIV instruction (r = ha:la/b)
//void div(Digit& r, Digit ha, Digit la, Digit b)
//{
//    // TODO: consult Knuth
//}
//
//// r = v
//void fromInteger(Digit* r, int v, int n, int integerBits)
//{
//    int fracBits = (n << logBitsPerDigit) - integerBits;
//    int shiftBits = fracBits & (bitsPerDigit - 1);
//    int shiftWords = fracBits >> logBitsPerDigit;
//
//    while (--shiftWords > 0) {
//        *(r++) = 0;
//        --n;
//    }
//    *(r++) = reinterpret_cast<Digit>(v << shiftBits);
//    --n;
//    if (shiftBits > 0 && n > 0) {
//        *(r++) = reinterpret_cast<Digit>(v >> (bitsPerDigit - shiftBits));
//        --n;
//    }
//    Digit sign = (v < 0 ? minusOne : 0);
//    while (--n > 0)
//        *(r++) = sign;
//}
//
//// r = a/b
//void divide(Digit* r, Digit* a, Digit* b, int n, int unitBits)
//{
//    // Get divide algorithm from Knuth
//    ?
//}
//
//// square
//// multiply by word
//// divide by word
//// exp
//// sin
//// cos
//// log
//// atan
//// sinh
//// cosh
//// acos
//// asin
//// atan2
//// pow
//// sqrt
