#include "unity/string.h"
#include "unity/character_source.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hash_table.h"

#ifdef _WIN32
#include "shellapi.h"
#endif

#include <stdio.h>
#include <stdlib.h>

class CommandLine
{
public:
#ifdef _WIN32
    CommandLine()
    {
        WindowsCommandLine windowsCommandLine;
        int nArgs = windowsCommandLine.nArgs();
        const LPWSTR* szArglist = windowsCommandLine.arguments();
        _arguments.allocate(nArgs);
        int nBytes = 0;
        for (int i = 0; i < nArgs; ++i)
            nBytes += String::countBytes(szArglist[i]);
        Reference<OwningBufferImplementation> bufferImplementation = new OwningBufferImplementation;
        bufferImplementation->allocate(nBytes);
        Buffer buffer(bufferImplementation);
        UInt8* p = bufferImplementation->data();
        int s = 0;
        for (int i = 0; i < nArgs; ++i) {
            UInt8* p2 = String::addToBuffer(szArglist[i], p);
            int n = p2 - p;
            p = p2;
            _arguments[i] = String(buffer, s, n);
            s += n;
        }
        _nArguments = nArgs;
    }
#else
    CommandLine(int argc, char** argv)
    {
        for (int i = 0; i < argc; ++i) {
            _arguments[i] = String(argv[i]);
    }
#endif
    String argument(int i) const { return _arguments[i]; }
    int arguments() const { return _nArguments; }
private:
#ifdef _WIN32
    class WindowsCommandLine
    {
    public:
        WindowsCommandLine()
        {
            _szArglist = CommandLineToArgvW(GetCommandLineW(), &_nArgs);
            if (_szArglist == NULL) {
                static String parsingCommandLine("Parsing command line");
                Exception::throwSystemError(parsingCommandLine);
            }
        }
        ~WindowsCommandLine()
        {
            LocalFree(static_cast<HLOCAL>(_szArglist));
        }
        const LPWSTR* arguments() const { return _szArglist; }
        int nArgs() const { return _nArgs; }
    private:
        LPWSTR* _szArglist;
        int _nArgs;
    };
#endif
    Array<String> _arguments;
    int _nArguments;
};

#include "Type.cpp"

class Variable;

class Value
{
public:
    Value() { }
    Value(int intValue) : _intValue(intValue) { }
    Value(String stringValue) : _stringValue(stringValue) { }
    Value(Variable* pointerValue) : _pointerValue(pointerValue) { }
    int getInt() const { return _intValue; }
    String getString() const { return _stringValue; }
    Variable* getPointer() const { return _pointerValue; }
private:
    int _intValue;
    String _stringValue;
    Variable* _pointerValue;
};

class Symbol : public ReferenceCounted
{
public:
    virtual Type type(String name, DiagnosticLocation location) const = 0;
    virtual Value value() const = 0;
};

class FunctionDeclarationStatement;

template<class T> class FunctionNameTemplate : public Symbol
{
public:
    void addOverload(TypeList argumentTypes, FunctionDeclarationStatement* function, DiagnosticLocation location)
    {
        if (_overloads.hasKey(argumentTypes)) {
            static String error("This overload has already been defined.");
            location.throwError(error);
        }
        _overloads.add(argumentTypes, function);
        if (_overloads.count() == 1)
            _functionType = FunctionType(function->returnType(), argumentTypes);
    }
    bool hasOverload(TypeList argumentTypes)
    {
        return _overloads.hasKey(argumentTypes);
    }
    FunctionDeclarationStatement* lookUpOverload(TypeList argumentTypes)
    {
        return _overloads.lookUp(argumentTypes);
    }
    Type type(String name, DiagnosticLocation location) const
    {
        if (_overloads.count() > 1) {
            static String error(" is an overloaded function - I don't know which overload you mean.");
            location.throwError(name + error);
        }
        return _functionType;
    }
    Value value() const { return Value(); }
private:
    int _overloadCount;
    HashTable<TypeList, FunctionDeclarationStatement*> _overloads;
    Type _functionType;
};

typedef FunctionNameTemplate<void> FunctionName;

class TypeDefinitionStatement;

class Variable : public Symbol
{
public:
    Variable(Type type) : _type(type) { }
    Type type(String name, DiagnosticLocation location) const { return _type; }
    Value value() const { return _value; }
    void setValue(Value value) { _value = value; }
private:
    Type _type;
    Value _value;
};

class Identifier;

template<class T> class ScopeTemplate;

typedef ScopeTemplate<void> Scope;

template<class T> class ScopeTemplate : public ReferenceCounted
{
public:
    ScopeTemplate(Scope* outer, bool functionScope = false)
      : _outer(outer)
    {
        if (functionScope)
            _functionScope = this;
        else
            _functionScope = outer->_functionScope;
    }
    Scope* outer() const { return _outer; }
    Scope* functionScope() const { return _functionScope; }
    Reference<Variable> addVariable(String name, Type type, DiagnosticLocation location)
    {
        if (_symbolTable.hasKey(name)) {
            static String error(" is already defined");
            location.throwError(name + error);
        }
        Reference<Variable> variable = new Variable(type);
        _symbolTable.add(name, variable);
        return variable;
    }
    Reference<Symbol> resolveSymbol(String name, DiagnosticLocation location)
    {
        if (!_symbolTable.hasKey(name)) {
            if (_outer != 0)
                return _outer->resolveSymbol(name, location);
            static String error("Undefined symbol ");
            location.throwError(error + name);
        }
        return _symbolTable.lookUp(name);
    }
    void addFunction(String name, TypeList argumentTypes, FunctionDeclarationStatement* function, DiagnosticLocation location)
    {
        _functionScope->doAddFunction(name, argumentTypes, function, location);
    }
    void addType(String name, TypeDefinitionStatement* type, DiagnosticLocation location)
    {
        _functionScope->doAddType(name, type, location);
    }
    FunctionDeclarationStatement* resolveFunction(Reference<Identifier> identifier, TypeList typeList, DiagnosticLocation location)
    {
        return _functionScope->doResolveFunction(identifier, typeList, location);
    }
    TypeDefinitionStatement* resolveType(String name, DiagnosticLocation location)
    {
        return _functionScope->doResolveType(name, location);
    }
private:
    void doAddFunction(String name, TypeList argumentTypes, FunctionDeclarationStatement* function, DiagnosticLocation location)
    {
        FunctionName* functionName;
        if (_symbolTable.hasKey(name)) {
            Reference<Symbol> symbol = _symbolTable.lookUp(name);
            functionName = dynamic_cast<FunctionName*>(static_cast<Symbol*>(symbol));
            if (functionName == 0) {
                static String error(" is already defined as a variable");
                location.throwError(name + error);
            }
        }
        else {
            functionName = new FunctionName;
            _symbolTable.add(name, functionName);
        }
        functionName->addOverload(argumentTypes, function, location);
    }
    void doAddType(String name, TypeDefinitionStatement* type, DiagnosticLocation location)
    {
        if (_typeTable.hasKey(name)) {
            static String error(" has already been defined.");
            location.throwError(name + error);
        }
        _typeTable.add(name, type);
    }
    FunctionDeclarationStatement* doResolveFunction(Reference<Identifier> identifier, TypeList typeList, DiagnosticLocation location)
    {
        String name = identifier->name();
        if (!_symbolTable.hasKey(name)) {
            if (_outer == 0) {
                static String error("Undefined function ");
                location.throwError(error + name);
            }
            return _outer->resolveFunction(identifier, typeList, location);
        }
        Reference<Symbol> symbol = _symbolTable.lookUp(name);
        FunctionName* functionName = dynamic_cast<FunctionName*>(static_cast<Symbol*>(symbol));
        if (functionName == 0) {
            static String error(" is not a function");
            location.throwError(name + error);
        }
        if (!functionName->hasOverload(typeList)) {
            static String error(" has no overload with argument types ");
            location.throwError(name + error + typeList.toString());
        }
        return functionName->lookUpOverload(typeList);
    }
    TypeDefinitionStatement* doResolveType(String name, DiagnosticLocation location)
    {
        if (!_typeTable.hasKey(name)) {
            if (_outer == 0) {
                static String error("Undefined type ");
                location.throwError(error + name);
            }
            return _outer->resolveType(name, location);
        }
        return _typeTable.lookUp(name);
    }
    HashTable<String, Reference<Symbol> > _symbolTable;
    HashTable<String, TypeDefinitionStatement*> _typeTable;
    Scope* _outer;
    Scope* _functionScope;
};

class Space
{
public:
    static void parse(CharacterSource* source)
    {
        do {
            CharacterSource s = *source;
            int c = s.get();
            if (c == ' ' || c == 10) {
                *source = s;
                continue;
            }
            if (parseComment(source))
                continue;
            return;
        } while (true);
    }
    static bool parseCharacter(CharacterSource* source, int character)
    {
        if (!source->parse(character))
            return false;
        parse(source);
        return true;
    }
    static void assertCharacter(CharacterSource* source, int character)
    {
        source->assert(character);
        parse(source);
    }
    static bool parseOperator(CharacterSource* source, String op)
    {
        static String empty("");
        CharacterSource s = *source;
        CharacterSource o(op, empty);
        do {
            int c = o.get();
            if (c == -1)
                break;
            if (s.get() != c)
                return false;
        } while (true);
        *source = s;
        parse(source);
        return true;
    }
private:
    static bool parseComment(CharacterSource* source)
    {
        static String endOfFile("End of file in comment");
        static String printableCharacter("printable character");
        CharacterSource s = *source;
        int c = s.get();
        if (c != '/')
            return false;
        c = s.get();
        if (c == '/') {
            do {
                *source = s;
                c = s.get();
                if (c == 10 || c == -1)
                    break;
                if (c < 0x20)
                    source->throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
            *source = s;
            return true;
        }
        if (c == '*') {
            do {
                if (parseComment(&s))
                    continue;
                *source = s;
                c = s.get();
                while (c == '*') {
                    c = s.get();
                    if (c == '/') {
                        *source = s;
                        return true;
                    }
                }
                if (c == -1)
                    source->location().throwError(endOfFile);
                if (c < 0x20 && c != 10)
                    source->throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
        }
        return false;
    }
};

#include "TypeSpecifier.cpp"
#include "Expression.cpp"
#include "Statement.cpp"

#ifdef _WIN32
int main()
#else
int main(int argc, char* argv[])
#endif
{
    BEGIN_CHECKED {
#ifdef _WIN32
        CommandLine commandLine;
#else
        CommandLine commandLine(argc, argv);
#endif
        if (commandLine.arguments() < 2) {
            static String syntax1("Syntax: ");
            static String syntax2(" <input file name>\n");
            (syntax1 + commandLine.argument(0) + syntax2).write(Handle::consoleOutput());
            exit(1);
        }
        File file(commandLine.argument(1));
        String contents = file.contents();
        Reference<Scope> scope = new Scope(0, true);

        Reference<PrintFunction> print = new PrintFunction();
        TypeList printArgumentTypes;
        printArgumentTypes.push(StringType());
        printArgumentTypes.finalize();
        Type printFunctionType = FunctionType(VoidType(), printArgumentTypes);
        scope->addFunction(String("print"), printArgumentTypes, print, DiagnosticLocation());

        CharacterSource source(contents, file.path());
        Space::parse(&source);
        Reference<StatementSequence> program = StatementSequence::parse(&source, scope);
        CharacterSource s = source;
        if (s.get() != -1) {
            static String error("Expected end of file");
            source.location().throwError(error);
        }

        Reference<StringTypeDefinitionStatement> stringTypeDefinitionStatement = new StringTypeDefinitionStatement();
        scope->addType(String("String"), stringTypeDefinitionStatement, DiagnosticLocation());
        Reference<VoidTypeDefinitionStatement> voidTypeDefinitionStatement = new VoidTypeDefinitionStatement();
        scope->addType(String("Void"), voidTypeDefinitionStatement, DiagnosticLocation());
        Reference<IntTypeDefinitionStatement> intTypeDefinitionStatement = new IntTypeDefinitionStatement();
        scope->addType(String("Int"), intTypeDefinitionStatement, DiagnosticLocation());
        Reference<BooleanTypeDefinitionStatement> booleanTypeDefinitionStatement = new BooleanTypeDefinitionStatement();
        scope->addType(String("Boolean"), booleanTypeDefinitionStatement, DiagnosticLocation());

        program->resolveTypes();
        program->compile();

        Stack<Value> stack;
        program->run(&stack);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
