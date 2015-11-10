class DRAM
{
public:
    void initialize(int size, int rowBits, int decayTime, UInt8 decayValue)
    {
        _data.allocate(size);
        _refreshTimes.allocate(1 << rowBits);
        _decayTime = decayTime;
        _decayValue = decayValue;
        _rowMask = (1 << rowBits) - 1;
        _ramSize = size;
    }
    bool decayed(Tick tick, int address)
    {
        return (tick - refresh(address) >= _decayTime);
    }
    UInt8 read(Tick tick, int address)
    {
        if (decayed(tick, address))
            return _decayValue;
        refresh(address) = tick;
        if (address >= _ramSize)
            return _decayValue;
        return _data[address];
    }
    void write(Tick tick, int address, UInt8 data)
    {
        refresh(address) = tick;
        if (address < _ramSize)
            _data[address] = data;
    }
    UInt8 memory(int address) { return _data[address]; }
    void maintain(Tick ticks)
    {
        for (auto& r : _refreshTimes) {
            if (r >= -_decayTime)
                r -= ticks;
        }
    }

    String save(Tick tick) const
    {
        String s("{ data: ###\n");
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
            if (tick - _refreshTimes[n] < _decayTime)
                break;
        ++n;
        for (int y = 0; y < n; y += 8) {
            String line;
            bool gotData = false;
            for (int x = 0; x < 8; ++x) {
                Tick v = max(tick - _refreshTimes[y + x], _decayTime);
                line += v;
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
            Value(StringType(), String())));
        members.add(StructuredType::Member("refresh",
            Value(SequenceType(IntegerType()), List<Value>())));
        return StructuredType("DRAM", members);
    }
    Value initial() const
    {
        return StructuredType::empty().convertTo(type());
    }
    void load(const Value& value)
    {
        for (int a = 0; a < _data.count(); ++a)
            _data[a] = _decayValue;

        auto members = value.value<HashTable<Identifier, Value>>();
        String s = members["data"].value<String>();
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

        auto refresh = members["refresh"].value<List<Value>>();
        int n = 0;
        for (auto i : refresh) {
            if (n < _refreshTimes.count())
                _refreshTimes[n] = _decayTime - i.value<int>();
            ++n;
        }
        // Initially, all memory is decayed so we'll get an NMI if we try to
        // read from it.
        for (;n < _refreshTimes.count(); ++n)
            _refreshTimes[n] = -_decayTime;
    }
    String name() const { return "dram"; }

private:
    Tick& refresh(int address) { return _refreshTimes[address & _rowMask]; }

    Array<UInt8> _data;
    Array<Tick> _refreshTimes;
    Tick _tick;
    Tick _decayTime;
    int _ramSize;
    int _rowMask;
    UInt8 _decayValue;
};
