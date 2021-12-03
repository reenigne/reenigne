#include "alfe/code.h"

#ifndef INCLUDED_RESOLVER_H
#define INCLUDED_RESOLVER_H

template<class T> class ScopeT;
typedef ScopeT<void> Scope;

template<class T> class ScopeT
{
public:
    ScopeT(Scope* parent, bool functionScope = true)
      : _parent(parent)
    {
        if (functionScope)
            _functionScope = this;
        else
            _functionScope = parent->_functionScope;
    }
    void addType(TypeT<T> type, TycoIdentifier identifier = TycoIdentifier())
    {
        if (!identifier.valid())
            identifier = TycoIdentifier(type.toString());
        _functionScope->_tycos.add(identifier, type);
    }
    void addObject(Identifier i, VariableDefinition s)
    {
        _objects[i] = s;
    }
    void addFunction(Identifier i, FuncoT<T> f)
    {
        if (_functionScope->_functions.hasKey(i))
            _functionScope->_functions[i].add(f);
        else {
            List<Funco> l;
            l.add(f);
            _functionScope->_functions.add(i, l);
        }
    }
    //ValueT<T> valueOfIdentifier(Identifier i)
    //{
    //    Span s = i.span();
    //    if (!_objects.has(i))
    //        s.throwError("Unknown identifier " + i.toString());
    //    return Value(LValueType::wrap(getValue(i).type()), LValue(this, i), s);
    //}
    Tyco resolveTycoIdentifier(TycoIdentifier i) const
    {
        if (!_functionScope->_tycos.hasKey(i))
            return Tyco();
        return _functionScope->_tycos[i];
    }
    VariableDefinition resolveVariable(Identifier identifier,
        ResolutionPath* path)
    {
        if (_objects.hasKey(identifier)) {
            VariableDefinition s = _objects[identifier];
            *path = ResolutionPath::local();
            return s;
        }
        if (_parent == 0) {
            identifier.span().throwError("Unknown identifier " +
                identifier.toString());
        }
        return _parent->resolveVariable(identifier, path);
    }
    FuncoT<T> resolveFunction(Identifier identifier,
        List<Expression> arguments)  // arguments just used for their types
    {
        List<Type> argumentTypes;
        for (auto e : arguments)
            argumentTypes.add(e.type());

        List<List<FuncoT<T>>> funcos = getFuncosForIdentifier(identifier);

        List<FuncoT<T>> bestCandidates;
        for (auto ff : funcos) {
            for (auto f : ff) {
                if (!f.argumentsMatch(argumentTypes))
                    continue;
                List<FuncoT<T>> newBestCandidates;
                bool newBest = true;
                for (auto b : bestCandidates) {
                    int r = f.compareTo(b);
                    if (r == 2) {
                        // b better than f
                        newBest = false;
                        break;
                    }
                    if (r != 1)
                        newBestCandidates.add(b);
                }
                if (newBest) {
                    bestCandidates = newBestCandidates;
                    bestCandidates.add(f);
                }
            }
        }
        for (auto f : bestCandidates) {
            for (auto b : bestCandidates) {
                int r = f.compareTo(b);
                if (r == 3) {
                    identifier.span().throwError(
                        "Ambiguous function call of " + identifier.toString() +
                        " with argument types " +
                        argumentTypesString(argumentTypes) + ". Could be " +
                        b.toString() + " or " + f.toString() + ".");
                }
            }
        }
        if (bestCandidates.count() == 0) {
            identifier.span().throwError("No matches for function " +
                identifier.toString() + " with argument types " +
                argumentTypesString(argumentTypes) + ".");
        }
        // We have a choice of possible funcos here. Logically they should
        // be equivalent, but some may be more optimal. For now we'll just
        // choose the first one, but later we may want to try to figure out
        // which one is most optimal.
        return *bestCandidates.begin();
    }
    //void setParentScope(Scope* parent) { _parent = parent; }
    //void setFunctionScope(Scope* scope) { _functionScope = scope; }
    //Scope* functionScope() { return _functionScope; }
private:
    String argumentTypesString(List<Type> argumentTypes) const
    {
        String s;
        bool needComma = false;
        for (auto t : argumentTypes) {
            if (needComma)
                s += ", ";
            needComma = true;
            s += t.toString();
        }
        return s;
    }

    List<List<Funco>> getFuncosForIdentifier(Identifier i)
    {
        List<List<Funco>> r;
        if (_functionScope->_parent != 0)
            r = _functionScope->_parent->getFuncosForIdentifier(i);
        if (_functionScope->_functions.hasKey(i))
            r.add(_functionScope->_functions[i]);
        return r;
    }

    HashTable<TycoIdentifier, Tyco> _tycos;
    HashTable<Identifier, VariableDefinition> _objects;
    HashTable<Identifier, List<Funco>> _functions;
    Scope* _parent;
    Scope* _functionScope;
};

class ScopeAnnotation : public Annotation
{
public:
    ScopeAnnotation(Scope* parent, bool functionScope = true)
      : Annotation(create<Body>(parent, functionScope)) { }
    ScopeAnnotation(const Annotation& annotation)
      : Annotation(to<Body>(annotation)) { }
    Scope* scope() { return body()->scope(); }
private:
    class Body : public Annotation::Body
    {
    public:
        Body(Scope* parent, bool functionScope)
          : _scope(parent, functionScope) { }
        Scope* scope() { return &_scope; }
    private:
        Scope _scope;
    };
    Body* body() { return as<Body>(); }

    friend class Annotation;
};


template<class T> class ResolutionPathT;
typedef ResolutionPathT<void> ResolutionPath;

template<class T> class ResolutionPathT : public ConstHandle
{
public:
    ResolutionPathT() { }
    static ResolutionPathT local() { return create<HereBody>(); }
    ValueT<T> evaluate(Structure* context, Identifier identifier) const
    {
        return body()->evaluate(context, identifier);
    }
private:
    ResolutionPathT(ConstHandle other) : ConstHandle(other) { }
    class Body : public ConstHandle::Body
    {
    public:
        virtual Value evaluate(Structure* context, Identifier identifier) const
            = 0;
    };
    class ParentBody : public Body
    {
    public:
        ValueT<T> evaluate(Structure* context, Identifier identifier) const
        {
            throw NotYetImplementedException();
        }
    private:
        ResolutionPathT<T> _rest;
    };
    class OuterBody : public Body
    {
    public:
        ValueT<T> evaluate(Structure* context, Identifier identifier) const
        {
            throw NotYetImplementedException();
        }
    private:
        ResolutionPathT<T> _rest;
    };
    class HereBody : public Body
    {
    public:
        ValueT<T> evaluate(Structure* context, Identifier identifier) const
        {
            Span s = identifier.span();
            //if (!has(i))
            //    s.throwError("Unknown identifier " + i.name());
            return Value(
                LValueTypeT<T>::wrap(context->getValue(identifier).type()),
                LValue(context, identifier), s);
            //return context->getValue(identifier);
        }
    };
    const Body* body() const { return as<Body>(); }
};

class Resolver
{
public:
    void resolve(Code code)
    {
        Scope scope;
        PopulateScopes populateScopes(&scope);
        code.walk(&populateScopes);
        Resolve resolve(&scope);
        code.walk(&resolve);
        code.annotate<CodeFormAnnotation>().setResolved(true);
    }
private:
    class PopulateScopes : public CodeWalker
    {
    public:
        PopulateScopes(Scope* scope) : _currentScope(scope) { }
        Result visit(Annotation c)
        {
            FunctionDefinitionCodeStatement fdcs = c;
            if (fdcs.valid()) {
                createScope(fdcs); // For instantiations
                fdcs.returnTypeSpecifier().walk(this);
                Code code = fdcs.code();
                Scope* inner = createScope(code);
                Scope* outer = _currentScope;
                _currentScope = inner;
                visitParameters(fdcs.parameters());
                populateInner(code, _currentScope);
                _currentScope = outer;
                _currentScope->addFunction(fdcs.name(), fdcs);
                return Result::advance;
            }
            FunctionDefinitionFromStatement fdfs = c;
            if (fdfs.valid()) {
                createScope(fdfs); // Can't instantiate FDFS yet
                fdfs.returnTypeSpecifier().walk(this);
                Code parameters = fdfs.parameters();
                Scope* outer = _currentScope;
                visitParameters(fdfs.parameters());
                _currentScope = outer;
                _currentScope->addFunction(fdfs.name(), fdfs);
                fsfs.from().walk(this);
                return Result::advance;
            }
            TycoDefinitionStatement tds = c;
            if (tds.valid()) {
                createScope(tds); // For instantiations
                TycoSignifier signifier = tds.tycoSignifier();
                signifier.walk(this);
                TycoSpecifier specifier = tds.tycoSpecifier();
                specifier.walk(this);
                _currentScope->addType(specifier.tyco(),
                    signifier.tycoIdentifier());
                return Result::advance;
            }
            ConditionalStatement cs = c;
            if (cs.valid()) {
                Scope* patternScope = createScope(cs);
                Code trueStatement = cs.trueStatement();
                // Any variables created in the condition are accessible from
                // the trueStatement's scope but not the falseStatement's.
                populateInner(trueStatement,
                    createScope(trueStatement, patternScope));
                PopulateScopes p(patternScope);
                cs.condition().walk(&p);
                populateInner(cs.falseStatement());
                return Result::advance;
            }
            ForeverStatement fes = c;
            if (fes.valid()) {
                populateInner(fes.code());
                return Result::advance;
            }
            WhileStatement ws = c;
            if (ws.valid()) {
                populateInner(ws.doStatement());
                Scope* patternScope = createScope(ws);
                Code statement = ws.statement();
                populateInner(statement, createScope(statement, patternScope));
                PopulateScopes p(patternScope);
                ws.condition().walk(&p);
                populateInner(ws.doneStatement());
                return Result::advance;
            }
            ForStatement fs = c;
            if (fs.valid()) {
                populateInner(fs.preStatement());
                populateInner(fs.postStatement());
                Scope* patternScope = createScope(fs);
                Code statement = fs.statement();
                populateInner(statement, createScope(statement, patternScope));
                PopulateScopes p(patternScope);
                fs.condition().walk(&p);
                populateInner(fs.doneStatement());
                return Result::advance;
            }
            VariableDefinitionStatement vds = c;
            if (vds.valid()) {
                _currentScope = createScope(vds, _currentScope, false);
                vds.variableDefinition().walk(this);
                return Result::advance;
            }
            return Result::recurse;
        }
        Result visit(ParseTreeObject o)
        {
            SwitchStatement::Case ssc = o;
            if (ssc.valid()) {
                // TODO: all the expressions in one Case need to expose the
                // same set of variable definitions.
                PopulateScopes p(populateInner(ssc.code()));
                for (auto e : ssc.expressions())
                    e.walk(&p);
                return Result::advance;
            }
            VariableDefinition vd = o;
            if (vd.valid()) {
                vd.tycoSpecifier().walk(this);
                vd.initializer().walk(this);
                _currentScope->addObject(vd.identifier(), vd);
            }
            ClassTycoSpecifier cts = o;
            if (cts.valid()) {
                populateInner(cts.contents());
                return Result::advance;
            }
            return Result::recurse;
        }
        Result visit(Tyco t)
        {
            return Result::recurse;
        }
    private:
        Scope* _currentScope = 0;

        Scope* createScope(CodeNode code, Scope* parent = 0,
            bool functionScope = true)
        {
            if (parent == 0)
                parent = _currentScope;
            return
                code.annotate<ScopeAnnotation>(parent, functionScope).scope();
        }
        Scope* populateInner(CodeNode code, Scope* scope = 0)
        {
            if (scope == 0)
                scope = createScope(code);
            PopulateScope p(scope);
            code.walk(&p);
            return scope;
        }
        void visitParameters(Code parameters)
        {
            for (auto p : parameters) {
                VariableDefinitionStatement vds(p);
                assert(vds.valid(), "Only VariableDefinitionStatements "
                    "are allowed in the parameters part of a function.");
                _currentScope = createScope(vds, _currentScope, false);
                vds.variableDefinition().walk(this);
            }
        }
    };
    class Resolve : public CodeWalker
    {
    public:
        Resolve(Scope* scope) : _currentScope(scope) { }
        Result visit(Annotation c)
        {
            FunctionDefinitionCodeStatement fdcs = c;
            if (fdcs.valid()) {
                fdcs.returnTypeSpecifier().walk(this);
                Code code = fdcs.code();
                Scope* inner = getScope(code);
                Scope* outer = _currentScope;
                _currentScope = inner;
                visitParameters(fdcs.parameters());
                code.walk(this);
                _currentScope = outer;
                return Result::advance;
            }
            FunctionDefinitionFromStatement fdfs = c;
            if (fdfs.valid()) {
                fdfs.returnTypeSpecifier().walk(this);
                Scope* outer = _currentScope;
                visitParameters(fdfs.parameters());
                _currentScope = outer;
                fsfs.from().walk(this);
                return Result::advance;
            }
            ConditionalStatement cs = c;
            if (cs.valid()) {
                // TODO: If cs.condition() is an equality expression and one
                // side is a pattern then the other side should be resolved in
                // the outer scope (_currentScope) rather than the getScope(cs)
                // scope. That way, patterns can override the compared object.
                Resolve r(getScope(cs));
                Expression c = cs.condition();
                c.walk(&r);
                checkBoolean(c);
                resolveInner(cs.trueStatement());
                cs.falseStatement().walk(this);
                return Result::advance;
            }
            ForeverStatement fes = c;
            if (fes.valid()) {
                resolveInner(fes.code());
                return Result::advance;
            }
            WhileStatement ws = c;
            if (ws.valid()) {
                resolveInner(ws.doStatement());
                Resolve r(getScope(ws));
                Expression c = ws.condition();
                c.walk(&r);
                checkBoolean(c);
                resolveInner(ws.statement());
                ws.doneStatement().walk(this);
                return Result::advance;
            }
            ForStatement fs = c;
            if (fs.valid()) {
                resolveInner(fs.preStatement());
                resolveInner(fs.postStatement());
                Resolve r(getScope(fs));
                Expression c = fs.condition();
                c.walk(&r);
                checkBoolean(c);
                resolveInner(fs.statement());
                fs.doneStatement().walk(this);
                return Result::advance;
            }
            VariableDefinitionStatement vds = c;
            if (vds.valid()) {
                vds.variableDefinition().walk(this);
                _currentScope = getScope(vds);
                return Result::advance;
            }
            return Result::recurse;
        }
        Result visit(ParseTreeObject o)
        {
            DotExpression de = o;
            if (de.valid()) {
                Expression l = de.left();
                l.walk(this);
                StructuredType t = l.type().rValue();
                if (!t.valid())
                    l.span().throwError("Expression has no members");
                Resolve r(t.scope());
                de.right().walk(&r);
                return Result::advance;
            }
            Identifier i = o;
            if (i.valid()) {
                ResolutionPath p;
                i.setDefinition(_currentScope->resolveVariable(i, &p));
                i.setResolutionPath(p);
                return Result::advance;
            }
            FunctionCallExpression fce = o;
            if (fce.valid()) {
                List<Expression> arguments = fce.arguments();
                for (Expression a : arguments)
                    a.walk(this);
                Expression function = fce.function();
                Identifier i = function;
                if (!i.valid()) {
                    function.walk(this);
                    return;
                }
                fce.setResolvedFunco(
                    _currentScope->resolveFunction(i, arguments));
                return Result::advance;
            }
            ConstructorCallExpression cce = o;
            if (cce.valid()) {
                List<Expression> arguments = cce.arguments();
                for (Expression a : arguments)
                    a.walk(this);
                cce.setType(_currentScope->resolveType(cce.tycoSpecifier()));
                return Result::advance;
            }
            VariableDefinition vd = o;
            if (vd.valid()) {
                TycoSpecifier ts = vd.tycoSpecifier();
                ts.walk(this);
                vd.initializer().walk(this);
                Tyco tyco = ts.tyco();
                Type t = tyco;
                if (!t.valid()) {
                    vd.span().throwError("Type constructor specifier does "
                        "not specify a type");
                }
                vd.setType(t);
                return Result::advance;
            }
            TycoIdentifier ti = o;
            if (ti.valid()) {
                ti.setTyco(_currentScope->resolveTycoIdentifier(ti));
                return Result::advance;
            }
            PointerTypeSpecifier pts = o;
            if (pts.valid()) {
                TycoSpecifier ts = pts.referent();
                ts.walk(this);
                Tyco referent = ts.tyco();
                pts.setTyco(PointerType(referent));
                return Result::advance;
            }
            FunctionTypeSpecifier fts = o;
            if (fts.valid()) {
                TycoSpecifier r = fts.returnType();
                r.walk(this);
                Type ret = resolveType(r);
                FunctionType f = FunctionTemplate().instantiate(ret);
                List<TycoSpecifier> argumentTypes = fts.argumentTypes();
                for (auto a : argumentTypes) {
                    a.walk(this);
                    f = f.instantiate(resolveType(a));
                }
                fts.setTyco(f);
                return Result::advance;
            }
            ClassTycoSpecifier cts = o;
            if (cts.valid()) {
                resolveInner(cts.contents());
                return Result::advance;
            }
            TypeOfTypeSpecifier tots = o;
            if (tots.valid()) {
                Expression e = tots.expression();
                e.walk(this);
                tots.setTyco(e.type());
                return Result::advance;
            }
            InstantiationTycoSpecifier its = o;
            if (its.valid()) {
                TycoIdentifier ti = its.tycoIdentifier();
                ti.walk(this);
                Tyco t = ti.tyco();
                TemplateArguments ta = its.templateArguments();
                ta.walk(this);
                List<TycoSpecifier> arguments = ta.arguments();
                for (auto argument : arguments) {
                    Template te = t;
                    assert(te.valid());
                    t = te.instantiate(argument.tyco());
                }
                its.setTyco(t);
                return Result::advance;
            }
            BinaryExpression be = o;
            if (be.valid()) {
                Expression l = be.left();
                l.walk(this);
                checkBoolean(l);
                Expression r = be.right();
                r.walk(this);
                checkBoolean(r);
                return Result::advance;
            }
            ConditionalExpression ce = o;
            if (ce.valid()) {
                Expression c = ce.condition();
                c.walk(this);
                checkBoolean(c);
                Expression t = ce.trueExpression();
                t.walk(this);
                Type tt = t.type();
                Expression f = ce.falseExpression();
                f.walk(this);
                Type ft = f.type();
                // TODO: Make the ConditionalExpression's type the supertype of
                // the true and false types?
                if (tt != ft) {
                    ce.span().throwError("Type mismatch in conditional "
                        "expression: true type is " + tt.toString() + ", false"
                        " type is " + ft.toString() + ".");
                }
                ce.setType(tt);
            }
            return Result::recurse;
        }
        Result visit(Tyco t) { return Result::recurse; }
    private:
        Scope* _currentScope = 0;

        Scope* getScope(CodeNode code)
        {
            return code.getAnnotation<ScopeAnnotation>().scope();
        }
        void resolveInner(CodeNode code, Scope* scope = 0)
        {
            if (scope == 0)
                scope = getScope(code);
            Resolve r(scope);
            code.walk(&r);
        }
        void visitParameters(Code parameters)
        {
            for (auto p : parameters) {
                VariableDefinitionStatement vds(p);
                assert(vds.valid(), "Only VariableDefinitionStatements "
                    "are allowed in the parameters part of a function.");
                vds.variableDefinition().walk(this);
                _currentScope = getScope(vds);
            }
        }
        Type resolveType(TycoSpecifier s) const
        {
            Tyco tyco = s.tyco();
            Type t = tyco;
            if (t.valid())
                return t;
            s.span().throwError("Type constructor specifier " + s.toString() +
                " does not specify a type but a type constructor of kind " +
                tyco.kind().toString() + ".");
        }
        void checkBoolean(Expression e)
        {
            Type t = e.type();
            if (t != BooleanType()) {
                e.span().throwError("Expression has type " + t.toString() +
                    " but an expression of type Boolean is required.");
            }
        }

    };
};

#endif // INCLUDED_RESOLVER_H