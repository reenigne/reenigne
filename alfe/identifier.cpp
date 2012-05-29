class Operator
{
public:
    template<class T> class Base : public Operator
    {
    public:
        Base() : Operator(implementation()) { }
    private:
        class Implementation : public Operator::Implementation
        {
        public:
            String name() const { return T::name(); }
        };
        static ConstReference<Implementation> _implementation;
        static ConstReference<Implementation> implementation()
        {
            if (!_implementation.valid())
                _implementation = new Implementation();
            return _implementation;
        }
    };
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual String name() const = 0;
    };
    Operator(const Implementation* implementation)
      : _implementation(implementation) { }
private:
    ConstReference<Implementation> _implementation;
};

class OperatorEqualTo : public Operator::Base<OperatorEqualTo>
{
public:
    static String name() { return "=="; }
};

ConstReference<Operator::Base<OperatorEqualTo>::Implementation>
    Operator::Base<OperatorEqualTo>::_implementation;

class OperatorAssignment : public Operator::Base<OperatorAssignment>
{
public:
    static String name() { return "="; }
};

ConstReference<Operator::Base<OperatorAssignment>::Implementation>
    Operator::Base<OperatorAssignment>::_implementation;

class OperatorAddAssignment : public Operator::Base<OperatorAddAssignment>
{
public:
    static String name() { return "+="; }
};

ConstReference<Operator::Base<OperatorAddAssignment>::Implementation>
    Operator::Base<OperatorAddAssignment>::_implementation;

class OperatorSubtractAssignment
  : public Operator::Base<OperatorSubtractAssignment>
{
public:
    static String name() { return "-="; }
};

ConstReference<Operator::Base<OperatorSubtractAssignment>::Implementation>
    Operator::Base<OperatorSubtractAssignment>::_implementation;

class OperatorMultiplyAssignment
  : public Operator::Base<OperatorMultiplyAssignment>
{
public:
    static String name() { return "*="; }
};

ConstReference<Operator::Base<OperatorMultiplyAssignment>::Implementation>
    Operator::Base<OperatorMultiplyAssignment>::_implementation;

class OperatorDivideAssignment
  : public Operator::Base<OperatorDivideAssignment>
{
public:
    static String name() { return "/="; }
};

ConstReference<Operator::Base<OperatorDivideAssignment>::Implementation>
    Operator::Base<OperatorDivideAssignment>::_implementation;

class OperatorModuloAssignment
  : public Operator::Base<OperatorModuloAssignment>
{
public:
    static String name() { return "%="; }
};

ConstReference<Operator::Base<OperatorModuloAssignment>::Implementation>
    Operator::Base<OperatorModuloAssignment>::_implementation;

class OperatorShiftLeftAssignment
  : public Operator::Base<OperatorShiftLeftAssignment>
{
public:
    static String name() { return "<<="; }
};

ConstReference<Operator::Base<OperatorShiftLeftAssignment>::Implementation>
    Operator::Base<OperatorShiftLeftAssignment>::_implementation;

class OperatorShiftRightAssignment
  : public Operator::Base<OperatorShiftRightAssignment>
{
public:
    static String name() { return ">>="; }
};

ConstReference<Operator::Base<OperatorShiftRightAssignment>::Implementation>
    Operator::Base<OperatorShiftRightAssignment>::_implementation;

class OperatorBitwiseAndAssignment
  : public Operator::Base<OperatorBitwiseAndAssignment>
{
public:
    static String name() { return "&="; }
};

ConstReference<Operator::Base<OperatorBitwiseAndAssignment>::Implementation>
    Operator::Base<OperatorBitwiseAndAssignment>::_implementation;

class OperatorBitwiseOrAssignment
  : public Operator::Base<OperatorBitwiseOrAssignment>
{
public:
    static String name() { return "|="; }
};

ConstReference<Operator::Base<OperatorBitwiseOrAssignment>::Implementation>
    Operator::Base<OperatorBitwiseOrAssignment>::_implementation;

class OperatorBitwiseXorAssignment
  : public Operator::Base<OperatorBitwiseXorAssignment>
{
public:
    static String name() { return "~="; }
};

ConstReference<Operator::Base<OperatorBitwiseXorAssignment>::Implementation>
    Operator::Base<OperatorBitwiseXorAssignment>::_implementation;

class OperatorPowerAssignment : public Operator::Base<OperatorPowerAssignment>
{
public:
    static String name() { return "^="; }
};

ConstReference<Operator::Base<OperatorPowerAssignment>::Implementation>
    Operator::Base<OperatorPowerAssignment>::_implementation;

class OperatorBitwiseAnd : public Operator::Base<OperatorBitwiseAnd>
{
public:
    static String name() { return "&"; }
};

ConstReference<Operator::Base<OperatorBitwiseAnd>::Implementation>
    Operator::Base<OperatorBitwiseAnd>::_implementation;

class OperatorBitwiseOr : public Operator::Base<OperatorBitwiseOr>
{
public:
    static String name() { return "|"; }
};

ConstReference<Operator::Base<OperatorBitwiseOr>::Implementation>
    Operator::Base<OperatorBitwiseOr>::_implementation;

class OperatorBitwiseXor : public Operator::Base<OperatorBitwiseXor>
{
public:
    static String name() { return "~"; }
};

ConstReference<Operator::Base<OperatorBitwiseXor>::Implementation>
    Operator::Base<OperatorBitwiseXor>::_implementation;

class OperatorNotEqualTo : public Operator::Base<OperatorNotEqualTo>
{
public:
    static String name() { return "!="; }
};

ConstReference<Operator::Base<OperatorNotEqualTo>::Implementation>
    Operator::Base<OperatorNotEqualTo>::_implementation;






class Identifier : public Expression
{
public:
    static Identifier parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'a' || c > 'z')
            return Identifier();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get();
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int end = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(start, end);
        static String keywords[] = {
            "assembly",
            "break",
            "case",
            "catch",
            "continue",
            "default",
            "delete",
            "do",
            "done",
            "else",
            "elseIf",
            "elseUnless",
            "finally",
            "from",
            "for",
            "forever",
            "if",
            "in",
            "new",
            "nothing",
            "return",
            "switch",
            "this",
            "throw",
            "try",
            "unless",
            "until",
            "while"};
        for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
            if (name == keywords[i])
                return Identifier();
        String op("operator");
        if (name != op) {
            *source = s2;
            return Identifier(name, Span(location, endLocation));
        }
        Span span3;
        Atom atom = atomLast;
        CharacterSource s3 = s2;
        if (Space::parseCharacter(&s2, '(', &endSpan)) {
            if (Space::parseCharacter(&s2, ')', &endSpan))
                atom = atomFunctionCall;
            else
                s2.location().throwError("Expected )");
        }
        else if (Space::parseCharacter(&s2, '[', &endSpan)) {
            if (Space::parseCharacter(&s2, ']', &endSpan))
                atom = atomFunctionCall;
            else
                s2.location().throwError("Expected ]");
        }
        else if (Space::parseOperator(&s2, equalTo, &endSpan))
            atom = atomEqualTo;
        else if (Space::parseCharacter(&s2, '=', &endSpan))
            atom = atomAssignment;
        else if (Space::parseOperator(&s2, addAssignment, &endSpan))
            atom = atomAddAssignment;
        else if (Space::parseOperator(&s2, subtractAssignment, &endSpan))
            atom = atomSubtractAssignment;
        else if (Space::parseOperator(&s2, multiplyAssignment, &endSpan))
            atom = atomMultiplyAssignment;
        else if (Space::parseOperator(&s2, divideAssignment, &endSpan))
            atom = atomDivideAssignment;
        else if (Space::parseOperator(&s2, moduloAssignment, &endSpan))
            atom = atomModuloAssignment;
        else if (Space::parseOperator(&s2, shiftLeftAssignment, &endSpan))
            atom = atomShiftLeftAssignment;
        else if (Space::parseOperator(&s2, shiftRightAssignment, &endSpan))
            atom = atomShiftRightAssignment;
        else if (Space::parseOperator(&s2, bitwiseAndAssignment, &endSpan))
            atom = atomBitwiseAndAssignment;
        else if (Space::parseOperator(&s2, bitwiseOrAssignment, &endSpan))
            atom = atomBitwiseOrAssignment;
        else if (Space::parseOperator(&s2, bitwiseXorAssignment, &endSpan))
            atom = atomBitwiseXorAssignment;
        else if (Space::parseOperator(&s2, powerAssignment, &endSpan))
            atom = atomPowerAssignment;
        else if (Space::parseCharacter(&s2, '|', &endSpan))
            atom = atomBitwiseOr;
        else if (Space::parseCharacter(&s2, '~', &endSpan))
            atom = atomBitwiseXor;
        else if (Space::parseCharacter(&s2, '&', &endSpan))
            atom = atomBitwiseAnd;
        else if (Space::parseOperator(&s2, notEqualTo, &endSpan))
            atom = atomNotEqualTo;
        else if (Space::parseOperator(&s2, lessThanOrEqualTo, &endSpan))
            atom = atomLessThanOrEqualTo;
        else if (Space::parseOperator(&s2, shiftRight, &endSpan))
            atom = atomShiftRight;
        else if (Space::parseCharacter(&s3, '<', &endSpan)) {
            atom = atomLessThan;
            // Only if we know it's not operator<<T>() can we try operator<<()
            CharacterSource s4 = s3;
            SymbolArray templateArgumentList = parseTemplateArgumentList(&s4);
            if (templateArgumentList.count() == 0 &&
                Space::parseOperator(&s2, shiftLeft, &endSpan))
                atom = atomShiftLeft;
            else
                s2 = s3;
        }
        else if (Space::parseOperator(&s2, greaterThanOrEqualTo, &endSpan))
            atom = atomGreaterThanOrEqualTo;
        else if (Space::parseCharacter(&s2, '>', &endSpan))
            atom = atomGreaterThan;
        else if (Space::parseCharacter(&s2, '+', &endSpan))
            atom = atomAdd;
        else if (Space::parseCharacter(&s2, '-', &endSpan))
            atom = atomSubtract;
        else if (Space::parseCharacter(&s2, '/', &endSpan))
            atom = atomDivide;
        else if (Space::parseCharacter(&s2, '*', &endSpan))
            atom = atomMultiply;
        else if (Space::parseCharacter(&s2, '%', &endSpan))
            atom = atomModulo;
        else
            s2.location().throwError("Expected an operator");
        return Symbol(atomIdentifier, Symbol(atom), newSpan(startSpan + endSpan));

    }
    String name() const { return implementation()->name(); }
private:
    class Implementation : public Expression::Implementation
    {
    public:
        Implementation(const String& name, const Span& span)
          : Expression::Implementation(span), _name(name) { }
        String name() const { return _name; }
    private:
        String _name;
    };
    Identifier() { }
    Identifier(const String& name, const Span& span)
      : Expression(new Implementation(name, span)) { }

    const Implementation* implementation() const
    {
        return ConstReference<Implementation>(_implementation);
    }
};

