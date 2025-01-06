#include "alfe/code.h"
#include "alfe/stack.h"

#ifndef INCLUDED_FLATTENER_H
#define INCLUDED_FLATTENER_H

class Flattener
{
public:
    Code flatten(Code code)
    {
        code = Flatten().flattened(code);
        code.annotate<CodeFormAnnotation>().setFlattened(true);
        return code;
    }
private:
    class Flatten : public CodeWalker
    {
    public:
        Result visit(Annotation c)
        {
            ConditionalStatement cs = c;
            if (cs.valid()) {
                // if ($condition) $trueStatement else $falseStatement
                // ->
                // if (!$condition) goto falseTarget;
                // $trueStatement
                // goto doneStatement;
                // falseTarget:
                // $falseStatement
                // doneStatement:
                // 
                // We can re-use the ConditionalStatement
                cs.setCondition(!cs.condition());
                Code falseStatement = cs.falseStatement();
                cs.setFalseStatement(Code());
                Code doJump;
                Identifier falseTarget(1);
                doJump.insert<GotoStatement>(falseTarget);
                Code trueStatement = cs.trueStatement();
                cs.setTrueStatement(doJump);
                CodeNode n = cs.next();
                n.insert(trueStatement);
                Identifier doneTarget(1);
                n.insert<GotoStatement>(doneTarget);
                n.insert<LabelStatement>(falseTarget);
                n.insert(falseStatement);
                n.insert<LabelStatement>(doneTarget);
                return Result::advance;
            }
            ForeverStatement fes = c;
            if (fes.valid()) {
                // forever $statement
                // ->
                // continueTarget:
                // $statement
                // goto continueTarget:
                // breakTarget:
                Identifier continueTarget(1);
                Identifier breakTarget(1);
                fes.insert<LabelStatement>(continueTarget);
                continueLabels.push(continueTarget);
                breakLabels.push(breakTarget);
                fes.insert(flattened(fes.code()));
                breakLabels.pop();
                continueLabels.pop();
                fes.insert<GotoStatement>(continueTarget);
                fes.insert<LabelStatement>(breakTarget);
                return Result::remove;
            }
            WhileStatement ws = c;
            if (ws.valid()) {
                // do $doStatment while($condition) $statement done $doneStatement
                // ->
                // loopTarget:
                // $doStatement  // Note that continue goes to condititionTarget here
                // conditionTarget:
                // if (!$condition) goto doneTarget;
                // $statement    // Note that continue goes to loopTarget here
                // goto loopTarget:
                // doneTarget:
                // $doneStatement
                // breakTarget:
                Identifier loopTarget(1);
                Identifier conditionTarget(1);
                Identifier doneTarget(1);
                Identifier breakTarget(1);
                continueLabels.push(conditionTarget);
                breakLabels.push(breakTarget);
                ws.insert<LabelStatement>(loopTarget);
                ws.insert(flattened(ws.doStatement()));
                Code doJump;
                doJump.insert<GotoStatement>(doneTarget);
                ws.insert<ConditionalStatement>(!ws.condition(), doJump,
                    Code());
                continueLabels.pop();
                continueLabels.push(loopTarget);
                ws.insert(flattened(ws.statement()));
                ws.insert<GotoStatement>(loopTarget);
                ws.insert<LabelStatement>(doneTarget);
                ws.insert(flattened(ws.doneStatement()));
                breakLabels.pop();
                continueLabels.pop();
                ws.insert<LabelStatement>(breakTarget);
                return Result::remove;
            }
            ForStatement fs = c;
            if (fs.valid()) {
                // for ($preStatement; $condition; $postStatement) $statement
                // ->
                // $preStatement
                // loopTarget:
                // if (!$condition) goto doneTarget;
                // $statement   
                // continueTarget:
                // $postStatement
                // goto loopTarget:
                // doneTarget:
                // $doneStatement
                // breakTarget:
                Identifier loopTarget(1);
                Identifier continueTarget(1);
                Identifier doneTarget(1);
                Identifier breakTarget(1);
                continueLabels.push(continueTarget);
                breakLabels.push(breakTarget);
                fs.insert(flattened(fs.preStatement()));
                fs.insert<LabelStatement>(loopTarget);
                Code doJump;
                doJump.insert<GotoStatement>(doneTarget);
                fs.insert<ConditionalStatement>(!fs.condition(), doJump,
                    Code());
                fs.insert(flattened(fs.statement()));
                fs.insert<LabelStatement>(continueTarget);
                fs.insert(flattened(fs.postStatement()));
                fs.insert<GotoStatement>(loopTarget);
                fs.insert<LabelStatement>(doneTarget);
                fs.insert(flattened(fs.doneStatement()));
                return Result::remove;
            }
            BreakOrContinueStatement bcs = c;
            if (bcs.valid()) {
                if (bcs.hasContinue()) {
                    bcs.insert<GotoStatement>(
                        continueLabels.fromTop(bcs.breakCount()));
                }
                else {
                    bcs.insert<GotoStatement>(
                        breakLabels.fromTop(bcs.breakCount() - 1));
                }
                return Result::remove;
            }
            return Result::recurse;
        }
        Result visit(ParseTreeObject o) { return Result::recurse; }
        Result visit(Tyco t) { return Result::recurse; }
        Code flattened(Code c)
        {
            c.walk(this);
            return c;
        }

    private:
        Stack<Identifier> breakLabels;
        Stack<Identifier> continueLabels;
    };
};

#endif // INCLUDED_FLATTENER_H
