#include "alfe/main.h"

#ifndef INCLUDED_CODE_H
#define INCLUDED_CODE_H

template<class U> class AnnotationT;
typedef AnnotationT<void> Annotation;

class ParseTreeObject;

template<class T> class TycoT;
typedef TycoT<void> Tyco;

class CodeWalker
{
public:
    enum class Result { recurse, advance, abort, remove };

    virtual Result visit(Annotation c) = 0;
    virtual Result visit(ParseTreeObject o) = 0;
    virtual Result visit(Tyco t) = 0;
    // TODO: Funco? OverloadedFunctionSet? Kind?
};

class CodeNode;

class Code;


template<class T> class ExpressionT;
typedef ExpressionT<void> Expression;

template<class T> class IdentifierT;
typedef IdentifierT<void> Identifier;

template<class T> class TycoSpecifierT;
typedef TycoSpecifierT<void> TycoSpecifier;

template<class T> class TypeSpecifierT;
typedef TypeSpecifierT<void> TypeSpecifier;

template<class T> class TycoIdentifierT;
typedef TycoIdentifierT<void> TycoIdentifier;

template<class T> class TemplateArgumentsT;
typedef TemplateArgumentsT<void> TemplateArguments;

template<class T> class ClassTycoSpecifierT;
typedef ClassTycoSpecifierT<void> ClassTycoSpecifier;

template<class T> class TypeOfTypeSpecifierT;
typedef TypeOfTypeSpecifierT<void> TypeOfTypeSpecifier;

template<class T> class TypeParameterT;
typedef TypeParameterT<void> TypeParameter;

template<class T> class TycoT;
typedef TycoT<void> Tyco;

template<class T> class ScopeT;
typedef ScopeT<void> Scope;


template<class T> class ExpressionT;
typedef ExpressionT<void> Expression;

template<class T> class DotExpressionT;
typedef DotExpressionT<void> DotExpression;

template<class T> class IdentifierT;
typedef IdentifierT<void> Identifier;

template<class T> class CallExpressionT;
typedef CallExpressionT<void> CallExpression;

template<class T> class FunctionCallExpressionT;
typedef FunctionCallExpressionT<void> FunctionCallExpression;

template<class T> class TypeT;
typedef TypeT<void> Type;

template<class T> class ValueT;
typedef ValueT<void> Value;

template<class T> class ValueT;
typedef ValueT<void> Value;

template<class T> class TycoIdentifierT;
typedef TycoIdentifierT<void> TycoIdentifier;

template<class T> class LogicalOrExpressionT;
typedef LogicalOrExpressionT<void> LogicalOrExpression;

template<class T> class ConditionalExpressionT;
typedef ConditionalExpressionT<void> ConditionalExpression;

template<class T> class NumericLiteralT;
typedef NumericLiteralT<void> NumericLiteral;

template<class T> class StructuredTypeT;
typedef StructuredTypeT<void> StructuredType;

template<class T> class LValueTypeT;
typedef LValueTypeT<void> LValueType;

template<class T> class LValueT;
typedef LValueT<void> LValue;

template<class T> class StructureT;
typedef StructureT<void> Structure;

template<class T> class OverloadedFunctionSetT;
typedef OverloadedFunctionSetT<void> OverloadedFunctionSet;

template<class T> class VariableDefinitionT;
typedef VariableDefinitionT<void> VariableDefinition;

class Function;

template<class T> class FuncoTypeT;
typedef FuncoTypeT<void> FuncoType;

template<class T> class BooleanTypeT;
typedef BooleanTypeT<void> BooleanType;

template<class T> class StringLiteralExpressionT;
typedef StringLiteralExpressionT<void> StringLiteralExpression;

class ArrayType;



#include "alfe/hash_table.h"
#include "alfe/space.h"
#include "alfe/any.h"
#include "alfe/nullary.h"
#include "alfe/kind.h"
#include "alfe/assert.h"
#include "alfe/operator.h"
#include "alfe/parse_tree_object.h"
#include "alfe/expression.h"
#include "alfe/type.h"
#include "alfe/type_specifier.h"
#include "alfe/statement.h"
#include "alfe/identifier.h"
#include "alfe/vectors.h"
#include "alfe/rational.h"
#include "alfe/reference.h"
#include "alfe/set.h"
#include "alfe/concrete.h"
#include "alfe/function.h"
#include "alfe/integer_functions.h"
#include "alfe/string_functions.h"
#include "alfe/rational_functions.h"
#include "alfe/concrete_functions.h"
#include "alfe/array_functions.h"
#include "alfe/boolean_functions.h"
#include "alfe/double_functions.h"

class AnnotationNotFoundException : public Exception
{
public:
    AnnotationNotFoundException() : Exception("Annotation not found") { }
};

template<class U> class AnnotationT : public HandleBase
{
public:
    AnnotationT(const HandleBase& other) : HandleBase(to<Body>(other)) { }
    template<typename T, typename... Args> T annotate(Args&&... args)
    {
        T t = create<T::Body, Args...>(std::forward<Args>(args)...);
        Annotation a = t;
        Body* b = a.body();
        b->_next = next();
        next(b);
        return t;
    }
    void remove() { body()->remove(); }
    template<class T> bool hasAnnotation() const
    {
        return body()->hasAnnotation<T>();
    }
    template<class T> T getAnnotation() const
    {
        return body()->getAnnotation<T>();
    }
    CodeWalker::Result walkAnnotations(CodeWalker* walker)
    {
        return body()->walkAnnotations(walker);
    }
    //template<class B> static Annotation to(const Annotation& other)
    //{
    //    return Annotation(HandleBase::to<B>(other));
    //}
protected:
    class Body : public HandleBase::Body
    {
    public:
        Body() : _next(this) { }
    protected:
        void destroy()
        {
            Body* a = _next;
            if (a == 0)  // We're already being destroyed. TODO: is this needed?
                return;
            while (a != this) {
                Body* n = a->_next;
                a->_next = 0;                           // TODO: is this needed?
                delete a;
                a = n;
            }
            delete this;
        }
    private:
        Annotation annotation() { return handle<Annotation>(); }
        CodeWalker::Result walkAnnotations(CodeWalker* walker)
        {
            Body* b = this;
            do {
                auto r = walker->visit(b->annotation());
                if (r == CodeWalker::Result::recurse) {
                    CodeNode n = b->annotation();
                    if (n.valid())
                        r = n.walk(walker);
                }
                if (r == CodeWalker::Result::abort)
                    return r;
                Body* last = b;
                b = b->_next;
                if (r == CodeWalker::Result::remove)
                    last->destroy();
            } while (b != this);
            return CodeWalker::Result::advance;
        }
        void remove()
        {
            Body* b = this;
            do {
                Body* n = b->_next;
                if (n == this) {
                    b->_next = _next;
                    delete this;
                    return;
                }
                b = n;
            } while (true);
        }
        template<class T> bool hasAnnotation()
        {
            Body* b = this;
            do {
                T::Body* a = dynamic_cast<T::Body*>(b);
                if (a != 0)
                    return true;
                b = b->_next;
            } while (b != this);
            return false;
        }
        template<class T> T getAnnotation()
        {
            Body* b = this;
            do {
                T::Body* a = dynamic_cast<T::Body*>(b);
                if (a != 0)
                    return a->handle<T>();
                b = b->_next;
            } while (b != this);
            throw AnnotationNotFoundException();
        }
        template<typename T, typename... Args> T
            annotate(Args&&... args)
        {
            if (hasAnnotation<T>)
                return getAnnotation<T>();  // TODO: optimize
            T a = HandleBase::create<T::Body>(std::forward<Args>(args)...);
            Body* b = Annotation(a).body();
            b->_next = _next;
            _next = b;
            return a;
        }

        Body* _next;
        friend class AnnotationT<U>;
    };
private:
    Body* body() const { return to<Body>(); }
    void next(Body* next) { body()->_next = next; }
    Body* next() { return body()->_next; }
};

class CodeFormAnnotation : public Annotation
{
public:
    bool resolved() { return body()->_resolved; }
    void setResolved(bool resolved) { body()->_resolved = resolved; }
    bool flattened() { return body()->_flattened; }
    void setFlattened(bool flattened) { body()->_flattened = flattened; }
protected:
    class Body : public Annotation::Body
    {
    public:
        Body() : _resolved(false), _flattened(false) { }
    private:
        bool _resolved;
        bool _flattened;
        friend class CodeFormAnnotation;
    };
    Body* body() { return as<Body>(); }
};

template<class T, class Base> class DoublyLinkedList;

// T is the type of the items in the list
// Base is the type that the list member type derives from
template<class T, class Base = HandleBase> class DoublyLinkedListMember
  : public Base
{
public:
    DoublyLinkedListMember(const HandleBase& base) : Base(to<Body>(base)) { }
    T previous() const { return body()->_prev; }
    T next() const { return body()->_next; }
    void destroy() const
    {
        T p = previous();
        T n = next();
        T t = p.next();
        p.next(n);
        n.prev(p);
        clear(t);
        Base::destroy();
    }
    template<typename T, typename... Args> DoublyLinkedListMember
        insert(Args&&... args)
    {
        T m = Base::create<T::Body>(std::forward<Args>(args)...);
        T p = previous();
        T t = p.next();
        m.next(t);
        m.prev(p);
        prev(m);
        p.next(m);
        return t;
    }
    // Moves all the elements in other to immediately before this. If this is
    // a LinkedList, other will be inserted at the end.
    template<class T2 = T, class B2 = Base> T insert(DoublyLinkedList<T2, B2> other)
    {
        T tt = previous().next();
        if (!o.isEmpty()) {
            T f = other.next();
            T l = other.previous();
            T t = l.next();
            T p = previous();
            f.prev(p);
            l.next(tt);
            p.next(f);
            prev(l);
            other.clear(t);
        }
        return tt;
    }
    DoublyLinkedList<T, Base> split(T start)
    {
        T l = start.previous();
        T p = previous();
        T t = p.next();
        DoublyLinkedList r;
        r.next(start);
        start.prev(r);
        r.prev(p);
        p.next(r);
        l.next(t);
        previous(l);
        return r;
    }
    // TODO: Add this back in if a non-Code user of DoublyLinkedList wants a
    // walk() method.
    //bool walk(const std::function<bool(T)>& function)
    //{
    //    return body()->walk(function);
    //}
    bool isSentinel() const { return as<DoublyLinkedList::Body>() != 0; }
    bool isEmpty() const { return isSameNode(*this, next()); }

    template<class B> static DoublyLinkedListMember to(
        const DoublyLinkedListMember& other)
    {
        return DoublyLinkedListMember(Base::to<B>(other));
    }
    bool isSameNode(DoublyLinkedListMember other)
    {
        return body() == other.body();
    }

protected:
    class Body : public Base::Body
    {
    protected:
        Body()
        {
            _prev = doublyLinkedListMember();
            _next = doublyLinkedListMember();
        }
        T doublyLinkedListMember() { return handle<T>(); }
        //virtual bool walk(
        //    const std::function<bool(T)>& function)
        //{
        //    return function(doublyLinkedListMember());
        //}
    private:
        T _prev;
        T _next;

        friend class DoublyLinkedList<T, Base>;
        friend class DoublyLinkedListMember<T, Base>;
    };
private:
    Body* body() const { return as<Body>(); }
    void next(T next) const { body()->_next = next; }
    void prev(T prev) const { body()->_prev = prev; }
    void clear(T t) const { next(t); prev(t); }

    friend class DoublyLinkedList<T, Base>;
//    friend class DoublyLinkedList<T, Base>::Body;
};

template<class T, class Base> class DoublyLinkedList : public T
{
public:
    DoublyLinkedList() : T(T::create<Body>()) { }
    DoublyLinkedList(const T& other) : T(to<Body>(other)) { }
    // Two linked lists compare as equal if they have the same number of
    // members, and corresponding members compare equal.
    bool operator==(const DoublyLinkedList& other) const
    {
        Iterator l = begin();
        Iterator r = other.begin();
        do {
            if (l == end())
                return r == other.end();
            if (r == other.end() || *l != *r)
                return false;
            ++l;
            ++r;
        } while (true);
    }
    bool operator!=(const DoublyLinkedList& other) const
    {
        return !(*this == other);
    }
    class Iterator
    {
    public:
        T& operator*() const { return _node; }
        T* operator->() const { return static_cast<T*>(&_node); }
        const Iterator& operator++()
        {
            _node = _node.next();
            return *this;
        }
        bool operator==(const Iterator& other) const
        {
            return _node.isSameNode(other._node);
        }
        bool operator!=(const Iterator& other) const
        {
            return !operator==(other);
        }
    private:
        DoublyLinkedListMember<T, Base> _node;

        Iterator(DoublyLinkedListMember<T, Base> node) : _node(node) { }

        friend class DoublyLinkedList<T, Base>;
    };
    Iterator begin() { return Iterator(next()); }
    Iterator end() { return Iterator(*this); }

    template<class B, typename... Args> static DoublyLinkedList
        create(Args&&... args)
    {
        return DoublyLinkedList(
            T::create<B, Args...>(std::forward<Args>(args)...));
    }
protected:
    class Body : public T::Body
    {
    public:
    protected:
        void destroy()
        {
            DoublyLinkedListMember e = handle<DoublyLinkedListMember>();
            DoublyLinkedListMember c = e.next();
            while (!c.isSameNode(e)) {
                DoublyLinkedListMember n = c.next();
                c.destroy();
                c = n;
            }
            T::Body::destroy();
        }
    };
};


class CodeNode : public DoublyLinkedListMember<CodeNode, Annotation>
{
private:
    typedef DoublyLinkedListMember<CodeNode, Annotation> B;
public:
    CodeNode(const HandleBase& other)
      : DoublyLinkedListMember(to<Body>(other)) { }
    CodeNode(const CodeNode& other) : DoublyLinkedListMember(other) { }

    // Unlike Handle/ConstHandle, CodeNode equality is just pointer equality.
    bool operator==(CodeNode other) const { return body() == other.body(); }
    bool operator!=(CodeNode other) const { return body() != other.body(); }

    CodeWalker::Result walk(CodeWalker* walker)
    {
        return body()->walk(walker);
    }

protected:
    class Body : public Annotation::Body
    {
    protected:
        Body() { }
        CodeNode codeNode() { return handle<CodeNode>(); }
        virtual CodeWalker::Result walk(CodeWalker* walker)
        {
            return walker->visit(codeNode());
        }
    private:
        friend class Code;
        friend class CodeNode;
    };
    Body* body() const { return as<Body>(); }

private:
    friend class Code;
    friend class Statement;
};

class Code : public ReferenceCounted<DoublyLinkedList<CodeNode, Annotation>>
{
private:
    typedef ReferenceCounted<DoublyLinkedList<CodeNode, Annotation>> Base;
public:

    Code() : Base(Base::create<Body>()) { }
protected:
    class Body : public Base::Body
    {
    protected:
        CodeWalker::Result walk(CodeWalker* walker)
        {
            CodeNode n = codeNode().next();
            while (!n.isSentinel()) {
                auto r = n.walk(walker);
                if (r == CodeWalker::Result::abort)
                    return r;
                CodeNode nn = n.next();
                if (r == CodeWalker::Result::remove)
                    n.remove();
                n = nn;
            }
            return Base::Body::walk(walker);
        }
    };
    Body* body() const { return as<Body>(); }
    //void reset() const { body()->release(); }
//private:
//    friend class Code;
};

// Normally, an Annotation represents a point in the program, so one would 
// expect it to be associated with a Location rather than a Span. However,
// there is typically whitespace separating adjacent Statements, so the end of
// one Statement does not generally have the same Location as the beginning of
// the next one. Thus source locations are represented by a SpanAnnotation on
// a Statement marking the beginning and end Locations of that Statement.
// 
// TODO: Possibly in the future change this so that the Annotation point holds
// the end Location of the previous statement and the start Location of the
// annotated statement?
//// How would this interact with the empty Code in a
//// LabelAnnotation? Presumably we'd need a NilStatement in the empty Code to
//// hold the start Location of the Label. The source locations would then not be
//// in order when traversing the Code. Does that matter?
class SpanAnnotation : public Annotation
{
public:
    SpanAnnotation(const Annotation& annotation)
      : Annotation(to<Body>(annotation)) { }
    Span span() const { return body()->span(); }
private:
    class Body : public Annotation::Body
    {
    public:
        Body(Span span) : _span(span) { }
        Span span() { return _span; }
    private:
        Span _span;
    };
    Body* body() const { return as<Body>(); }

    template<class T> friend class AnnotationT;
};

//class LabelAnnotation : public Annotation
//{
//public:
//    LabelAnnotation(const Annotation& annotation)
//      : Annotation(to<Body>(annotation)) { }
//    Identifier identifier() const { return body()->_identifier; }
//private:
//    class Body : public Annotation::Body
//    {
//    public:
//        Body(const Identifier& identifier) : _identifier(identifier) { }
//        Body(const Identifier& identifier, const Span& span)
//          : _identifier(identifier)
//        {
//            _source.annotate<SpanAnnotation>(span);
//        }
//    private:
//        Identifier _identifier;
//
//        // A Label is an Annotation rather than a Statement as it does not
//        // conceptually progress the state of the program, just marks a point
//        // in it. However, it can have a Span associated with it as it has a
//        // textual representation in the program's source. So as not to burden
//        // every LabelAnnotation (or every Annotation) with the weight of a
//        // Span, we have a Code here to represent that piece of source code.
//        Code _source;
//
//        friend class LabelAnnotation;
//    };
//    Body* body() const { return as<Body>(); }
//
//    friend class Annotation;
//};

class Statement : public CodeNode
{
public:
    Statement(CodeNode code) : CodeNode(to<Body>(code)) { }
    Span span() const
    {
        if (!hasAnnotation<SpanAnnotation>())
            return Span();
        return getAnnotation<SpanAnnotation>().span();
    }
protected:
    class Body : public CodeNode::Body
    {
    public:
        Body(Span span) { statement().annotate<SpanAnnotation>(span); }
        Statement statement() { return handle<Statement>(); }
    private:

        friend class Statement;
    };
    //Statement(Body* body) : CodeNode(body) { }
private:
    Body* body() const { return as<Body>(); }
};

class LabelStatement : public Statement
{
public:
    VariableDefinition variableDefinition()
    {
        return body()->_variableDefinition;
    }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Identifier& identifier, const Span& span)
          : Statement::Body(span), _variableDefinition(identifier) { }
    private:
        VariableDefinition _variableDefinition;

        friend class LabelStatement;
    };

    Body* body() const { return as<Body>(); }
};

class ExpressionStatement : public Statement
{
public:
    Expression expression() const { return body()->expression(); }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        Expression expression() { return _expression; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse)
                return _expression.walk(walker);
            return r;
        }
    private:
        Expression _expression;
    };
    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class FuncoDefinitionStatement : public Statement
{
public:
    FuncoDefinitionStatement(const Handle& other)
      : Statement(to<Body>(other)) { }
    TycoSpecifier returnTypeSpecifier()
    {
        return body()->_returnTypeSpecifier;
    }
    Identifier name() { return body()->_name; }
    TemplateParameters templateParameters()
    {
        return body()->_templateParameters;
    }
    Code parameters() { return body()->_parameters; }
protected:
    class Body : public Statement::Body
    {
    public:
        Body(const TycoSpecifier& returnTypeSpecifier, const Identifier& name,
            TemplateParameters templateParameters, Code parameters,
            const Span& span)
          : Statement::Body(span),
            _returnTypeSpecifier(returnTypeSpecifier), _name(name),
            _templateParameters(templateParameters), _parameters(parameters)
        { }
    protected:
        CodeWalker::Result walk(CodeWalker* walker)
        {
            if (_returnTypeSpecifier.walk(walker) == CodeWalker::Result::abort)
                return CodeWalker::Result::abort;
            if (_name.walk(walker) == CodeWalker::Result::abort)
                return CodeWalker::Result::abort;
            if (_templateParameters.walk(walker) == CodeWalker::Result::abort)
                return CodeWalker::Result::abort;
            return _parameters.walk(walker);
        }
    private:
        TycoSpecifier _returnTypeSpecifier;
        Identifier _name;
        Code _parameters;
        TemplateParameters _templateParameters;

        friend class FuncoDefinitionStatement;
    };
private:
    Body* body() { return as<Body>(); }
};

class FuncoDefinitionCodeStatement : public FuncoDefinitionStatement
{
public:
    FuncoDefinitionCodeStatement(Annotation a)
      : FuncoDefinitionStatement(to<Body>(a)) { }
    Code code() const { return body()->_body; }
private:
    class Body : public FuncoDefinitionStatement::Body
    {
    public:
        Body(const TycoSpecifier& returnTypeSpecifier, const Identifier& name,
            TemplateParameters templateParameters, Code parameters, Code body,
            const Span& span)
          : FuncoDefinitionStatement::Body(returnTypeSpecifier, name,
              templateParameters, parameters, span),
            _body(body) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (FuncoDefinitionStatement::Body::walk(walker) ==
                    CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_body.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Code _body;

        friend class FuncoDefinitionCodeStatement;
    };
    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class FunctionDefinitionFromStatement : public FuncoDefinitionStatement
{
public:
    FunctionDefinitionFromStatement(Annotation a)
      : FuncoDefinitionStatement(to<Body>(a)) { }
    Expression from() const { return body()->_from; }
private:
    class Body : public FuncoDefinitionStatement::Body
    {
    public:
        Body(const TycoSpecifier& returnTypeSpecifier, const Identifier& name,
            Code parameters, Expression from, const Span& span)
            : FuncoDefinitionStatement::Body(returnTypeSpecifier, name,
                TemplateParameters(), parameters, span),
            _from(from) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (FuncoDefinitionStatement::Body::walk(walker) ==
                    CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_from.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _from;

        friend class FunctionDefinitionFromStatement;
    };
    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

// An external function definition is one that comes from the code invoking the
// ALFE compiler/interpreter rather than the ALFE code being
// compiled/interpreted.
class ExternalFuncoDefinitionStatement : public FuncoDefinitionStatement
{
public:
    ExternalFuncoDefinitionStatement(Annotation a)
      : FuncoDefinitionStatement(to<Body>(a)) { }
private:
    class Body : public FuncoDefinitionStatement::Body
    {
    public:
        Body(const Funco& funco)
            : FuncoDefinitionStatement::Body(returnTypeSpecifier, name,
                templateParameters, parameters, span),
            _body(body) { }

    };
    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

// TycoDefinitionStatement := TycoSignifier "=" TycoSpecifier ";"
class TycoDefinitionStatement : public Statement
{
public:
    TycoDefinitionStatement(Annotation a) : Statement(to<Body>(a)) { }
    TycoSignifier tycoSignifier() const { return body()->_tycoSignifier; }
    TycoSpecifier tycoSpecifier() const { return body()->_tycoSpecifier; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const TycoSignifier& tycoSignifier,
            const TycoSpecifier& tycoSpecifier, const Span& span)
          : Statement::Body(span), _tycoSignifier(tycoSignifier),
            _tycoSpecifier(tycoSpecifier) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_tycoSignifier.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_tycoSpecifier.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        TycoSignifier _tycoSignifier;
        TycoSpecifier _tycoSpecifier;

        friend class TycoDefinitionStatement;
    };
    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

// ConditionalStatement = (`if` | `unless`) ConditionedStatement
//   ((`elseIf` | `elseUnless`) ConditionedStatement)* [`else` Statement];
// ConditionedStatement = "(" Expression ")" Statement;
class ConditionalStatement : public Statement
{
public:
    ConditionalStatement(Annotation other) : Statement(to<Body>(other)) { }
    Expression condition() const { return body()->_condition; }
    Code trueStatement() const { return body()->_trueStatement; }
    Code falseStatement() const { return body()->_falseStatement; }
    void setCondition(Expression condition) { body()->_condition = condition; }
    void setTrueStatement(Code t) { body()->_trueStatement = t; }
    void setFalseStatement(Code f) { body()->_falseStatement = f; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& condition, Code trueStatement,
            Code falseStatement, const Span& span = Span())
          : Statement::Body(span), _condition(condition),
            _trueStatement(trueStatement), _falseStatement(falseStatement) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_condition.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_trueStatement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_falseStatement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _condition;
        Code _trueStatement;
        Code _falseStatement;

        friend class ConditionalStatement;
    };
    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class SwitchStatement : public Statement
{
public:
    class Case : public ParseTreeObject
    {
    public:
        Case(ParseTreeObject other) : ParseTreeObject(to<Body>(other)) { }
        bool isDefault() { return body()->isDefault(); }
        Code code() { return body()->_code; }
        List<Expression> expressions()
        {
            ValueBody* v = as<ValueBody>();
            if (v == 0)
                return List<Expression>();
            return v->_expressions;
        }
        Case() { }
        Case(Code code, const Span& span)
          : ParseTreeObject(create<DefaultBody>(code, span)) { }
        Case(const List<Expression>& expressions, Code code, const Span& span)
          : ParseTreeObject(create<ValueBody>(expressions, code, span)) { }

        class Body : public ParseTreeObject::Body
        {
        public:
            Body(Code code, const Span& span)
              : ParseTreeObject::Body(span), _code(code) { }
            virtual bool isDefault() = 0;
            CodeWalker::Result walk(CodeWalker* walker)
            {
                CodeWalker::Result r = walker->visit(parseTreeObject());
                if (r == CodeWalker::Result::recurse) {
                    if (_code.walk(walker) == CodeWalker::Result::abort)
                        return CodeWalker::Result::abort;
                }
                return r;
            }
        protected:
            Code _code;

            friend class Case;
        };
    private:

        Body* body() { return as<Body>(); }

        class DefaultBody : public Body
        {
        public:
            DefaultBody(Code code, const Span& span) : Body(code, span) { }
            bool isDefault() { return true; }
        };
        class ValueBody : public Body
        {
        public:
            ValueBody(const List<Expression>& expressions, Code code,
                const Span& span)
              : Body(code, span), _expressions(expressions) { }
            bool isDefault() { return false; }
            CodeWalker::Result walk(CodeWalker* walker)
            {
                CodeWalker::Result r = walker->visit(parseTreeObject());
                if (r == CodeWalker::Result::recurse) {
                    for (auto e : _expressions) {
                        if (e.walk(walker) == CodeWalker::Result::abort)
                            return CodeWalker::Result::abort;
                    }
                    if (_code.walk(walker) == CodeWalker::Result::abort)
                        return CodeWalker::Result::abort;
                }
                return r;
            }
        private:
            List<Expression> _expressions;

            friend class Case;
        };
    };

    Expression expression() { return body()->_expression; }
    Case defaultCase() { return body()->_defaultCase; }
    List<Case> cases() { return body()->_cases; }
    //bool walkCases(std::function<bool(Case)>& function)
    //{
    //    for (auto c : cases()) {
    //        if (!function(c))
    //            return false;
    //    }
    //    return function(defaultCase());
    //}

private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Case& defaultCase,
            const List<Case>& cases, const Span& span)
          : Statement::Body(span), _expression(expression),
            _defaultCase(defaultCase), _cases(cases) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_expression.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                for (auto c : _cases) {
                    if (c.walk(walker) == CodeWalker::Result::abort)
                        return CodeWalker::Result::abort;
                }
                if (_defaultCase.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _expression;
        Case _defaultCase;
        List<Case> _cases;

        friend class SwitchStatement;
    };

    Body* body() { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class ReturnStatement : public Statement
{
public:
    Expression expression() const { return body()->_expression; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
          : Statement::Body(span), _expression(expression) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_expression.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _expression;

        friend class ReturnStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class BreakOrContinueStatement : public Statement
{
public:
    BreakOrContinueStatement(Annotation other) : Statement(to<Body>(other)) { }
    int breakCount() const { return body()->_breakCount; }
    bool hasContinue() const { return body()->_hasContinue; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Span& span, int breakCount, bool hasContinue)
          : Statement::Body(span), _breakCount(breakCount),
            _hasContinue(hasContinue)
        { }
    private:
        int _breakCount;
        bool _hasContinue;

        friend class BreakOrContinueStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class ForeverStatement : public Statement
{
public:
    ForeverStatement(Annotation other) : Statement(to<Body>(other)) { }
    Code code() const { return body()->_code; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(Code code, const Span& span) : Statement::Body(span), _code(code)
        { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_code.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Code _code;

        friend class ForeverStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class WhileStatement : public Statement
{
public:
    WhileStatement(Annotation other) : Statement(to<Body>(other)) { }
    Code doStatement() const { return body()->_doStatement; }
    Expression condition() const { return body()->_condition; }
    Code statement() const { return body()->_statement; }
    Code doneStatement() const { return body()->_doneStatement; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(Code doStatement, const Expression& condition,
            Code statement, Code doneStatement,
            const Span& span)
          : Statement::Body(span), _doStatement(doStatement),
            _condition(condition), _statement(statement),
            _doneStatement(doneStatement) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_doStatement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_condition.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_statement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_doneStatement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Code _doStatement;
        Expression _condition;
        Code _statement;
        Code _doneStatement;

        friend class WhileStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class ForStatement : public Statement
{
public:
    ForStatement(Annotation other) : Statement(to<Body>(other)) { }
    Code preStatement() const { return body()->_preStatement; }
    Expression condition() const { return body()->_condition; }
    Code postStatement() const { return body()->_postStatement; }
    Code statement() const { return body()->_statement; }
    Code doneStatement() const { return body()->_doneStatement; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(Code preStatement, const Expression& condition,
            Code postStatement, Code statement,
            Code doneStatement, const Span& span)
          : Statement::Body(span), _preStatement(preStatement),
            _condition(condition), _postStatement(postStatement),
            _statement(statement), _doneStatement(doneStatement) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_preStatement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_condition.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_postStatement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_statement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_doneStatement.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Code _preStatement;
        Expression _condition;
        Code _postStatement;
        Code _statement;
        Code _doneStatement;

        friend class ForStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class GotoStatement : public Statement
{
public:
    Expression expression() const { return body()->_expression; }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const Expression& expression, const Span& span)
            : Statement::Body(span), _expression(expression) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_expression.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _expression;

        friend class GotoStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

class VariableDefinitionStatement : public Statement
{
public:
    VariableDefinitionStatement(Annotation other)
      : Statement(to<Body>(other)) { }
    VariableDefinition variableDefinition()
    {
        return body()->_variableDefinition;
    }
private:
    class Body : public Statement::Body
    {
    public:
        Body(const VariableDefinition& variableDefinition, const Span& span)
          : Statement::Body(span), _variableDefinition(variableDefinition) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                if (_variableDefinition.walk(walker) ==
                    CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        VariableDefinition _variableDefinition;

        friend class VariableDefinitionStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

template<class T> class AccessSpecifierT;
typedef AccessSpecifierT<void> AccessSpecifier;

template<class T> class AccessTycoSpecifierT;
typedef AccessTycoSpecifierT<void> AccessTycoSpecifier;

template<class T> class AccessSpecifierT : public ParseTreeObject
{
public:
    AccessSpecifierT(Handle code) : ParseTreeObject(to<Body>(code))
    { }
    static AccessSpecifier parseOrFail(CharacterSource* source)
    {
        AccessTycoSpecifier ats = AccessTycoSpecifier::parse(source);
        if (ats.valid()) {
            if (Space::parseCharacter(source, '.'))
                return FunctionAccessSpecifier::parseOrFail(ats, source);
            return ats;
        }
        return parseFunctionOrFail(ats, source);
    }
protected:
    class Body : public ParseTreeObject::Body
    {
    public:
        Body(Span span) : ParseTreeObject::Body(span) { }
    private:
    };

    Body* body() const { return as<Body>(); }
};

template<class T> class AccessTycoSpecifierT : public AccessSpecifier
{
public:
    AccessTycoSpecifier parse(CharacterSource* source)
    {
        CharacterSource* s2 = source;
        do {
            Span span;
            AccessTycoSpecifier ats;
            if (Space::parseCharacter(s2, ':', &span)) {
                AccessTycoSpecifier parent = parse(s2);
                ats = DerivedAccessTycoSpecifier::create<Body>(parent);
            }
            else {
                TycoIdentifier ti = TycoIdentifier::parse(s2);
                if (!ti.valid())
                    return AccessTycoSpecifier();

            }



        } while (true);
    }
private:
    class Body : public AccessSpecifier::Body
    {
    public:
    private:
    };
};

class FunctionAccessSpecifier : public AccessSpecifier
{
public:
    static AccessSpecifier parseOrFail(AccessTycoSpecifier ats,
        CharacterSource* source)
    {
        Identifier i = Identifier::parse(source);
        if (!i.valid())
            source->location().throwError("Identifier expected");

        Span span;
        Space::assertCharacter(source, '(', &span);

        List<Expression> list;
        if (!Space::parseCharacter(source, ')')) {
            list.add(Expression::parseOrFail(source));
            while (Space::parseCharacter(source, ',', &span))
                list.add(Expression::parseOrFail(source));
            Space::assertCharacter(source, ')', &span);
        }
        return create<Body>(ats.span() + span, ats, i, list);
    }
private:
    class Body : public AccessSpecifier::Body
    {
    public:
        Body(Span span, AccessTycoSpecifier accessTycoSpecifier,
            Identifier identifier, List<Expression> arguments)
          : AccessSpecifier::Body(span), _accessTycoSpecifier(accessTycoSpecifier),
            _identifier(identifier), _arguments(arguments)
        { }
    private:
        AccessTycoSpecifier _accessTycoSpecifier;
        Identifier _identifier;
        List<Expression> _arguments;
    };

    Body* body() const { return as<Body>(); }
};

class AccessSpecifierStatement : public Statement
{
public:
    AccessSpecifierStatement(Annotation other) : Statement(to<Body>(other)) { }
    List<AccessSpecifier> accessSpecifiers()
    {
        return body()->_accessSpecifiers;
    }
private:
    class Body : public Statement::Body
    {
    public:
        Body(List<AccessSpecifier> accessSpecifiers, const Span& span)
          : Statement::Body(span), _accessSpecifiers(accessSpecifiers) { }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(codeNode());
            if (r == CodeWalker::Result::recurse) {
                for (auto a : _accessSpecifiers) {
                    if (a.walk(walker) == CodeWalker::Result::abort)
                        return CodeWalker::Result::abort;
                }
            }
            return r;
        }
    private:
        List<AccessSpecifier> _accessSpecifiers;

        friend class AccessSpecifierStatement;
    };

    Body* body() const { return as<Body>(); }

    template<class N, class B> friend class DoublyLinkedListMember;
};

#include "alfe/parser.h"
#include "alfe/interpreter.h"

#endif // INCLUDED_CODE_H
