class ParseTreeObject
{
public:
    Span span() const { return _implementation->span(); }
    bool valid() const { return _implementation.valid(); }
    template<class T> bool is() const
    {
        return _implementation.is<T::Implementation>();
    }
    template<class T> const typename T::Implementation* as() const
    {
        return _implementation.referent<T::Implementation>();
    }

    class Implementation : public ReferenceCounted
    {
    public:
        Implementation(const Span& span) : _span(span) { }
        Span span() const { return _span; }
    private:
        Span _span;
    };

protected:
    ParseTreeObject() { }
    ParseTreeObject(const Implementation* implementation)
      : _implementation(implementation) { }

    template<class T> const ParseTreeObject& operator=(const T* implementation)
    {
        _implementation = implementation;
        return *this;
    }
    const ParseTreeObject& operator=(const ParseTreeObject& other)
    {
        _implementation = other._implementation;
        return *this;
    }

    ConstReference<Implementation> _implementation;
};
