class DRAM
{
public:
    void initialize(int size, int rowBits, int decayTime, UInt8 decayValue)
    {
        _data.allocate(size);
        _refreshTimes.allocate(1 << rowBits);
        _decayTime = decayTime;
        _decayValue = decayValue;
        _adjustRow = 0;
        _rowMask = (1 << rowBits) - 1;
        _ramSize = size;
    }
    void simulateCycle()
    {
        ++_cycle;
        if (_cycle - _refreshTimes[_adjustRow] > _decayTime)
            _refreshTimes[_adjustRow] = _cycle - _decayTime;
        _adjustRow = (_adjustRow + 1) & _rowMask;
    }
    bool decayed(int address)
    {
        return (_cycle - refresh(address) >= _decayTime);
    }
    UInt8 read(int address)
    {
        if (decayed(address))
            return _decayValue;
        refresh(address) = _cycle;
        if (address >= _ramSize)
            return _decayValue;
        return _data[address];
    }
    void write(int address, UInt8 data)
    {
        refresh(address) = _cycle;
        if(address < _ramSize) _data[address] = data;
    }
    UInt8 memory(int address) { return _data[address]; }

    String save() const
    {
        String s("dram: { data: ###\n");
        for (int y = 0; y < _data.count(); y += 0x20) {
            String line;
            bool gotData = false;
            for (int x = 0; x < 0x20; x += 4) {
                int p = y + x;
                UInt32 v = (_data[p]<<24) + (_data[p+1]<<16) +
                    (_data[p+2]<<8) + _data[p+3];
                if (v != _decayValue)
                    gotData = true;
                line += " " + hex(v, 8, false);
            }
            if (gotData)
                s += hex(y, 5, false) + ":" + line + "\n";
        }
        s += "###,\n  refresh: {";
        int n;
        for (n = _refreshTimes.count() - 1; n >= 0; --n)
            if (_cycle - _refreshTimes[n] < _decayTime)
                break;
        ++n;
        for (int y = 0; y < n; y += 8) {
            String line;
            bool gotData = false;
            for (int x = 0; x < 8; ++x) {
                int v = max(_cycle - _refreshTimes[y + x], _decayTime);
                line += String("0x") + hex(v, 4);
                if (y + x < n - 1)
                    line += ", ";
            }
            if (y + 8 < n)
                s += line + "\n";
        }
        s += "}}";
        return s;
    }
    Type type() const
    {
        List<StructuredType::Member> members;
        members.add(StructuredType::Member("data",
            TypedValue(Type::string, String())));
        members.add(StructuredType::Member("refresh",
            TypedValue(Type::array(Type::integer), List<TypedValue>())));
        return StructuredType("DRAM", members);
    }
    TypedValue initial() const
    {
        return TypedValue(StructuredType(String(),
            List<StructuredType::Member>()),
            Value<HashTable<String, TypedValue>>()).convertTo(type());
    }
    void load(const TypedValue& value)
    {
        for (int a = 0; a < _data.count(); ++a)
            _data[a] = _decayValue;

        auto members = value.value<Value<HashTable<String, TypedValue>>>();
        String s = (*members)["data"].value<String>();
        CharacterSource source(s);
        Space::parse(&source);
        do {
            Span span;
            int t = parseHexadecimalCharacter(&source, &span);
            if (t == -1)
                break;
            int a = t;
            for (int i = 0; i < 4; ++i) {
                t = parseHexadecimalCharacter(&source, &span);
                if (t < 0)
                    span.throwError("Expected hexadecimal character");
                a = (a << 4) + t;
            }
            Space::assertCharacter(&source, ':', &span);
            for (int i = 0; i < 16; ++i) {
                t = parseHexadecimalCharacter(&source, &span);
                if (t < 0)
                    span.throwError("Expected hexadecimal character");
                int t2 = parseHexadecimalCharacter(&source, &span);
                if (t2 < 0)
                    span.throwError("Expected hexadecimal character");
                if (a < _data.count())
                    _data[a] = (t << 4) + t2;
                ++a;
                Space::parse(&source);
            }
        } while (true);
        CharacterSource s2(source);
        if (s2.get() != -1)
            source.location().throwError("Expected hexadecimal character");

        auto refresh = (*members)["refresh"].value<List<TypedValue>>();
        int n = 0;
        for (auto i = refresh.begin(); i != refresh.end(); ++i) {
            if (n < _refreshTimes.count())
                _refreshTimes[n] = _decayTime - (*i).value<int>();
            ++n;
        }
        // Initially, all memory is decayed so we'll get an NMI if we try to
        // read from it.
        for (;n < _refreshTimes.count(); ++n)
            _refreshTimes[n] = _decayTime;
        _cycle = _decayTime;

        _adjustRow = 0;
    }
    String name() const { return "dram"; }

private:
    int& refresh(int address) { return _refreshTimes[address & _rowMask]; }

    Array<UInt8> _data;
    Array<int> _refreshTimes;
    int _cycle;
    int _decayTime;
    int _ramSize;
    int _rowMask;
    UInt8 _decayValue;
    int _adjustRow;
};
