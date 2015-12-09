template<class T> class BaseT;
typedef BaseT<void> Base;

template<class T> class DerivedT;
typedef DerivedT<void> Derived;

template<class T> class BaseT : public Handle
{
public:
    BaseT(...) : Handle(create<Body>(...)) { }
    void process() { return body()->process(); }
protected:
    BaseT(const Handle& other) : Handle(other) { }

    class Body : public Handle::Body
    {
    public:
        virtual void process() = 0;
    };
private:
    Body* body() { return as<Body>(); }
};

template<class T> class DerivedT : public Base
{
public:
    DerivedT(...) : Base(create<Body>(...)) { }
private:
    DerivedT(const Handle& other) : Base(other) { }

    class Body : public Base::Body
    {
    public:
        void process() { }
    };
};


