#include "alfe/code.h"

#ifndef INCLUDED_FLATTENER_H
#define INCLUDED_FLATTENER_H

class Flattener
{
public:
    void resolve(Code code)
    {
        Flatten flatten;
        code.walk(&flatten);
        code.annotate<CodeFormAnnotation>().setFlattened(true);
    }
private:
    class Flatten : public CodeWalker
    {
    public:
        Result visit(Annotation c)
        {
            ConditionalStatement cs = c;
            if (cs.valid()) {
                cs.setCondition(!cs.condition());
                cs.setFalseStatement(Code());
                Code doJump;
                Identifier falseTarget(1);
                doJump.insert<GotoStatement>(falseTarget);
                cs.setTrueStatement(doJump);
                CodeNode n = cs.next();
                n.insert(cs.trueStatement());
                Identifier doneTarget(1);
                n.insert<GotoStatement>(doneTarget);
                n.insert<LabelStatement>(falseTarget);
                n.insert(cs.falseStatement());
                n.insert<LabelStatement>(doneTarget);
                return Result::advance;
            }
            ForeverStatement fes = c;
            if (fes.valid()) {
                CodeNode n = fes.next();
                Identifier l(1);
                n.insert<LabelStatement>(l);
                n.insert(fes.code());
                n.insert<GotoStatement>(l);
                fes.destroy();
                return Result::advance;
            }
            WhileStatement ws = c;
            if (ws.valid()) {
                CodeNode n = ws.next();
                Identifier l(1);
                Identifier d(1);
                n.insert<LabelStatement>(l);
                n.insert(ws.doStatement());
                Code doJump;
                doJump.insert<GotoStatement>(d);
                n.insert<ConditionalStatement>(!ws.condition(), doJump,
                    Code());
                n.insert(ws.statement());
                n.insert<GotoStatement>(l);
                n.insert<LabelStatement>(d);
                n.insert(ws.doneStatement());
                ws.destroy();
                return Result::advance;
            }
            ForStatement fs = c;
            if (fs.valid()) {
                CodeNode n = fs.next();
                Identifier l(1);
                Identifier d(1);
                n.insert(fs.preStatement());
                n.insert<LabelStatement>(l);
                Code doJump;
                doJump.insert<GotoStatement>(d);
                n.insert<ConditionalStatement>(!fs.condition(), doJump,
                    Code());
                n.insert(fs.statement());
                n.insert(fs.postStatement());
                n.insert<GotoStatement>(l);
                n.insert<LabelStatement>(d);
                n.insert(fs.doneStatement());
                fs.destroy();
                return Result::advance;
            }

            return Result::recurse;
        }
        Result visit(ParseTreeObject o) { return Result::recurse; }
        Result visit(Tyco t) { return Result::recurse; }
    private:
        List<LabelStatement> breakLabels;
        List<LabelStatement> continueLabels;
    };
};

#endif // INCLUDED_FLATTENER_H
