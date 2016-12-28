#include "alfe/main.h"

#ifndef INCLUDED_SPACE_H
#define INCLUDED_SPACE_H

#include "alfe/rational.h"

class Space
{
public:
    static void parse(CharacterSource* source)
    {
        do {
            CharacterSource s = *source;
            int c = s.get();
            if (c == ' ' || c == 10) {
                *source = s;
                continue;
            }
            if (parseComment(source))
                continue;
            return;
        } while (true);
    }
    static bool parseCharacter(CharacterSource* source, int character,
        Span* span = 0)
    {
        if (!source->parse(character, span))
            return false;
        parse(source);
        return true;
    }
    static void assertCharacter(CharacterSource* source, int character,
        Span* span = 0)
    {
        source->assert(character, span);
        parse(source);
    }
    static bool parseOperator(CharacterSource* source, String op,
        Span* span = 0)
    {
        CharacterSource s = *source;
        CharacterSource o(op);
        Span sp;
        do {
            Span sp2;
            int c = o.get();
            if (c == -1)
                break;
            if (s.get(&sp2) != c)
                return false;
            sp += sp2;
        } while (true);
        *source = s;
        if (span != 0)
            *span += sp;
        parse(source);
        return true;
    }
    static bool parseKeyword(CharacterSource* source, String keyword,
        Span* span = 0)
    {
        CharacterSource s = *source;
        CharacterSource o(keyword);
        Span sp;
        Span sp2;
        do {
            int c = o.get();
            if (c == -1)
                break;
            if (s.get(&sp2) != c)
                return false;
            sp += sp2;
        } while (true);
        CharacterSource s2 = s;
        int c = s2.get(&sp2);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_')
            return false;
        *source = s;
        if (span != 0)
            *span += sp;
        parse(source);
        return true;
    }
    static bool parseNumber(CharacterSource* source, Rational* result,
        Span* span = 0)
    {
        CharacterSource s = *source;
        int n = 0;
        int c = s.get(span);
        int denominator = 1;
        bool seenPoint = false;
        if (c < '0' || c > '9')
            return false;
        if (c == '0') {
            CharacterSource s2 = s;
            Span span2;
            int c = s2.get(&span2);
            if (c == 'x') {
                bool okay = false;
                int n = 0;
                do {
                    c = s2.get(&span2);
                    if (c == '.' && !seenPoint)
                        seenPoint = true;
                    else
                        if (c >= '0' && c <= '9') {
                            n = n*0x10 + c - '0';
                            if (seenPoint)
                                denominator <<= 4;
                        }
                        else
                            if (c >= 'A' && c <= 'F') {
                                n = n*0x10 + c + 10 - 'A';
                                if (seenPoint)
                                    denominator <<= 4;
                            }
                            else
                                if (c >= 'a' && c <= 'f') {
                                    n = n*0x10 + c + 10 - 'a';
                                    if (seenPoint)
                                        denominator <<= 4;
                                }
                                else
                                    if (okay) {
                                        parse(source);
                                        *result = Rational(n, denominator);
                                        return true;
                                    }
                                    else
                                        return false;
                    okay = true;
                    *source = s2;
                    if (span != 0)
                        *span += span2;
                } while (true);
            }
        }
        do {
            n = n*10 + c - '0';
            if (seenPoint)
                denominator *= 10;
            *source = s;
            Span span2;
            c = s.get(&span2);
            if (c == '.' && !seenPoint) {
                seenPoint = true;
                c = s.get(&span2);
            }
            if (c < '0' || c > '9') {
                parse(source);
                *result = Rational(n, denominator);
                return true;
            }
            if (span != 0)
                *span += span2;
        } while (true);
    }
private:
    static bool parseComment(CharacterSource* source)
    {
        CharacterSource s = *source;
        int c = s.get();
        if (c != '/')
            return false;
        c = s.get();
        if (c == '/') {
            do {
                *source = s;
                c = s.get();
                if (c == 10 || c == -1)
                    break;
                if (c < 0x20)
                    source->throwUnexpected("printable character", hex(c, 2));
            } while (true);
            *source = s;
            return true;
        }
        if (c == '*') {
            do {
                if (parseComment(&s))
                    continue;
                *source = s;
                c = s.get();
                while (c == '*') {
                    c = s.get();
                    if (c == '/') {
                        *source = s;
                        return true;
                    }
                }
                if (c == -1)
                    source->location().throwError("End of file in comment");
                if (c < 0x20 && c != 10)
                    source->throwUnexpected("printable character", hex(c, 2));
            } while (true);
        }
        return false;
    }
};

#endif // INCLUDED_SPACE_H
