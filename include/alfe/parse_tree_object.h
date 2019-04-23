#include "alfe/main.h"

#ifndef INCLUDED_PARSE_TREE_OBJECT_H
#define INCLUDED_PARSE_TREE_OBJECT_H

class ParseTreeObject : public Handle
{
public:
    Span span() const { return body()->span(); }
    void setSpan(Span span) { body()->setSpan(span); }

    class Body : public Handle::Body
    {
    public:
        Body(const Span& span) : _span(span) { }
        Span span() const { return _span; }
        void setSpan(Span span) { _span = span; }
    private:
        Span _span;
    };

protected:
    ParseTreeObject() { }
    ParseTreeObject(Handle other) : Handle(other) { }

    const Body* body() const { return as<Body>(); }
    Body* body() { return as<Body>(); }
};

#endif // INCLUDED_PARSE_TREE_OBJECT_H
