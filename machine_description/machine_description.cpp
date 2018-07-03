class RegisterClassDetails
{
public:
private:
    BitSet _members;
};

class RegisterClass
{
public:
private:
    int _index;
};

class ConstantClassDetails
{
public:
    bool matches(int c)
    {
        if (c < _minimum || c > _maximum)
            return false;
        return (c - _minimum) % _step == 0;
    }
private:
    int _minimum;
    int _maximum;
    int _step;
};

class ConstantClass
{
public:
private:
    int _index;
};

class Register
{
public:
private:
    int _index;
};

class InstructionPattern
{
public:
    Array<RegisterClass> _registerHoles;
    Array<ConstantClass> _constantHoles;

};

class MachineDescription
{
public:
    Array<InstructionPattern> _patterns;
    Array<RegisterClassDetails> _registerClassDetails;
    Array<ConstantClassDetails> _constantClassDetails;
};
