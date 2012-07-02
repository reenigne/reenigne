class ParseTreeObject
{
public:
    Span span() const { return _implementation->span(); }
    bool valid() const { return _implementation.valid(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        Implementation(const Span& span) : _span(span) { }
        Span span() const { return _span; }
    private:
        Span _span;
    };

    ParseTreeObject() { }
    ParseTreeObject(const Implementation* implementation)
      : _implementation(implementation) { }

    template<class T> const ParseTreeObject& operator=(const T* implementation)
    {
        _implementation = implementation;
        return *this;
    }

    ConstReference<Implementation> _implementation;
};