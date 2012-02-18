#include "alfe/integer_types.h"

class DoublyLinkedListEntry
{
public:
    DoublyLinkedListEntry* _previous;
    DoublyLinkedListEntry* _next;
};

class LowLevelPointer : public DoublyLinkedListEntry
{
public:
    Void* _referent;
};

class Raviolo
{
public:
    virtual void fixupLists(PointerDifference d)
    {
        _p0._previous->_next += d;
        _p0._next->_previous += d;
        _p1._previous->_next += d;
        _p1._next->_previous += d;
        _p2._previous->_next += d;
        _p2._next->_previous += d;
        _p3._previous->_next += d;
        _p3._next->_previous += d;
    }
    void move(Raviolo* to)
    {
        *to = *this;
        DoublyLinkedListEntry* e = _list._next;
        PointerDifference d = to - this;
        while (e != &_list) {
            static_cast<Raviolo*>(static_cast<LowLevelPointer*>(e)->_referent) += d;
            e = e->_next;
        }
        fixupLists(d);
    }
    LowLevelPointer _p0;
    LowLevelPointer _p1;
    LowLevelPointer _p2;
    LowLevelPointer _p3;
    Void* _spare;
    DoublyLinkedListEntry _list;
};

