#include "alfe/main.h"

#ifndef INCLUDED_FUNCTION_H
#define INCLUDED_FUNCTION_H

template<class T> class TypedValueTemplate;
typedef TypedValueTemplate<void> TypedValue;

class Function
{
public:
    virtual TypedValue evaluate(List<TypedValue> arguments) const = 0;
};

#endif // INCLUDED_FUNCTION_H
