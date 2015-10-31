#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/type.h"
#include "alfe/rational.h"
#include "alfe/complex.h"

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

template<class T> class SoundObjectTemplate : public Handle
{
public:
    SoundObject power(const SoundObject& other) const
    {
        return body()->power(other.body());
    }
    SoundObject operator-() const { return body()->negative(); }
    SoundObject timeScale(const SoundObject& other) const
    {
        return body()->timeScale(other.body());
    }
    SoundObject operator*(const SoundObject& other) const
    {
        return body()->product(other.body());
    }
    SoundObject operator/(const SoundObject& other) const
    {
        return body()->quotient(other.body());
    }
    SoundObject operator+(const SoundObject& other) const
    {
        return body()->sum(other.body());
    }
    SoundObject operator-(const SoundObject& other) const
    {
        return body()->difference(other.body());
    }
    SoundObject operator&(const SoundObject& other) const
    {
        return body()->voice(other.body());
    }
    SoundObject operator|(const SoundObject& other) const
    {
        return body()->sequence(other.body());
    }
protected:
    class Body : public Handle::Body
    {
    public:
        virtual SoundObject power(const Body* other) const = 0;
        virtual SoundObject negative() const = 0;
        virtual SoundObject timeScale(const Body* other) const = 0;
        virtual SoundObject product(const Body* other) const = 0;
        virtual SoundObject quotient(const Body* other) const = 0;
        virtual SoundObject sum(const Body* other) const = 0;
        virtual SoundObject difference(const Body* other) const = 0;
        virtual SoundObject voice(const Body* other) const = 0;
        virtual SoundObject sequence(const Body* other) const = 0;
    };
};

template<class T> class EntryTemplate : public SoundObject
{
protected:
    class Body : public SoundObject::Body
    {
    };
};

template<class T> class ScalarTemplate : public Entry
{
public:
    ScalarTemplate(int value = 0, int unit = 0)
      : Entry(new RationalBody(unit, value)) { }

    bool isReal() { return body()->isReal(); }
    int unit() { return body()->unit(); }
    double toRealDouble() { return body()->toDouble().x; }
protected:
    ScalarTemplate(Body* body) : Entry(body) { }

    class Body : public Entry::Body
    {
    public:
        Body(int unit) : _unit(unit) { }
        bool isNumber() const { return _unit == 0; }
        int unit() const { return _unit; }
        virtual bool isRational() const = 0;
        virtual bool isInteger() const = 0;
        virtual int toInteger() const = 0;
        virtual Complex<double> toDouble() const = 0;
        virtual Complex<Rational> toRational() const = 0;
    protected:
        int _unit;
    };

    class DoubleBody : public Body
    {
    public:
        DoubleBody(int unit = 0, Complex<double> value = 0)
          : Body(unit), _value(value) { }
        bool isRational() const { return false; }
        bool isInteger() const
        {
            return _value.y == 0 && isDoubleInteger(_value.x);
        }
        int toInteger() const { return _value.x; }
        Complex<double> toDouble() const { return _value; }
        Complex<Rational> toRational() const
        {
            return Complex<Rational>(
                rationalFromDouble(_value.x), rationalFromDouble(_value.y));
        }

        SoundObject power(const SoundObject::Body* o) const
        {
            auto other = o->as<Body>;
            if (other == 0)
                throw Exception("Don't know how to do that yet!\n");
            if (other->isRational()) {
                Complex<Rational> p = _unit;
                p *= other->toRational();
                if (p.y.numerator != 0 || p.x.denominator != 1)
                    throw Exception(
                        "Operation yields a non-integral time power.\n");
                return new DoubleBody(p.x.numerator,
                    pow(_value, other->toDouble()));
            }
            Complex<double> p = _unit;
            p *= other->toDouble();
            if (p.y != 0 || !isDoubleInteger(p.x))
                throw Exception(
                    "Operation yields a non-integral time power.\n");
            return new DoubleBody(static_cast<int>(p.x),
                pow(_value, other->toDouble()));
        }
        SoundObject negative() const
        {
            return new DoubleBody(_unit, -_value);
        }
        SoundObject timeScale(const SoundObject::Body* o) const
        {
            throw NotYetImplementedException;
        }
        SoundObject product(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject quotient(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject sum(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject difference(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject voice(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject sequence(const Body* other) const
        {
            throw NotYetImplementedException;
        }
    private:
        Complex<double> _value;
    };

    class RationalBody : public Body
    {
    public:
        RationalBody(int unit = 0, Complex<Rational> value = 0)
          : Body(unit), _value(value) { }
        bool isRational() const { return true; }
        bool isInteger() const
        {
            return _value.x.denominator == 1 && _value.y.numerator == 0;
        }
        int toInteger() const
        {
            return _value.x.numerator/_value.x.denominator;
        }
        Complex<double> toDouble() const
        {
            return Complex<double>(
                static_cast<double>(_value.numerator.x)/_value.denominator.x,
                static_cast<double>(_value.numerator.y)/_value.denominator.y);
        }
        Complex<double> toRational() const { return _value; }

        SoundObject power(const SoundObject::Body* o) const
        {
            throw Exception("Don't know how to do that yet!\n");
        }
        SoundObject negative() const
        {
            return new RationalBody(_unit, -_value);
        }
        SoundObject timeScale(const SoundObject::Body* o) const
        {
            throw Exception("Don't know how to do that yet!\n");
        }
        SoundObject product(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject quotient(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject sum(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject difference(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject voice(const Body* other) const
        {
            throw NotYetImplementedException;
        }
        SoundObject sequence(const Body* other) const
        {
            throw NotYetImplementedException;
        }
    private:
        Complex<Rational> _value;
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
    class Body : public Entry::Body
    {
    public:
        Body(Scalar extent) : _extent(extent) { }
        virtual void writeOutput(Byte* destination, int count, int samplesRate)
            const = 0;
    protected:
        Scalar _extent;
    };
};

template<class T> class ImplicitSampleTemplate : public Sample
{
protected:
    class Body : public Sample::Body
    {
    };
};

template<class T> class SineSampleTemplate : public ImplicitSample
{
protected:
    class Body : public ImplicitSample::Body
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

class SampleData : public Handle
{
public:
    SampleData(int count) : Handle(new Body(count)) { }
    Complex<double>* data() { return body()->data(); }
private:
    class Body : public Handle::Body
    {
    public:
        Body(int count) : _data(count) { }
        Complex<double>* data() { return &_data[0]; }
    private:
        Array<Complex<double>> _data;  // TODO: Use FFTW type?
    };
    Body* body() { return as<Body>(); }
};

template<class T> class ExplicitSampleTemplate : public Sample
{
protected:
    class Body : public Sample::Body
    {
    public:
        Body(Scalar extent, SampleData data)
          : Sample::Body(extent), _data(data) { }
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

class ParseTreeObject : public ConstHandle
{
public:
    Span span() const { return body()->span(); }

    class Body : public Handle::Body
    {
    public:
        Body(const Span& span) : _span(span) { }
        Span span() const { return _span; }
    private:
        Span _span;
    };

protected:
    ParseTreeObject() { }
    ParseTreeObject(const Body* body) : ConstHandle(body) { }

    const Body* body() const { return as<Body>(); }
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
      : Statement(new Body(expression, span)) { }
private:
    ExpressionStatement() { }

    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
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
    StatementTemplate(const Body* body)
      : ParseTreeObject(body) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span) : ParseTreeObject::Body(span) { }
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
        return new Body(sequence, span);
    }
private:
    StatementSequence(const Body* body) : ParseTreeObject(body) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const List<Statement>& sequence, const Span& span)
          : ParseTreeObject::Body(span), _sequence(sequence) { }
    private:
        List<Statement> _sequence;
    };
};

template<class T> class SymbolTableTemplate : public Handle
{
public:
    void add(String identifier, Expression expression) { body()->add(identifier, expression); }
    Expression lookup(String identifier) { return body()->lookup(identifier); }

private:
    class Body : public Handle::Body
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
    class RootBody : public Body
    {
    public:
        bool isRoot() const { return true; }
    };
    class VoiceBody : public Body
    {
    public:
        bool isRoot() const { return false; }
    private:
        SymbolTable _parent;
        int _voice;
    };
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
        AutoStream st = File(outputWave).openWrite();
        st.write("RIFF");
        st.write<UInt32>(l + 0x20);
        st.write("WAVEfmt ");
        st.write<UInt32>(0x0c);
        st.write<UInt16>(1);
        st.write<UInt16>(1); // Mono only for now
        st.write<UInt16>(outputWaveRate);
        st.write<UInt16>(bytesPerSecond);
        st.write<UInt16>(blockAlign);
        st.write<UInt16>(0);
        st.write("data");
        st.write<UInt16>(l);
        Array<Byte> outputData(l);
        output.write(&outputData[0], count, outputWaveRate);
        st.write(outputData);
    }
};
