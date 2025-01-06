#include "alfe/main.h"

#ifndef INCLUDED_PATTERN_DATABASE_H
#define INCLUDED_PATTERN_DATABASE_H

class PatternDatabase
{
public:
    void addPattern(Pattern pattern)
    {
        _patterns.append(pattern);
    }
private:
    AppendableArray<Pattern> _patterns;
};

#endif // INCLUDED_PATTERN_DATABASE_H
