class Operator
{
public:
    String name() const { return _implementation->name(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual String name() const = 0;
    };
private:
    ConstReference<Implementation> _implementation;
};

template<class T> class OperatorBase : public Operator
{
public:
    OperatorBase() : Operator(op()) { }
private:
    class Implementation : public Operator::Implementation
    {
    public:
        String name() const { return T::name(); }
    };
    static Operator _operator;
    static Operator op()
    {
        if (!_operator.valid())
            _operator = new Implementation();
        return _operator;
    }
};

class OperatorEqualTo : public OperatorBase<OperatorEqualTo>
{
public:
    static String name() { return "=="; }
};

Operator OperatorBase<OperatorEqualTo>::_operator;

class OperatorAssignment : public OperatorBase<OperatorAssignment>
{
public:
    static String name() { return "="; }
};

Operator OperatorBase<OperatorAssignment>::_operator;

class OperatorAddAssignment : public OperatorBase<OperatorAddAssignment>
{
public:
    static String name() { return "+="; }
};

Operator OperatorBase<OperatorAddAssignment>::_operator;

class OperatorSubtractAssignment
  : public OperatorBase<OperatorSubtractAssignment>
{
public:
    static String name() { return "-="; }
};

Operator OperatorBase<OperatorSubtractAssignment>::_operator;

class OperatorMultiplyAssignment
  : public OperatorBase<OperatorMultiplyAssignment>
{
public:
    static String name() { return "*="; }
};

Operator OperatorBase<OperatorMultiplyAssignment>::_operator;

class OperatorDivideAssignment : public OperatorBase<OperatorDivideAssignment>
{
public:
    static String name() { return "/="; }
};

Operator OperatorBase<OperatorDivideAssignment>::_operator;

class OperatorModuloAssignment : public OperatorBase<OperatorModuloAssignment>
{
public:
    static String name() { return "%="; }
};

Operator OperatorBase<OperatorModuloAssignment>::_operator;

class OperatorShiftLeftAssignment
  : public OperatorBase<OperatorShiftLeftAssignment>
{
public:
    static String name() { return "<<="; }
};

Operator OperatorBase<OperatorShiftLeftAssignment>::_operator;

class OperatorShiftRightAssignment
  : public OperatorBase<OperatorShiftRightAssignment>
{
public:
    static String name() { return ">>="; }
};

Operator OperatorBase<OperatorShiftRightAssignment>::_operator;

class OperatorBitwiseAndAssignment
  : public OperatorBase<OperatorBitwiseAndAssignment>
{
public:
    static String name() { return "&="; }
};

Operator OperatorBase<OperatorBitwiseAndAssignment>::_operator;

class OperatorBitwiseOrAssignment
  : public OperatorBase<OperatorBitwiseOrAssignment>
{
public:
    static String name() { return "|="; }
};

Operator OperatorBase<OperatorBitwiseOrAssignment>::_operator;

class OperatorBitwiseXorAssignment
  : public OperatorBase<OperatorBitwiseXorAssignment>
{
public:
    static String name() { return "~="; }
};

Operator OperatorBase<OperatorBitwiseXorAssignment>::_operator;

class OperatorPowerAssignment : public OperatorBase<OperatorPowerAssignment>
{
public:
    static String name() { return "^="; }
};

Operator OperatorBase<OperatorPowerAssignment>::_operator;

class OperatorBitwiseAnd : public OperatorBase<OperatorBitwiseAnd>
{
public:
    static String name() { return "&"; }
};

Operator OperatorBase<OperatorBitwiseAnd>::_operator;

class OperatorBitwiseOr : public OperatorBase<OperatorBitwiseOr>
{
public:
    static String name() { return "|"; }
};

Operator OperatorBase<OperatorBitwiseOr>::_operator;

class OperatorBitwiseXor : public OperatorBase<OperatorBitwiseXor>
{
public:
    static String name() { return "~"; }
};

Operator OperatorBase<OperatorBitwiseXor>::_operator;

class OperatorNotEqualTo : public OperatorBase<OperatorNotEqualTo>
{
public:
    static String name() { return "!="; }
};

Operator OperatorBase<OperatorNotEqualTo>::_operator;

class OperatorLessThan : public OperatorBase<OperatorLessThan>
{
public:
    static String name() { return "<"; }
};

Operator OperatorBase<OperatorLessThan>::_operator;

class OperatorGreaterThan : public OperatorBase<OperatorGreaterThan>
{
public:
    static String name() { return ">"; }
};

Operator OperatorBase<OperatorGreaterThan>::_operator;

class OperatorLessThanOrEqualTo : public OperatorBase<OperatorLessThanOrEqualTo>
{
public:
    static String name() { return "<="; }
};

Operator OperatorBase<OperatorLessThanOrEqualTo>::_operator;

class OperatorGreaterThanOrEqualTo
    : public OperatorBase<OperatorGreaterThanOrEqualTo>
{
public:
    static String name() { return ">="; }
};

Operator OperatorBase<OperatorGreaterThanOrEqualTo>::_operator;

class OperatorShiftLeft : public OperatorBase<OperatorShiftLeft>
{
public:
    static String name() { return "<<"; }
};

Operator OperatorBase<OperatorShiftLeft>::_operator;

class OperatorShiftRight : public OperatorBase<OperatorShiftRight>
{
public:
    static String name() { return ">>"; }
};

Operator OperatorBase<OperatorShiftRight>::_operator;

class OperatorAdd : public OperatorBase<OperatorAdd>
{
public:
    static String name() { return "+"; }
};

Operator OperatorBase<OperatorAdd>::_operator;

class OperatorSubtract : public OperatorBase<OperatorSubtract>
{
public:
    static String name() { return "-"; }
};

Operator OperatorBase<OperatorSubtract>::_operator;

class OperatorMultiply : public OperatorBase<OperatorMultiply>
{
public:
    static String name() { return "*"; }
};

Operator OperatorBase<OperatorMultiply>::_operator;

class OperatorDivide : public OperatorBase<OperatorDivide>
{
public:
    static String name() { return "/"; }
};

Operator OperatorBase<OperatorDivide>::_operator;

class OperatorModulo : public OperatorBase<OperatorModulo>
{
public:
    static String name() { return "%"; }
};

Operator OperatorBase<OperatorModulo>::_operator;

class OperatorPower : public OperatorBase<OperatorPower>
{
public:
    static String name() { return "^"; }
};

Operator OperatorBase<OperatorPower>::_operator;

class OperatorFunctionCall : public OperatorBase<OperatorFunctionCall>
{
public:
    static String name() { return "()"; }
};

Operator OperatorBase<OperatorFunctionCall>::_operator;

class OperatorIndex : public OperatorBase<OperatorIndex>
{
public:
    static String name() { return "[]"; }
};

Operator OperatorBase<OperatorIndex>::_operator;

template<class T> class IdentifierTemplate;
typedef IdentifierTemplate<void> Identifier;

template<class T> class IdentifierTemplate : public ExpressionTemplate<T>
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
        Span span(location, endLocation);
        if (name != op) {
            *source = s2;
            return Identifier(new NameImplementation(name, span));
        }
        Span endSpan;
        Span span3;
        Operator o;
        CharacterSource s3 = s2;
        if (Space::parseCharacter(&s2, '(', &endSpan)) {
            if (Space::parseCharacter(&s2, ')', &endSpan))
                o = OperatorFunctionCall();
            else
                s2.location().throwError("Expected )");
        }
        else if (Space::parseCharacter(&s2, '[', &endSpan)) {
            if (Space::parseCharacter(&s2, ']', &endSpan))
                o = OperatorIndex();
            else
                s2.location().throwError("Expected ]");
        }
        else if (Space::parseOperator(&s2, equalTo, &endSpan))
            o = OperatorEqualTo();
        else if (Space::parseCharacter(&s2, '=', &endSpan))
            o = OperatorAssignment();
        else if (Space::parseOperator(&s2, addAssignment, &endSpan))
            o = OperatorAddAssignment();
        else if (Space::parseOperator(&s2, subtractAssignment, &endSpan))
            o = OperatorSubtractAssignment();
        else if (Space::parseOperator(&s2, multiplyAssignment, &endSpan))
            o = OperatorMultiplyAssignment();
        else if (Space::parseOperator(&s2, divideAssignment, &endSpan))
            o = OperatorDivideAssignment();
        else if (Space::parseOperator(&s2, moduloAssignment, &endSpan))
            o = OperatorModuloAssignment();
        else if (Space::parseOperator(&s2, shiftLeftAssignment, &endSpan))
            o = OperatorShiftLeftAssignment();
        else if (Space::parseOperator(&s2, shiftRightAssignment, &endSpan))
            o = OperatorShiftRightAssignment();
        else if (Space::parseOperator(&s2, bitwiseAndAssignment, &endSpan))
            o = OperatorBitwiseAndAssignment();
        else if (Space::parseOperator(&s2, bitwiseOrAssignment, &endSpan))
            o = OperatorBitwiseOrAssignment();
        else if (Space::parseOperator(&s2, bitwiseXorAssignment, &endSpan))
            o = OperatorBitwiseXorAssignment();
        else if (Space::parseOperator(&s2, powerAssignment, &endSpan))
            o = OperatorPowerAssignment();
        else if (Space::parseCharacter(&s2, '|', &endSpan))
            o = OperatorBitwiseOr();
        else if (Space::parseCharacter(&s2, '~', &endSpan))
            o = OperatorBitwiseXor();
        else if (Space::parseCharacter(&s2, '&', &endSpan))
            o = OperatorBitwiseAnd();
        else if (Space::parseOperator(&s2, notEqualTo, &endSpan))
            o = OperatorNotEqualTo();
        else if (Space::parseOperator(&s2, lessThanOrEqualTo, &endSpan))
            o = OperatorLessThanOrEqualTo();
        else if (Space::parseOperator(&s2, shiftRight, &endSpan))
            o = OperatorShiftRight();
        else if (Space::parseCharacter(&s3, '<', &endSpan)) {
            o = OperatorLessThan();
            // Only if we know it's not operator<<T>() can we try operator<<()
            CharacterSource s4 = s3;
            TemplateArgumentList templateArgumentList =
                TemplateArgumentList::parse(&s4);
            if (templateArgumentList.count() == 0 &&
                Space::parseOperator(&s2, shiftLeft, &endSpan))
                o = OperatorShiftLeft();
            else
                s2 = s3;
        }
        else if (Space::parseOperator(&s2, greaterThanOrEqualTo, &endSpan))
            o = OperatorGreaterThanOrEqualTo();
        else if (Space::parseCharacter(&s2, '>', &endSpan))
            o = OperatorGreaterThan();
        else if (Space::parseCharacter(&s2, '+', &endSpan))
            o = OperatorAdd();
        else if (Space::parseCharacter(&s2, '-', &endSpan))
            o = OperatorSubtract();
        else if (Space::parseCharacter(&s2, '/', &endSpan))
            o = OperatorDivide();
        else if (Space::parseCharacter(&s2, '*', &endSpan))
            o = OperatorMultiply();
        else if (Space::parseCharacter(&s2, '%', &endSpan))
            o = OperatorModulo();
        else if (Space::parseCharacter(&s2, '^', &endSpan))
            o = OperatorPower();
        else
            s2.location().throwError("Expected an operator");
        return Identifier(o, span + endSpan);
    }

    IdentifierTemplate(const Operator& op, const Span& span)
      : Expression(new OperatorImplementation(op, span)) { }

    String name() const { return implementation<Identifier>()->name(); }
private:
    class Implementation : public ExpressionTemplate<T>::Implementation
    {
    public:
        Implementation(const Span& span) : Expression::Implementation(span) { }
        virtual String name() const = 0;
    };
    class NameImplementation : public Implementation
    {
    public:
        NameImplementation(const String& name, const Span& span)
          : Implementation(span), _name(name) { }
        String name() const { return _name; }
    private:
        String _name;
    };
    class OperatorImplementation : public Implementation
    {
    public:
        OperatorImplementation(const Operator& name, const Span& span)
          : Implementation(span), _name(name) { }
        String name() const { return "operator" + _c.name(); }
    private:
        Operator _o;
    };
    IdentifierTemplate() { }
    IdentifierTemplate(Implementation* implementation)
      : Expression(implementation) { }
};

