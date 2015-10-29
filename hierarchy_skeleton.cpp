template<class T> class BaseTemplate;
typedef BaseTemplate<void> Base;

template<class T> class DerivedTemplate;
typedef DerivedTemplate<void> Derived;

template<class T> class BaseTemplate : public Handle
{
protected:
    BaseTemplate() { }
    BaseTemplate(Body* body) : Handle(body) { }

    class Body : public Handle::Body
    {
    public:
        virtual void process() = 0;
    };
};

template<class T> class DerivedTemplate : public Base
{
protected:
    DerivedTemplate() { }
    DerivedTemplate(Body* body) : Base(body) { }

    class Body : public Base::Body
    {
    public:
        void process() { }
    };
};


