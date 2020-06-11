#include "alfe/main.h"
#include "alfe/config_file.h"
#include "alfe/statement.h"

class InstructionPattern
{
};

class Operand
{
};

class AssemblyInstruction : public Statement
{
public:
	AssemblyInstruction(Handle statement) : Statement(statement) { }
	AppendableArray<Byte>::Iterator begin() { return _bytes.begin(); }
	AppendableArray<Byte>::Iterator end() { return _bytes.end(); }

private:
	class Body : public Statement::Body
	{
	public:
		Body(InstructionPattern pattern, List<Operand> operands) { }
	private:
	};
	AppendableArray<Byte> _bytes;
};

class Compiler : public Context
{
public:
	void addCode(StatementSequence code)
	{
		_code = code;
	}
	void compile()
	{
		for (auto statement : _code) {
			// As required:
			//   Resolve names
			//   Instantiate templates
			//   Generate functions
			//   Generate assembly instructions
		}
	}
	void outputExecutableFile(File outputFile)
	{
		AppendableArray<Byte> output;

		for (auto statement : _code) {
			AssemblyInstruction instruction(statement);
			if (!instruction.valid())
				throw Exception("Statement in output is not an assembly "
					"instruction");
			for (Byte b : instruction)
				output.append(b);
		}


	}
private:
	StatementSequence _code;
};

class Program : public ProgramBase
{
public:
	void run()
	{
		if (_arguments.count() < 2) {
			console.write("Syntax: " + _arguments[0] + " <input file name>\n");
			return;
		}
		File file(_arguments[1]);
		String contents = file.contents();
		CharacterSource source(contents, file.path());
		Space::parse(&source);
		StatementSequence code = StatementSequence::parse(&source);
		CharacterSource s = source;
		if (s.get() != -1)
			source.location().throwError("Expected end of file");
		Compiler compiler;
		compiler.addCode(code);
		compiler.compile();

		String outputFileName;
		String inputFileName = file.name();
		int i;
		for (i = inputFileName.length() - 1; i >= 0; --i)
			if (inputFileName[i] == '.')
				break;
		if (i != -1)
			inputFileName = inputFileName.subString(0, i);
		outputFileName = inputFileName + ".com";

		compiler.outputExecutableFile(File(outputFileName, true));
	}
};
