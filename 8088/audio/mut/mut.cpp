#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/type.h"
#include "alfe/rational.h"

template<class T> class SoundObjectTemplate;
typedef SoundObjectTemplate<void> SoundObject;

template<class T> class EntryTemplate;
typedef EntryTemplate<void> Entry;

template<class T> class ScalarTemplate;
typedef ScalarTemplate<void> Scalar;

template<class T> class SampleTemplate;
typedef SampleTemplate<void> Sample;

template<class T> class ImplicitSampleTemplate;
typedef ImplicitSampleTemplate<void> ImplicitSample;

template<class T> class SineSampleTemplate;
typedef SineSampleTemplate<void> SineSample;

template<class T> class TriangleSampleTemplate;
typedef TriangleSampleTemplate<void> TriangleSample;

template<class T> class SquareSampleTemplate;
typedef SquareSampleTemplate<void> SquareSample;

template<class T> class SawtoothSampleTemplate;
typedef SawtoothSampleTemplate<void> SawtoothSample;

template<class T> class NoiseSampleTemplate;
typedef NoiseSampleTemplate<void> NoiseSample;

template<class T> class ExplicitSampleTemplate;
typedef ExplicitSampleTemplate<void> ExplicitSample;

template<class T> class TableTemplate;
typedef TableTemplate<void> Table;

template<class T> class StatementTemplate;
typedef StatementTemplate<void> Statement;

template<class T> class SymbolTableTemplate;
typedef SymbolTableTemplate<void> SymbolTable;

template<class T> class SoundObjectTemplate
{
public:
    SoundObject power(const SoundObject& other) const { return _implementation->power(other._implementation); }
    SoundObject operator-() const { return _implementation->negative(); }
    SoundObject timeScale(const SoundObject& other) const { return _implementation->timeScale(other._implementation); }
    SoundObject operator*(const SoundObject& other) const { return _implementation->product(other._implementation); }
    SoundObject operator/(const SoundObject& other) const { return _implementation->quotient(other._implementation); }
    SoundObject operator+(const SoundObject& other) const { return _implementation->sum(other._implementation); }
    SoundObject operator-(const SoundObject& other) const { return _implementation->difference(other._implementation); }
    SoundObject operator&(const SoundObject& other) const { return _implementation->voice(other._implementation); }
    SoundObject operator|(const SoundObject& other) const { return _implementation->sequence(other._implementation); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual SoundObject power(const Implementation* other) const = 0;
        virtual SoundObject negative() const = 0;
        virtual SoundObject timeScale(const Implementation* other) const = 0;
        virtual SoundObject product(const Implementation* other) const = 0;
        virtual SoundObject quotient(const Implementation* other) const = 0;
        virtual SoundObject sum(const Implementation* other) const = 0;
        virtual SoundObject difference(const Implementation* other) const = 0;
        virtual SoundObject voice(const Implementation* other) const = 0;
        virtual SoundObject sequence(const Implementation* other) const = 0;
    };
    Reference<Implementation> _implementation;
};

template<class T> class EntryTemplate : public SoundObject
{
protected:
    class Implementation : public SoundObject::Implementation
    {
    };
};

template<class T> class ScalarTemplate : public Entry
{
public:
    ScalarTemplate(int value = 0, int unit = 0) : Entry(new RationalImplementation(unit, value)) { }

    bool isReal() { return _implementation->isReal(); }
    int unit() { return _implementation->unit(); }
    double toRealDouble() { return _implementation->toDouble().x; }
protected:
    ScalarTemplate(Implementation* implementation) : Entry(implementation) { }

    class Implementation : public Entry::Implementation
    {
    public:
        Implementation(int unit) : _unit(unit) { }
        bool isNumber() const { return _unit == 0; }
        int unit() const { return _unit; }
        virtual bool isRational() const = 0;
        virtual bool isInteger() const = 0;
        virtual int toInteger() const = 0;
        virtual Complex<double> toDouble() const = 0;
        virtual Complex<Rational<int>> toRational() const = 0;
    protected:
        int _unit;
    };

    class DoubleImplementation : public Implementation
    {
    public:
        DoubleImplementation(int unit = 0, Complex<double> value = 0) : Implementation(unit), _value(value) { }
        bool isRational() const { return false; }
        bool isInteger() const { return _value.y == 0 && isDoubleInteger(_value.x); }
        int toInteger() const { return _value.x; }
        Complex<double> toDouble() const { return _value; }
        Complex<Rational<int>> toRational() const { return Complex<Rational<int>>(rationalFromDouble(_value.x), rationalFromDouble(_value.y)); }

        SoundObject power(const SoundObject::Implementation* o) const
        {
            const Implementation* other = dynamic_cast<const Implementation*>(o);
            if (other == 0)
                throw Exception("Don't know how to do that yet!\n");
            if (other->isRational()) {
                Complex<Rational<int>> p = _unit;
                p *= other->toRational();
                if (p.y.numerator != 0 || p.x.denominator != 1)
                    throw Exception("Operation yields a non-integral time power.\n");
                return new DoubleImplementation(p.x.numerator, pow(_value, other->toDouble()));
            }
            Complex<double> p = _unit;
            p *= other->toDouble();
            if (p.y != 0 || !isDoubleInteger(p.x))
                throw Exception("Operation yields a non-integral time power.\n");
            return new DoubleImplementation(static_cast<int>(p.x), pow(_value, other->toDouble()));
        }
        SoundObject negative() const
        {
            return new DoubleImplementation(_unit, -_value);
        }
        SoundObject timeScale(const SoundObject::Implementation* o) const
        {
            throw Exception("Don't know how to do that yet!\n");
        }
        SoundObject product(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject quotient(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject sum(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject difference(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject voice(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject sequence(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
    private:
        Complex<double> _value;
    };

    class RationalImplementation : public Implementation
    {
    public:
        RationalImplementation(int unit = 0, Complex<Rational<int>> value = 0) : Implementation(unit), _value(value) { }
        bool isRational() const { return true; }
        bool isInteger() const { return _value.x.denominator == 1 && _value.y.numerator == 0; }
        int toInteger() const { return _value.x.numerator/_value.x.denominator; }
        Complex<double> toDouble() const { return Complex<double>(static_cast<double>(_value.numerator.x)/_value.denominator.x, static_cast<double>(_value.numerator.y)/_value.denominator.y); }
        Complex<double> toRational() const { return _value; }

        SoundObject power(const SoundObject::Implementation* o) const
        {
            throw Exception("Don't know how to do that yet!\n");
        }
        SoundObject negative() const
        {
            return new RationalImplementation(_unit, -_value);
        }
        SoundObject timeScale(const SoundObject::Implementation* o) const
        {
            throw Exception("Don't know how to do that yet!\n");
        }
        SoundObject product(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject quotient(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject sum(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject difference(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject voice(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
        SoundObject sequence(const Implementation* other) const { throw Exception("Don't know how to do that yet!\n");}
    private:
        Complex<Rational<int>> _value;
    };

private:
    bool isDoubleInteger(double x)
    {
        if (!isfinite(x))
            return false;
        return (static_cast<int>(x) == x);
    }
};

template<class T> class SampleTemplate : public Entry
{
protected:
    class Implementation : public Entry::Implementation
    {
    public:
        Implementation(Number extent) : _extent(extent) { }
        virtual void writeOutput(Byte* destination, int count, int samplesRate) const = 0;
    protected:
        Number _extent;
    };
};

template<class T> class ImplicitSampleTemplate : public Sample
{
protected:
    class Implementation : public Sample::Implementation
    {
    };
};

template<class T> class SineSampleTemplate : public ImplicitSample
{
protected:
    class Implementation : public ImplicitSample::Implementation
    {
    public:

    };
};

template<class T> class TriangleSampleTemplate : public ImplicitSample
{
};

template<class T> class SquareSampleTemplate : public ImplicitSample
{
};

template<class T> class SawtoothSampleTemplate : public ImplicitSample
{
};

template<class T> class NoiseSampleTemplate : public ImplicitSample
{
};

class SampleData
{
public:
    SampleData(int count) : _implementation(new Implementation(count)) { }
    Complex<double>* data() { return _implementation->data(); }
private:
    class Implementation : public ReferenceCounted
    {
    public:
        Implementation(int count) : _data(count) { }
        Complex<double>* data() { return &_data[0]; }
    private:
        Array<Complex<double>> _data;  // TODO: Use FFTW type?
    };
    Reference<Implementation> _implementation;
};

template<class T> class ExplicitSampleTemplate : public Sample
{
protected:
    class Implementation : public Sample::Implementation
    {
    public:
        Implementation(Number extent, SampleData data)
          : Sample::Implementation(extent), _data(data) { }
        void writeOutput(Byte* destination, int count, int samplesRate) const
        {
            // TODO
        }
    private:
        SampleData _data;
    };
};

template<class T> class TableTemplate : public SoundObject
{
};

class ParseTreeObject
{
public:
    Span span() const { return _implementation->span(); }
    bool valid() const { return _implementation.valid(); }
    template<class T> bool is() const
    {
        return _implementation.is<T::Implementation>();
    }
    template<class T> const typename T::Implementation* as() const
    {
        return _implementation.referent<T::Implementation>();
    }

    class Implementation : public ReferenceCounted
    {
    public:
        Implementation(const Span& span) : _span(span) { }
        Span span() const { return _span; }
    private:
        Span _span;
    };

protected:
    ParseTreeObject() { }
    ParseTreeObject(const Implementation* implementation)
      : _implementation(implementation) { }

    template<class T> const ParseTreeObject& operator=(const T* implementation)
    {
        _implementation = implementation;
        return *this;
    }
    const ParseTreeObject& operator=(const ParseTreeObject& other)
    {
        _implementation = other._implementation;
        return *this;
    }

    ConstReference<Implementation> _implementation;
};

class ExpressionStatement : public Statement
{
public:
    static ExpressionStatement parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        Expression expression = Expression::parse(&s);
        if (!expression.valid())
            return ExpressionStatement();
        Span span;
        if (!Space::parseCharacter(&s, ';', &span))
            return ExpressionStatement();
        *source = s;
        if (!expression.is<FunctionCallExpression>())
            source->location().throwError("Statement has no effect");
        return ExpressionStatement(expression, expression.span() + span);
    }

    ExpressionStatement(const Expression& expression, const Span& span)
      : Statement(new Implementation(expression, span)) { }
private:
    ExpressionStatement() { }

    class Implementation : public Statement::Implementation
    {
    public:
        Implementation(const Expression& expression, const Span& span)
          : Statement::Implementation(span), _expression(expression) { }
    private:
        Expression _expression;
    };
};

class LValue : public ParseTreeObject
{
public:
    static LValue parse(CharacterSource* source)
    {
    }
private:

};

class AssignmentStatement : public Statement
{
public:
    static AssignmentStatement parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        LValue left = LValue::parse(&s);
        Location operatorLocation = s.location();
        if (!left.valid())
            return AssignmentStatement();
        Span span;

        static const Operator ops[] = {
            OperatorAssignment(), OperatorAddAssignment(),
            OperatorSubtractAssignment(), OperatorMultiplyAssignment(),
            OperatorDivideAssignment(), OperatorTimeScaleAssignment(),
            OperatorVoiceAssignment(), OperatorSequenceAssignment(),
            OperatorPowerAssignment(),
            Operator()};

        int operatorNumber = -1;

        if (Space::parseOperator(&s, "=", &span))
            operatorNumber = 0;
        if (operatorNumber == -1 && Space::parseOperator(&s, "+=", &span))
            operatorNumber = 1;
        if (operatorNumber == -1 && Space::parseOperator(&s, "-=", &span))
            operatorNumber = 2;
        if (operatorNumber == -1 && Space::parseOperator(&s, "*=", &span))
            operatorNumber = 3;
        if (operatorNumber == -1 && Space::parseOperator(&s, "/=", &span))
            operatorNumber = 4;
        if (operatorNumber == -1 && Space::parseOperator(&s, "@=", &span))
            operatorNumber = 5;
        if (operatorNumber == -1 && Space::parseOperator(&s, "&=", &span))
            operatorNumber = 6;
        if (operatorNumber == -1 && Space::parseOperator(&s, "|=", &span))
            operatorNumber = 7;
        if (operatorNumber == -1 && Space::parseOperator(&s, "^=", &span))
            operatorNumber = 8;
        if (operatorNumber == -1)
            return AssignmentStatement();

        *source = s;
        Expression right = Expression::parseOrFail(source);
        Space::assertCharacter(source, ';', &span);

        if (left.isFunction()) {
            if (operatorNumber != 0)
                span.throwError("Can't define a function that way.\n");
            return new FunctionDefinition(left.name(), left.parameters(), right);
        }

        switch (operatorNumber) {
            case 0:
                return AssignmentStatement(left, right, span);
            case 1:
                return AssignmentStatement(left, AddExpression(left, right, span), span);
            case 2:
                return AssignmentStatement(left, SubtractExpression(left, right, span), span);
            case 3:
                return AssignmentStatement(left, MultiplyExpression(left, right, span), span);
            case 4:
                return AssignmentStatement(left, DivideExpression(left, right, span), span);
            case 5:
                return AssignmentStatement(left, TimeScaleExpression(left, right, span), span);
            case 6:
                return AssignmentStatement(left, VoiceExpression(left, right, span), span);
            case 7:
                return AssignmentStatement(left, SequenceExpression(left, right, span), span);
            case 8:
                return AssignmentStatement(left, PowerExpression(left, right, span), span);
        }
    }
};

template<class T> class StatementTemplate : public ParseTreeObject
{
public:
    static Statement parse(CharacterSource* source)
    {
        Statement statement = ExpressionStatement::parse(source);
        if (statement.valid())
            return statement;
        statement = AssignmentStatement::parse(source);
        if (statement.valid())
            return statement;
        return IncludeStatement::parse(source);
    }
    static Statement parseOrFail(CharacterSource* source)
    {
        Statement statement = parse(source);
        if (!statement.valid())
            source->location().throwError("Expected statement");
        return statement;
    }
    StatementTemplate() { }
protected:
    StatementTemplate(const Implementation* implementation)
      : ParseTreeObject(implementation) { }

    class Implementation : public ParseTreeObject::Implementation
    {
    public:
        Implementation(const Span& span)
          : ParseTreeObject::Implementation(span) { }
    };
};

class StatementSequence : public ParseTreeObject
{
public:
    static StatementSequence parse(CharacterSource* source)
    {
        Span span;
        List<Statement> sequence;
        do {
            Statement statement = Statement::parse(source);
            if (!statement.valid())
                break;
            span += statement.span();
            sequence.add(statement);
        } while (true);
        return new Implementation(sequence, span);
    }
private:
    StatementSequence(const Implementation* implementation)
      : ParseTreeObject(implementation) { }

    class Implementation : public ParseTreeObject::Implementation
    {
    public:
        Implementation(const List<Statement>& sequence, const Span& span)
          : ParseTreeObject::Implementation(span), _sequence(sequence) { }
    private:
        List<Statement> _sequence;
    };
};

template<class T> class SymbolTableTemplate
{
public:
    void add(String identifier, Expression expression) { _implementation->add(identifier, expression); }
    Expression lookup(String identifier) { return _implementation->lookup(identifier); }

private:
    class Implementation : public ReferenceCounted
    {
    public:
        void add(String identifier, Expression expression) { _table[identifier] = expression; }
        Expression lookup(String identifier) const
        {
            if (_table.hasKey(identifier))
                return _table[identifier];
            throw Exception("Unknown identifier " + identifier);
        }
        virtual bool isRoot() const = 0;
    private:
        HashTable<String, Expression> _table;
        AppendableArray<SymbolTable> _children;
    };
    class RootImplementation : public Implementation
    {
    public:
        bool isRoot() const { return true; }
    };
    class VoiceImplementation : public Implementation
    {
    public:
        bool isRoot() const { return false; }
    private:
        SymbolTable _parent;
        int _voice;
    };

    Reference<Implementation> _implementation;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] + " <input file name>\n");
            return;
        }
        File file(_arguments[1]);
        String contents = file.contents();
        CharacterSource s(contents, file.path());
        Space::parse(&s);
        SymbolTable symbols;
        symbols.add("s", Scalar(1, 1));
        symbols.add("sine", SineSample());
        symbols.add("square", SquareSample());
        symbols.add("triangle", TriangleSample());
        symbols.add("sawtooth", SawtoothSample());
        symbols.add("noise", NoiseSample());
        symbols.add("output", Table());

        StatementSequence code = StatementSequence::parse(&s);
        CharacterSource s2 = s;
        if (s2.get() != -1)
            s.location().throwError("Expected end of file");

        code.evaluate();

        Type soundObjectType = AtomicType("SoundObject");
        Type sampleType = AtomicType("Sample");
        Type scalarType = AtomicType("Scalar");
        Type tableType = AtomicType("Table");

        String outputWave = symbols.lookup<String>("outputWave", Type::string);
        int outputWaveRate = symbols.lookupReal<int>("outputWaveRate", -1);
//        if (!outputWaveRate.isReal() || outputWaveRate.unit() != -1)
//            throw Exception("outputWaveRate must be real and have units of Hz.\n");
//        int rate = static_cast<int>(outputWaveRate.toDouble());
        int outputWaveBits = symbols.lookupReal<int>("outputWaveBits", 0);
        SoundObject output = symbols.lookup<SoundObject>("output", soundObjectType);

        Scalar length = output.length();
        if (!length.unit() == 1)
            throw Exception("output must be a time sequence");
        int blockAlign = (outputWaveBits >> 3);
        int bytesPerSecond = outputWaveRate * blockAlign;
        int count = static_cast<int>(length.toDouble());
        int l = count * bytesPerSecond;
        AutoHandle h = File(outputWave).openWrite();
        h.write("RIFF");
        h.write<UInt32>(l + 0x20);
        h.write("WAVEfmt ");
        h.write<UInt32>(0x0c);
        h.write<UInt16>(1);
        h.write<UInt16>(1); // Mono only for now
        h.write<UInt16>(outputWaveRate);
        h.write<UInt16>(bytesPerSecond);
        h.write<UInt16>(blockAlign);
        h.write<UInt16>(0);
        h.write("data");
        h.write<UInt16>(l);
        Array<Byte> outputData(l);
        output.write(&outputData[0], count, outputWaveRate);
        h.write(outputData);
    }
};
