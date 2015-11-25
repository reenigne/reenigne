class RAM : public Component
{
public:
    static String typeName() { return "RAM"; }
    RAM()
    {
        connector("parityError", &_parityError);
        persist("data", &_data, Value(PersistDataType(this)));
        persist("refresh", &_refreshTimes[0], Tick(0), ArrayType(Tick::Type(), 0));
    }
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
        if (decayed(tick, address)) {
            // RAM has decayed! On a real machine this would not always signal
            // an NMI but we'll make the NMI happen every time to make DRAM
            // decay problems easier to find.

            // TODO: In the config file we might want to have an option for
            // "realistic mode" to prevent this from being used to detect the
            // emulator.
            //if (_nmiSwitch->nmiOn() && (_ppi->portB() & 0x10) != 0)
            //    _cpu->nmi();
            return _decayValue;
        }
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
        String s = "refresh: {";
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
    void load(const Value& value)
    {
        for (int a = 0; a < _data.count(); ++a)
            _data[a] = _decayValue;

        auto members = value.value<HashTable<Identifier, Value>>();

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
    class PersistDataType : public ::Type
    {
    public:
        PersistDataType(RAM* ram) : ::Type(new Body(ram)) { }
    private:
        class Body : public ::Type::Body
        {
        public:
            Body(RAM* ram) : _ram(ram) { }

            String serialize(void* p, int width, int used, int indent,
                int delta) const
            {
                if (indent == 0)
                    return "*";
                auto data = static_cast<Array<UInt8>*>(p);
                String s("###\n");
                for (int y = 0; y < data->count(); y += 0x20) {
                    String line;
                    bool gotData = false;
                    for (int x = 0; x < 0x20; x += 4) {
                        const UInt8* p = &((*data)[y + x]);
                        UInt32 v = (p[0]<<24) + (p[1]<<16) + (p[2]<<8) + p[3];
                        if (v != _ram->_decayValue)
                            gotData = true;
                        line += " " + hex(v, 8, false);
                    }
                    if (gotData)
                        s += hex(y, 5, false) + ":" + line + "\n";
                }
                s += "###";

                return decimal(*static_cast<int*>(p));
            }
            void deserialize(const Value& value, void* p) const
            {
                CharacterSource source(value.value<String>());
                Space::parse(&source);
                auto data = static_cast<Array<UInt8>*>(p);
                for (int i = 0; i < data->count(); ++i)
                    (*data)[i] = _ram->_decayValue;
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
                        if (a < data->count())
                            (*data)[a] = (t << 4) + t2;
                        else
                            span.throwError("Address out of range");
                        ++a;
                        Space::parse(&source);
                    }
                } while (true);
                CharacterSource s2(source);
                if (s2.get() != -1) {
                    source.location().throwError(
                        "Expected hexadecimal character");
                }
            }
            int size() const { return 0; }
            Value defaultValue() const { return ""; }
            Value value(void* p) const { return *static_cast<int*>(p); }
            String toString() const { return "RAMData"; }
        private:
            RAM* _ram;
        };
    };

    Tick& refresh(int address) { return _refreshTimes[address & _rowMask]; }

    OutputConnector<bool> _parityError;
    Array<UInt8> _data;
    Array<Tick> _refreshTimes;
    Tick _tick;
    Tick _decayTime;
    int _ramSize;
    int _rowMask;
    UInt8 _decayValue;
};
