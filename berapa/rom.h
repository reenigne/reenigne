template<class T> class ROMT : public ISA8BitComponent<ROMT<T>>
{
public:
    static String typeName() { return "ROM"; }
    ROMT(Component::Type type) : ROMT(type, 0, 0, "", 0) { }
    ROMT(Component::Type type, int mask, int address, String fileName,
        int offset)
      : ISA8BitComponent<ROMT<T>>(type)
    {
        this->persist("address", &_address, HexPersistenceType(5));
        if (fileName == "")
            return;
        _mask = mask | 0xc0000000;
        _start = address;
        String data =
            File(fileName, this->simulator()->directory()).contents();
        int length = ((_start | ~_mask) & 0xfffff) + 1 - _start;
        int dl = data.length();
        int rl = length + offset;
        if (dl < rl) {
            throw Exception(fileName + " is too short: " + decimal(dl) +
                " bytes found, " + decimal(rl) + " bytes required");
        }
        _data.allocate(length);
        for (int i = 0; i < length; ++i)
            _data[i] = data[i + offset];
    }
    void setAddress(UInt32 address)
    {
        _address = address & 0xfffff & ~_mask;
        this->_active = ((address & _mask) == _start);
    }
    void read() { this->set(_data[_address & ~_mask]); }
    UInt8 debugRead(UInt32 address)
    {
        if ((address & _mask) == _start)
            return _data[address & ~_mask];
        return 0xff;
    }
    class Type : public ISA8BitComponent<ROMT<T>>::Type
    {
    public:
        Type(Simulator* simulator)
          : ISA8BitComponent<ROMT<T>>::Type(ISA8BitComponent<ROMT<T>>::Type::
                template create<Body>(simulator)) { }
    private:
        class Body : public ISA8BitComponent<ROMT<T>>::Type::Body
        {
        public:
            Body(Simulator* simulator)
              : ISA8BitComponent<ROMT<T>>::Type::Body(simulator)
            {
                List<StructuredType::Member> members;
                members.add(StructuredType::Member("mask", IntegerType()));
                members.add(StructuredType::Member("address", IntegerType()));
                members.add(StructuredType::Member("fileName", StringType()));
                members.add(StructuredType::Member("fileOffset",
                    Value(IntegerType(), 0)));
                _structuredType = StructuredType(toString(), members);
            }
            String toString() const { return "ROM"; }
            bool canConvertFrom(const ::Type& other, String* why) const
            {
                return _structuredType.canConvertFrom(other, why);
            }
            Value convert(const Value& value) const
            {
                auto m = _structuredType.convert(value).
                    value<HashTable<Identifier, Value>>();
                int mask = m["mask"].value<int>();
                int address = m["address"].value<int>();
                String file = m["fileName"].value<String>();
                if (file == "")
                    throw Exception("Invalid ROM path");
                int offset = m["fileOffset"].value<int>();
                auto rom = Reference<Component>::create<ROM>(this->type(),
                    mask, address, file, offset);
                this->_simulator->addComponent(rom);
                return Value(this->type(), static_cast<Structure*>(&(*rom)),
                    value.span());
            }
        private:
            StructuredType _structuredType;
        };
    };
private:
    int _mask;
    int _start;
    int _address;
    Array<UInt8> _data;
};
