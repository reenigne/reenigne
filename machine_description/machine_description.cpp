class RegisterClassDetails
{
public:

private:
    BitSet _members;
};

Array<RegisterClassDetails>

class RegisterClass
{
public:
private:
    int _index;
};

typedef int ConstantClass;
typedef int Register;

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
