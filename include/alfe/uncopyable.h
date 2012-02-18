#ifndef INCLUDED_UNCOPYABLE_H
#define INCLUDED_UNCOPYABLE_H

class Uncopyable
{
public:
    Uncopyable() { }
private:
    Uncopyable(const Uncopyable& other);
    const Uncopyable& operator=(const Uncopyable& other);
};

#endif // INCLUDED_UNCOPYABLE_H
