class CharacterPointer
{
public:
    CharacterPointer(const String& string, int offset) : _string(string), _offset(offset) { }
    int operator*() const
    {
        // TODO: decode UTF-8
    }
    CharacterPointer& operator++()
    {
        // TODO: increment
        return *this;
    }
    CharacterPointer operator++(int)
    {
        CharacterPointer current = *this;
        ++(*this);
        return current;
    }
    bool atEnd()
    {
        return _offset == _string.length();
    }
private:
    String _string;
    int _offset;
};


