#ifndef INCLUDED_EXCEPTION_H
#define INCLUDED_EXCEPTION_H

class Exception;

#include "string.h"

class Exception
{
public:
    Exception(const String& message) : _message(message) { }
    void print() const { _message.print(); }
private:
    String _message;
};

#endif // INCLUDED_EXCEPTION_H
