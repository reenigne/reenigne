#include "alfe/main.h"

#ifndef INCLUDED_PARSE_TREE_OBJECT_H
#define INCLUDED_PARSE_TREE_OBJECT_H

class ParseTreeObject : public ConstHandle
{
public:
    Span span() const { return body()->span(); }

    class Body : public ConstHandle::Body
    {
    public:
        Body(const Span& span) : _span(span) { }
        Span span() const { return _span; }
    private:
        Span _span;
    };

protected:
    ParseTreeObject() { }
    ParseTreeObject(const ConstHandle& other) : ConstHandle(other) { }

    const Body* body() const { return as<Body>(); }
};

#endif // INCLUDED_PARSE_TREE_OBJECT_H
