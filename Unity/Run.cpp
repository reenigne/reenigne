class RunTimeStack
{
public:
    RunTimeStack(int bytes)
    {
        int entries = bytes >> 2;
        _data.allocate(entries);
        _sp = &_data[entries];

    }
    template<class T> T pop()
    {
        UInt32* p = _sp;
        ++_sp;
        return *reinterpret_cast<T*>(p);
    }
    template<> String pop<String>()
    {
        return pop<StringImplementation*>();
    }
    template<class T> void push(T value)
    {
        --_sp;
        *reinterpret_cast<T*>(_sp) = value;
    }
    template<> void push<String>(String value)
    {
        push(static_cast<StringImplementation*>(value.implementation()));
    }
    UInt32* pointer() const { return _sp; }
    void setPointer(UInt32* pointer) { _sp = pointer; }
private:
    Array<UInt32> _data;
    UInt32* _sp;
};

int power(int a, int b)
{
    if (b < 0)
        return 1/power(a, -b);
    int r = 1;
    while (b != 0) {
        if ((b & 1) != 0)
            r *= a;
        b >>= 1;
        a *= a;
    }
    return r;
}

void run(SymbolArray program)
{
    class InstructionPointer
    {
    public:
        InstructionPointer(SymbolArray program)
        {
            _block = program[0];
            setup();
        }
        void jump(int label)
        {
            _block = Symbol::labelled(label);
            setup();
        }
        Symbol instruction()
        {
            Symbol instruction = _instructions[_instruction];
            ++_instruction;
            if (_instruction == _instructionsInBlock) {
                _label = _block[2].integer();
                jump(_label);
            }
            return instruction;
        }
        int label() const { return _label; }
    private:
        void setup()
        {
            _instructions = _block[1].array();
            _instruction = 0;
            _instructionsInBlock = _instructions.count();
        }
        Symbol _block;
        SymbolArray _instructions;
        int _instruction;
        int _instructionsInBlock;
        int _label;
    };
    RunTimeStack stack(0x100000);
    InstructionPointer ip(program);
    do {
        Symbol instruction = ip.instruction();
        switch (instruction.atom()) {
            case atomExit:
                return;
            case atomPrintFunction:
                stack.pop<String>().write(Handle::consoleOutput());
                break;
            case atomIntegerConstant:
                stack.push(instruction[1].integer());
                break;
            case atomStringConstant:
                stack.push(instruction[1].string());
                break;
            case atomTrue:
                stack.push(true);
                break;
            case atomFalse:
                stack.push(false);
                break;
            case atomNull:
                stack.push(0);
                break;
            case atomCall:
                {
                    int label = stack.pop<int>();
                    stack.push(ip.label());
                    ip.jump(label);
                }
                break;
            case atomReturn:
            case atomGoto:
                ip.jump(stack.pop<int>());
                break;
            case atomJumpIfTrue:
                {
                    int label = stack.pop<int>();
                    if (stack.pop<bool>())
                        ip.jump(label);
                }
                break;
            // TODO: Need implementations of the following for each possible type.
            case atomBitwiseOr:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l - r);
                }
                break;
            case atomBitwiseXor:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l ^ r);
                }
                break;
            case atomBitwiseAnd:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l & r);
                }
                break;
            case atomEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l == r);
                }
                break;
            case atomNotEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l != r);
                }
                break;
            case atomLessThanOrEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l <= r);
                }
                break;
            case atomGreaterThanOrEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l >= r);
                }
                break;
            case atomLessThan:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l < r);
                }
                break;
            case atomGreaterThan:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l > r);
                }
                break;
            case atomLeftShift:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l << r);
                }
                break;
            case atomRightShift:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l >> r);
                }
                break;
            case atomAdd:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l + r);
                }
                break;
            case atomSubtract:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l - r);
                }
                break;
            case atomMultiply:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l * r);
                }
                break;
            case atomDivide:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l / r);
                }
                break;
            case atomModulo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l % r);
                }
                break;
            case atomNot:
                stack.push(~stack.pop<int>());
                break;
            case atomNegative:
                stack.push(-stack.pop<int>());
                break;
            case atomStackPointer:
                stack.push(stack.pointer());
                break;
            case atomSetStackPointer:
                stack.setPointer(stack.pop<UInt32*>());
                break;
            case atomDereference:
                stack.push(*stack.pop<int*>());        
                break;
            case atomDuplicate:
                {
                    int value = stack.pop<int>();
                    stack.push(value);
                    stack.push(value);
                }
                break;
            case atomDrop:
                stack.pop<int>();
                break;
            case atomStore:
                {
                    int value = stack.pop<int>();
                    *stack.pop<int*>() = value;
                }
                break;
            case atomPower:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(power(l, r));
                }
                break;
            case atomStringConcatenate:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l + r);
                }
                break;
            case atomStringEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l == r);
                }
                break;
            case atomStringNotEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l != r);
                }
                break;
            case atomStringLessThanOrEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l <= r);
                }
                break;
            case atomStringGreaterThanOrEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l >= r);
                }
                break;
            case atomStringLessThan:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l < r);
                }
                break;
            case atomStringGreaterThan:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l > r);
                }
                break;
            case atomStringIntegerMultiply:
                {
                    String l = stack.pop<String>();
                    int r = stack.pop<int>();
                    stack.push(l * r);
                }
        }
    } while (true);
}
