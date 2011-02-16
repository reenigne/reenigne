#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hash_table.h"
#include "unity/character_source.h"
#include "unity/command_line.h"

#include <stdlib.h>

#include "Symbol.cpp"
#include "Type.cpp"
#include "Resolve.cpp"

#include "Space.cpp"
#include "Expression.cpp"
#include "TypeSpecifier.cpp"
#include "Statement.cpp"

#include "TypeCheck.cpp"
#include "Compiler.cpp"
#include "Run.cpp"

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

        int printLabel = Symbol::newLabel();
        IdentifierCache* printCache = new IdentifierCache(Span(), printLabel);
        Symbol print(atomPrintFunction, Symbol(atomVoid), String("print"), SymbolArray(Symbol(atomString)), printCache);
        print.setLabel(printLabel);
        scope->addFunction(String("print"), printLabel, Span());

        CharacterSource source(contents, file.path());
        Space::parse(&source);
        SymbolArray program = parseStatementSequence(&source);
        CharacterSource s = source;
        if (s.get() != -1) {
            static String error("Expected end of file");
            source.location().throwError(error);
        }

        setScopes(program, scope);
        resolveIdentifiersAndTypes(program);
        checkTypes(program, Symbol(atomVoid));
        Compiler compiler;
        compiler.compileFunctionBody(program);
        SymbolArray compiledProgram = compiler.compiledProgram();
        run(compiledProgram);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
