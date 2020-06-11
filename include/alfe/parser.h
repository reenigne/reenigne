#include "alfe/main.h"

#ifndef INCLUDED_PARSER_H
#define INCLUDED_PARSER_H

#include "alfe/code.h"
#include "alfe/space.h"

class Parser : Uncopyable
{
public:
    CodeList parseFromFile(File file)
    {
        CodeList c;
        parseFromFile(c, file);
        return c;
    }
    void parseFromString(Code code, String s)
    {
        CharacterSource source(s);
        do {
            CharacterSource s2 = source;
            if (s2.get() == -1)
                return;
            parseStatementOrFail(code, &source);
        } while (true);
    }
private:
    void parseFromFile(Code code, File file)
    {
        parseFromString(code, file.contents());
    }
    bool parseStatement(Code code, CharacterSource* source)
    {
        if (parseExpressionStatement(code, source))
            return true;
        if (parseFunctionDefinitionStatement(code, source))
            return true;
        if (parseAssignment(code, source))
            return true;
        if (parseCompoundStatement(code, source))
            return true;
        if (parseTycoDefinitionStatement(code, source))
            return true;
        if (parseNothingStatement(source))
            return true;
        if (parseIncrementDecrementStatement(code, source))
            return true;
        if (parseConditionalStatement(code, source))
            return true;
        if (parseSwitchStatement(code, source))
            return true;
        if (parseReturnStatement(code, source))
            return true;
        if (parseIncludeStatement(code, source))
            return true;
        if (parseBreakOrContinueStatement(code, source))
            return true;
        if (parseForeverStatement(code, source))
            return true;
        if (parseWhileStatement(code, source))
            return true;
        if (parseForStatement(code, source))
            return true;
        if (parseLabelStatement(code, source))
            return true;
        if (parseGotoStatement(code, source))
            return true;
        return false;
    }
    void parseStatementOrFail(Code code, CharacterSource* source)
    {
        if (!parseStatement(code, source))
            source->location().throwError("Expected statement");
    }
    bool parseExpressionStatement(Code code, CharacterSource* source)
    {
        CharacterSource s = *source;
        Expression expression = Expression::parse(&s);
        if (!expression.valid())
            return false;
        Span span;
        if (!Space::parseCharacter(&s, ';', &span))
            return false;
        _lastSpan = span;
        *source = s;
        if (!expression.mightHaveSideEffect())
            source->location().throwError("Statement has no effect");
        code.insert<ExpressionStatement>(expression, expression.span() + span);
        return true;
    }
    bool parseAssignment(Code code, CharacterSource* source)
    {
        CharacterSource s = *source;
        Expression left = Expression::parse(&s);
        Location operatorLocation = s.location();
        if (!left.valid())
            return false;
        Span span;

        static const Operator ops[] = {
            OperatorAssignment(), OperatorAddAssignment(),
            OperatorSubtractAssignment(), OperatorMultiplyAssignment(),
            OperatorDivideAssignment(), OperatorModuloAssignment(),
            OperatorShiftLeftAssignment(), OperatorShiftRightAssignment(),
            OperatorBitwiseAndAssignment(), OperatorBitwiseOrAssignment(),
            OperatorBitwiseXorAssignment(), OperatorPowerAssignment(),
            Operator() };

        const Operator* op;
        for (op = ops; op->valid(); ++op)
            if (Space::parseOperator(&s, op->toString(), &span))
                break;
        if (!op->valid())
            return false;

        *source = s;
        Expression right = Expression::parseOrFail(source);
        Space::assertCharacter(source, ';', &span);
        _lastSpan = span;

        code.insert<ExpressionStatement>(FunctionCallExpression::binary(*op,
            span,
            FunctionCallExpression::unary(OperatorAmpersand(), Span(), left),
            right), left.span() + span);
        return true;
    }
    List<VariableDefinition> parseParameterList(CharacterSource* source)
    {
        List<VariableDefinition> list;
        VariableDefinition parameter = VariableDefinition::parse(source);
        if (!parameter.valid())
            return list;
        list.add(parameter);
        Span span;
        while (Space::parseCharacter(source, ',', &span)) {
            VariableDefinition parameter = VariableDefinition::parse(source);
            if (!parameter.valid())
                source->location().throwError("Expected parameter");
            list.add(parameter);
        }
        return list;
    }
    bool parseFunctionDefinitionStatement(Code code, CharacterSource* source)
    {
        CharacterSource s = *source;
        TycoSpecifier returnTypeSpecifier = TycoSpecifier::parse(&s);
        if (!returnTypeSpecifier.valid())
            return false;
        Identifier name = Identifier::parse(&s);
        if (!name.valid())
            return false;
        Span span;
        if (!Space::parseCharacter(&s, '('))
            return false;
        *source = s;
        List<VariableDefinition> parameterList = parseParameterList(source);
        Space::assertCharacter(source, ')');
        Span span;
        if (Space::parseKeyword(source, "from", &span)) {
            Expression dll = Expression::parseOrFail(source);
            Space::assertCharacter(source, ';', &span);
            _lastSpan = span;
            code.insert<FunctionDefinitionFromStatement>(returnTypeSpecifier,
                name, parameterList, dll, returnTypeSpecifier.span() + span);
            return true;
        }
        CodeList body;
        parseStatementOrFail(body, source);
        code.insert<FunctionDefinitionCodeStatement>(returnTypeSpecifier,
            name, parameterList, body, returnTypeSpecifier.span() + _lastSpan);
        return true;
    }
    void parseStatementSequence(Code code, CharacterSource* source)
    {
        do {
            if (!parseStatement(code, source))
                return;
        } while (true);
    }
    bool parseCompoundStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (!Space::parseCharacter(source, '{', &span))
            return false;
        parseStatementSequence(code, source);
        Space::assertCharacter(source, '}', &span);
        _lastSpan = span;
        return true;
    }
    // TycoDefinitionStatement := TycoSignifier "=" TycoSpecifier ";"
    bool parseTycoDefinitionStatement(Code code, CharacterSource* source)
    {
        CharacterSource s = *source;
        CharacterSource s2 = s;
        TycoSignifier tycoSignifier = TycoSignifier::parse(&s);
        if (!tycoSignifier.valid())
            return false;
        if (!Space::parseCharacter(&s, '='))
            return false;
        *source = s;
        TycoSpecifier tycoSpecifier = TycoSpecifier::parse(source);
        Span span;
        Space::assertCharacter(source, ';', &span);
        _lastSpan = span;
        code.insert<TycoDefinitionStatement>(tycoSignifier, tycoSpecifier,
            tycoSignifier.span() + span);
        return true;
    }
    bool parseNothingStatement(CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "nothing", &span))
            return false;
        Space::assertCharacter(source, ';', &span);
        _lastSpan = span;
        return true;
    }
    bool parseIncrementDecrementStatement(Code code, CharacterSource* source)
    {
        Span span;
        Operator o = OperatorIncrement().parse(source, &span);
        if (!o.valid())
            o = OperatorDecrement().parse(source, &span);
        if (!o.valid())
            return false;
        Expression lValue = Expression::parse(source);
        Span span2;
        Space::assertCharacter(source, ';', &span2);
        _lastSpan = span2;
        code.insert<ExpressionStatement>(FunctionCallExpression::unary(o, span,
            FunctionCallExpression::unary(
                OperatorAmpersand(), Span(), lValue)),
            span + span2);
        return true;
    }
    void parseConditionalStatement2(Code code, CharacterSource* source,
        Span span, bool unlessStatement)
    {
        Space::assertCharacter(source, '(');
        Expression condition = Expression::parseOrFail(source);
        Space::assertCharacter(source, ')');
        CodeList conditionalCode;
        parseStatementOrFail(conditionalCode, source);
        span += _lastSpan;
        CodeList elseCode;
        if (Space::parseKeyword(source, "else"))
            parseStatementOrFail(elseCode, source);
        else {
            if (Space::parseKeyword(source, "elseIf"))
                parseConditionalStatement2(elseCode, source, span, false);
            else {
                if (Space::parseKeyword(source, "elseUnless"))
                    parseConditionalStatement2(elseCode, source, span, true);
            }
        }
        if (unlessStatement)
            condition = !condition;
        code.insert<ConditionalStatement>(condition, conditionalCode, elseCode,
            span + _lastSpan);
    }
    // ConditionalStatement = (`if` | `unless`) ConditionedStatement
    //   ((`elseIf` | `elseUnless`) ConditionedStatement)* [`else` Statement];
    // ConditionedStatement = "(" Expression ")" Statement;
    bool parseConditionalStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (Space::parseKeyword(source, "if", &span)) {
            parseConditionalStatement2(code, source, span, false);
            return true;
        }
        if (Space::parseKeyword(source, "unless", &span)) {
            parseConditionalStatement2(code, source, span, true);
            return true;
        }
        return false;
    }
    SwitchStatement::Case parseCase(CharacterSource* source)
    {
        List<Expression> expressions;
        bool defaultType;
        Span span;
        if (Space::parseKeyword(source, "case", &span)) {
            defaultType = false;
            do {
                Expression expression = Expression::parseOrFail(source);
                expressions.add(expression);
                if (!Space::parseCharacter(source, ','))
                    break;
            } while (true);
        }
        else {
            defaultType = true;
            if (!Space::parseKeyword(source, "default", &span))
                source->location().throwError("Expected case or default");
        }
        Space::assertCharacter(source, ':');
        CodeList code;
        parseStatementOrFail(code, source);
        span += _lastSpan;
        if (defaultType)
            return SwitchStatement::Case(code, span);
        return SwitchStatement::Case(expressions, code, span);
    }
    bool parseSwitchStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "switch", &span))
            return false;
        Space::assertCharacter(source, '(');
        Expression expression = Expression::parseOrFail(source);
        Space::assertCharacter(source, ')');
        Space::assertCharacter(source, '{');
        SwitchStatement::Case defaultCase;

        CharacterSource s = *source;
        List<SwitchStatement::Case> cases;
        do {
            SwitchStatement::Case c = parseCase(source);
            if (!c.valid())
                break;
            if (c.isDefault()) {
                if (defaultCase.valid())
                    s.location().throwError(
                        "This switch statement already has a default case");
                defaultCase = c;
            }
            else
                cases.add(c);
        } while (true);
        Space::assertCharacter(source, '}', &span);
        _lastSpan = span;
        code.insert<SwitchStatement>(expression, defaultCase, cases, span);
        return true;
    }
    bool parseReturnStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "return", &span))
            return false;
        Expression expression = Expression::parseOrFail(source);
        Space::assertCharacter(source, ';', &span);
        _lastSpan = span;
        code.insert<ReturnStatement>(expression, span);
        return true;
    }
    bool parseIncludeStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "include", &span))
            return false;
        Expression expression = parseExpression(source);

        StringLiteralExpression s(expression);
        if (!s.valid()) {
            expression.span().throwError("Argument to include must be a "
                "simple string");
        }
        String path = s.string();

        //Resolver resolver;
        //resolver.resolve(expression);
        //Evaluator evaluator;
        //Value value = evaluator.evaluate(expression).convertTo(StringType());
        //String path = value.value<String>();

        File lastFile = _currentFile;
        _currentFile = File(path, _currentFile.parent());
        parseFromFile(code, _currentFile);
        _currentFile = lastFile;
    }
    bool parseBreakOrContinueStatement(Code code, CharacterSource* source)
    {
        int breakCount = 0;
        bool hasContinue = false;
        Span span;
        do {
            if (Space::parseKeyword(source, "break", &span))
                ++breakCount;
            else
                break;
        } while (true);
        if (Space::parseKeyword(source, "continue", &span))
            hasContinue = true;
        if (breakCount == 0 && !hasContinue)
            return false;
        Space::assertCharacter(source, ';', &span);
        _lastSpan = span;
        code.insert<BreakOrContinueStatement>(span, breakCount, hasContinue);
        return true;
    }
    bool parseForeverStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "forever", &span))
            return false;
        CodeList body;
        parseStatementOrFail(body, source);
        code.insert<ForeverStatement>(body, span);
        return true;
    }
    bool parseWhileStatement(Code code, CharacterSource* source)
    {
        Span span;
        CodeList doCode;
        bool foundDo = false;
        if (Space::parseKeyword(source, "do", &span)) {
            foundDo = true;
            parseStatementOrFail(doCode, source);
        }
        bool foundWhile = false;
        bool foundUntil = false;
        if (Space::parseKeyword(source, "while", &span))
            foundWhile = true;
        else
            if (Space::parseKeyword(source, "until", &span))
                foundUntil = true;
        if (!foundWhile && !foundUntil) {
            if (foundDo)
                source->location().throwError("Expected while or until");
            return false;
        }
        Space::assertCharacter(source, '(');
        Expression condition = Expression::parse(source);
        Space::assertCharacter(source, ')');
        CodeList body;
        parseStatementOrFail(body, source);
        CodeList doneCode;
        if (Space::parseKeyword(source, "done"))
            parseStatementOrFail(doneCode, source);
        span += _lastSpan;
        if (foundUntil)
            condition = !condition;
        code.insert<WhileStatement>(doCode, condition, body, doneCode, span);
        return true;
    }
    bool parseForStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "for", &span))
            return false;
        Space::assertCharacter(source, '(');
        CodeList preCode;
        if (!parseStatement(preCode, source))
            Space::assertCharacter(source, ';');
        Expression expression = Expression::parse(source);
        Space::assertCharacter(source, ';');
        CodeList postCode;
        parseStatement(postCode, source);
        Space::parseCharacter(source, ')');
        CodeList body;
        parseStatement(body, source);
        span += _lastSpan;
        CodeList doneCode;
        if (Space::parseKeyword(source, "done")) {
            parseStatementOrFail(doneCode, source);
            span += _lastSpan;
        }
        code.insert<ForStatement>(preCode, expression, postCode, body,
            doneCode, span);
        return true;
    }
    bool parseLabelStatement(Code code, CharacterSource* source)
    {
        CharacterSource s2 = *source;
        Identifier identifier = Identifier::parse(&s2);
        if (!identifier.valid())
            return false;
        Span span;
        if (!Space::parseCharacter(&s2, ':', &span))
            return false;
        _lastSpan = span;
        code.insert<LabelStatement>(identifier, identifier.span() + span);
        return true;
    }
    bool parseGotoStatement(Code code, CharacterSource* source)
    {
        Span span;
        if (!Space::parseKeyword(source, "goto", &span))
            return false;
        Expression expression = Expression::parseOrFail(source);
        Span span2;
        Space::parseCharacter(source, ';', &span);
        code.insert<GotoStatement>(expression, span);
        return true;
    }

    Span _lastSpan;
    File _currentFile;
};

#endif // INCLUDED_PARSER_H