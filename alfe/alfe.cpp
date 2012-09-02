#include <stdlib.h>
#include "alfe/main.h"
#include "alfe/stack.h"
#include "alfe/hash_table.h"
#include "alfe/space.h"
#include "alfe/type.h"

//enum Atom
//{
//    atomAuto,
//    atomBit,
//    atomBoolean,
//    atomByte,
//    atomCharacter,
//    atomClass,
//    atomFunction,
//    atomInt,
//    atomPointer,
//    atomString,
//    atomTycoIdentifier,
//    atomTypeOf,
//    atomUInt,
//    atomVoid,
//    atomWord,
//    atomLabel,
//    atomTemplateTycoSpecifier,
//    atomTycoSignifier,
//    atomTemplateParameter,
//    atomTycoDefinitionStatement,
//
//    atomLogicalOr,
//    atomLogicalAnd,
//    atomDot,
//
//    atomBitwiseOr,
//    atomBitwiseXor,
//    atomBitwiseAnd,
//    atomEqualTo,
//    atomNotEqualTo,
//    atomLessThanOrEqualTo,
//    atomGreaterThanOrEqualTo,
//    atomLessThan,
//    atomGreaterThan,
//    atomShiftLeft,
//    atomShiftRight,
//    atomAdd,
//    atomSubtract,
//    atomMultiply,
//    atomDivide,
//    atomModulo,
//    atomNot,
//    atomPositive,
//    atomNegative,
//    atomDereference,
//    atomAddressOf,
//    atomPower,
//    atomFunctionCall,
//
//    atomStringConstant,
//    atomIdentifier,
//    atomIntegerConstant,
//    atomLabelConstant,
//    atomTrue,
//    atomFalse,
//    atomNull,
//    atomArrayLiteral,
//
//    atomParameter,
//
//    atomExpressionStatement,
//    atomFunctionDefinitionStatement,
//    atomFromStatement,
//    atomVariableDefinitionStatement,
//    atomAssignment,
//    atomAddAssignment,
//    atomSubtractAssignment,
//    atomMultiplyAssignment,
//    atomDivideAssignment,
//    atomModuloAssignment,
//    atomShiftLeftAssignment,
//    atomShiftRightAssignment,
//    atomBitwiseAndAssignment,
//    atomBitwiseOrAssignment,
//    atomBitwiseXorAssignment,
//    atomPowerAssignment,
//    atomCompoundStatement,
//    atomTypeAliasStatement,
//    atomNothingStatement,
//    atomIncrement,
//    atomDecrement,
//    atomIfStatement,
//    atomSwitchStatement,
//    atomReturnStatement,
//    atomIncludeStatement,
//    atomBreakStatement,
//    atomContinueStatement,
//    atomForeverStatement,
//    atomWhileStatement,
//    atomUntilStatement,
//    atomForStatement,
//    atomEmit,
//    atomLabelStatement,
//    atomGotoStatement,
//
//    atomCase,
//    atomDefaultCase,
//
//    atomPrintFunction,
//    atomExit,
//
//    atomBasicBlock,
//
//    atomCall,
//    atomReturn,
//    atomGoto,
//    atomJumpIfTrue,
//    atomStackPointer,
//    atomSetStackPointer,
//    atomStore,
//    atomDuplicate,
//    atomDrop,
//
//    atomStringConcatenate,
//    atomStringEqualTo,
//    atomStringNotEqualTo,
//    atomStringLessThanOrEqualTo,
//    atomStringGreaterThanOrEqualTo,
//    atomStringLessThan,
//    atomStringGreaterThan,
//    atomStringIntegerMultiply,
//
//    atomTypeKind,
//    atomTemplateKind,
//
//    atomLast
//};
//
//String atomToString(Atom atom)
//{
//    class LookupTable
//    {
//    public:
//        LookupTable()
//        {
//            _table[atomAuto] = String("Auto");
//            _table[atomBit] = String("Bit");
//            _table[atomBoolean] = String("Boolean");
//            _table[atomByte] = String("Byte");
//            _table[atomCharacter] = String("Character");
//            _table[atomClass] = String("Class");
//            _table[atomFunction] = String("Function");                               // returnType     argumentTypes
//            _table[atomInt] = String("Int");
//            _table[atomPointer] = String("Pointer");                                 // referentType
//            _table[atomString] = String("String");
//            _table[atomTycoIdentifier] = String("TycoIdentifier"); // name
//            _table[atomTypeOf] = String("TypeOf");                                   // expression
//            _table[atomUInt] = String("UInt");
//            _table[atomVoid] = String("Void");
//            _table[atomWord] = String("Word");
//            _table[atomLabel] = String("Label");
//            _table[atomTemplateTycoSpecifier] = String("TemplateTycoSpecifier");   // name argumentTypeSpecifiers
//
//            _table[atomLogicalOr] = String("||");                                    // leftExpression rightExpression
//            _table[atomLogicalAnd] = String("&&");                                   // leftExpression rightExpression
//            _table[atomDot] = String(".");                                           // leftExpression rightExpression
//
//            _table[atomBitwiseOr] = String("|");                                     // leftExpression rightExpression
//            _table[atomBitwiseXor] = String("~");                                    // leftExpression rightExpression
//            _table[atomBitwiseAnd] = String("&");                                    // leftExpression rightExpression
//            _table[atomEqualTo] = String("==");                                      // leftExpression rightExpression
//            _table[atomNotEqualTo] = String("!=");                                   // leftExpression rightExpression
//            _table[atomLessThanOrEqualTo] = String("<=");                            // leftExpression rightExpression
//            _table[atomGreaterThanOrEqualTo] = String(">=");                         // leftExpression rightExpression
//            _table[atomLessThan] = String("<");                                      // leftExpression rightExpression
//            _table[atomGreaterThan] = String(">");                                   // leftExpression rightExpression
//            _table[atomShiftLeft] = String("<<");                                    // leftExpression rightExpression
//            _table[atomShiftRight] = String(">>");                                   // leftExpression rightExpression
//            _table[atomAdd] = String("+");                                           // leftExpression rightExpression
//            _table[atomSubtract] = String("-");                                      // leftExpression rightExpression
//            _table[atomMultiply] = String("*");                                      // leftExpression rightExpression
//            _table[atomDivide] = String("/");                                        // leftExpression rightExpression
//            _table[atomModulo] = String("%");                                        // leftExpression rightExpression
//            _table[atomNot] = String("!");                                           // expression
//            _table[atomPositive] = String("u+");                                     // expression
//            _table[atomNegative] = String("u-");                                     // expression
//            _table[atomDereference] = String("u*");                                  // expression
//            _table[atomAddressOf] = String("u&");                                    // expression
//            _table[atomPower] = String("^");                                         // leftExpression rightExpression
//            _table[atomFunctionCall] = String("call");                               // expression     arguments
//
//            _table[atomStringConstant] = String("string");                           // string
//            _table[atomIdentifier] = String("identifier");                           // name
//            _table[atomIntegerConstant] = String("integer");                         // value
//            _table[atomLabelConstant] = String("labelConstant");                     // label
//            _table[atomTrue] = String("true");
//            _table[atomFalse] = String("false");
//            _table[atomNull] = String("null");
//            _table[atomArrayLiteral] = String("array");                              // values
//
//            _table[atomParameter] = String("parameter");                             // typeSpecifier  name
//
//            _table[atomExpressionStatement] = String("expression");                  // expression
//            _table[atomFunctionDefinitionStatement] = String("functionDefinition");  // returnType     name            parameters     statement     basicBlock
//            _table[atomFromStatement] = String("from");                              // dllExpression
//            _table[atomVariableDefinitionStatement] = String("variableDefinition");  // typeSpecifier  identifier      initializer
//            _table[atomAssignment] = String("=");
//            _table[atomAddAssignment] = String("+=");
//            _table[atomSubtractAssignment] = String("-=");
//            _table[atomMultiplyAssignment] = String("*=");
//            _table[atomDivideAssignment] = String("/=");
//            _table[atomModuloAssignment] = String("%=");
//            _table[atomShiftLeftAssignment] = String("<<=");
//            _table[atomShiftRightAssignment] = String(">>=");
//            _table[atomBitwiseAndAssignment] = String("&=");
//            _table[atomBitwiseOrAssignment] = String("|=");
//            _table[atomBitwiseXorAssignment] = String("~=");
//            _table[atomPowerAssignment] = String("^=");
//            _table[atomCompoundStatement] = String("compound");                      // statements
//            _table[atomTypeAliasStatement] = String("type");                         // tycoIdentifier typeSpecifier
//            _table[atomNothingStatement] = String("nothing");
//            _table[atomIncrement] = String("++");
//            _table[atomDecrement] = String("--");
//            _table[atomIfStatement] = String("if");                                  // condition      trueStatement   falseStatement
//            _table[atomSwitchStatement] = String("switch");                          // expression     defaultCase     cases
//            _table[atomReturnStatement] = String("return");                          // expression
//            _table[atomIncludeStatement] = String("include");                        // expression
//            _table[atomBreakStatement] = String("break");                            // statement
//            _table[atomContinueStatement] = String("continue");
//            _table[atomForeverStatement] = String("forever");                        // statement
//            _table[atomWhileStatement] = String("while");                            // doStatement    condition       statement      doneStatement
//            _table[atomUntilStatement] = String("until");                            // doStatement    condition       statement      doneStatement
//            _table[atomForStatement] = String("for");                                // preStatement   expression      postStatement  statement     doneStatement
//            _table[atomLabelStatement] = String("label");                            // identifier
//            _table[atomGotoStatement] = String("goto");                              // identifier
//
//            _table[atomCase] = String("case");                                       // expressions    statement
//            _table[atomDefaultCase] = String("default");                             // statement
//
//            _table[atomPrintFunction] = String("print");                             // returnType     name            parameters
//            _table[atomExit] = String("exit");
//
//            _table[atomBasicBlock] = String("block");                                // instructions   nextBlock
//
//            _table[atomCall] = String("call");
//            _table[atomReturn] = String("return");
//            _table[atomGoto] = String("goto");
//            _table[atomJumpIfTrue] = String("jumpIfTrue");
//            _table[atomStackPointer] = String("stackPointer");
//            _table[atomSetStackPointer] = String("setStackPointer");
//            _table[atomStore] = String("store");
//            _table[atomDuplicate] = String("duplicate");
//            _table[atomDrop] = String("drop");
//
//            _table[atomStringConcatenate] = String("String::+");
//            _table[atomStringEqualTo] = String("String::==");
//            _table[atomStringNotEqualTo] = String("String::!=");
//            _table[atomStringLessThanOrEqualTo] = String("String::<=");
//            _table[atomStringGreaterThanOrEqualTo] = String("String::>=");
//            _table[atomStringLessThan] = String("String::<");
//            _table[atomStringGreaterThan] = String("String::>");
//            _table[atomStringIntegerMultiply] = String("String::*");
//
//            _table[atomTypeKind] = String("TypeKind");
//            _table[atomTemplateKind] = String("TemplateKind");                       // parameterKinds
//        }
//        String lookUp(Atom atom) { return _table[atom]; }
//    private:
//        String _table[atomLast];
//    };
//    static LookupTable lookupTable;
//    return lookupTable.lookUp(atom);
//}

//#include "alfe/symbol.h"
#include "resolve.cpp"

#include "scope.cpp"

#include "parse_tree_object.cpp"
#include "type.cpp"
#include "operator.cpp"
#include "expression.cpp"
#include "statement.cpp"

#include "type_check.cpp"
#include "compiler.cpp"
#include "run.cpp"

void print(RunTimeStack* stack)
{
    console.write(stack->pop<String>());
}

class Program : public ProgramBase
{
protected:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] + " <input file name>\n");
            return;
        }
        File file(_arguments[1]);
        String contents = file.contents();

        Scope scope;

        TycoIdentifier voidIdentifier("Void");
        TycoDefinitionStatement voidDefinition(voidIdentifier,
            BuiltInTycoSpecifier(Kind::type));
        scope.add(voidDefinition);

        TycoIdentifier stringIdentifier("String");
        TycoDefinitionStatement stringDefinition(stringIdentifier,
            BuiltInTycoSpecifier(Kind::type));
        scope.add(stringDefinition);

        typedef FunctionDefinitionStatement::Parameter Parameter;
        List<Parameter> printParameters;
        printParameters.add(Parameter(stringIdentifier, Identifier("string")));

        FunctionDefinitionStatement printFunction(voidIdentifier,
            Identifier("print"), printParameters, BuiltInStatement(print));
        scope.add(printFunction);

        CharacterSource source(contents, file.path());
        Space::parse(&source);
        StatementSequence code = StatementSequence::parse(&source);
        CharacterSource s = source;
        if (s.get() != -1)
            source.location().throwError("Expected end of file");

        code.setScope(scope);

        //FunctionDefinitionStatement main(Type::voidType, "",
        //    CompoundStatement(mainCode), scope);
        ////Symbol main(atomFunctionDefinitionStatement, voidType, String(),
        ////    SymbolArray(), Symbol(atomCompoundStatement, mainCode),
        ////    new FunctionDefinitionCache(Span()));
        ////int mainLabel = labelOf(main);
        ////setScopes(main, scope);
        //resolveIdentifiersAndTypes(main);
        ////checkTypes(main, Symbol(atomVoid));
        //checkTypes(main, Type::voidType);
        //Program program;
        ////evaluate(&program, Symbol(atomFunctionCall, Symbol(atomIdentifier, new IdentifierCache(Span(), mainLabel))));
        //evaluate(&program, FunctionalCall(&main));
    }
};
