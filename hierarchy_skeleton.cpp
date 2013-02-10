template<class T> class BaseTemplate;
typedef BaseTemplate<void> Base;

template<class T> class DerivedTemplate;
typedef DerivedTemplate<void> Derived;

template<class T> class BaseTemplate
{
protected:
    BaseTemplate() { }
    BaseTemplate(Implementation* implementation)
      : _implementation(implementation) { }

    class Implementation : public ReferenceCounted
    {
    public:
        virtual void process() = 0;
    };

    Reference<Implementation> _implementation;
};

template<class T> class DerivedTemplate : public Base
{
protected:
    DerivedTemplate() { }
    DerivedTemplate(Implementation* implementation)
      : Base(implementation) { }

    class Implementation : public Base::Implementation
    {
    public:
        void process() { }
    };
};


