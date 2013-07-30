#ifndef COMMON_H
#define COMMON_H

#include "alfe/string.h"
#include "alfe/array.h"
#include "alfe/file.h"
#include "alfe/stack.h"
#include "alfe/hash_table.h"
#include "alfe/main.h"
#include "alfe/space.h"
//#include "alfe/config_file.h"
#include "alfe/type.h"

#include <stdlib.h>

int parseHexadecimalCharacter(CharacterSource* source, Span* span)
{
    CharacterSource s = *source;
    int c = s.get(span);
    if (c >= '0' && c <= '9') {
        *source = s;
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        *source = s;
        return c + 10 - 'A';
    }
    if (c >= 'a' && c <= 'f') {
        *source = s;
        return c + 10 - 'a';
    }
    return -1;
}

template<class T> class Intel8088Template;

typedef Intel8088Template<void> Intel8088;

class Component
{
public:
    virtual void simulateCycle() { }
    virtual String save() { return String(); }
    virtual Type type() { return Type(); }
    virtual String name() { return String(); }
    virtual void load(const TypedValue& value) { }
    virtual TypedValue initial() { return TypedValue(); }
};

class Simulator
{
public:
    Simulator() : _halted(false) { }
    void simulate()
    {
        do {
            for (auto i = _components.begin(); i != _components.end(); ++i)
                (*i)->simulateCycle();
        } while (!_halted);
    }
    void addComponent(Component* component) { _components.add(component); }
    String save()
    {
        String s("simulator = {");
        bool needComma = false;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            if ((*i)->name().empty())
                continue;
            if (needComma)
                s += ", ";
            needComma = true;
            s += (*i)->save();
        }
        s += "};";
        return s;
    }
    Type type()
    {
        List<StructuredType::Member> members;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            Type type = (*i)->type();
            if (!type.valid())
                continue;
            members.add(StructuredType::Member((*i)->name(), (*i)->type()));
        }
        return StructuredType("Simulator", members);
    }
    void load(const TypedValue& value)
    {
        Value<HashTable<String, TypedValue> > object =
            value.value<Value<HashTable<String, TypedValue> > >();
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->load(object->operator[]((*i)->name()));
    }
    TypedValue initial()
    {
        Value<HashTable<String, TypedValue> > object;
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->type().valid())
                object->operator[]((*i)->name()) = (*i)->initial();
        return TypedValue(type(), object);
    }
    void halt() { _halted = true; }
private:
    List<Component*> _components;
    bool _halted;
};

class ISA8BitBus;

template<class T> class ISA8BitComponentTemplate : public Component
{
public:
    // Address bit 31 = write
    // Address bit 30 = IO
    virtual void setAddress(UInt32 address) = 0;
    virtual void write(UInt8 data) = 0;
    virtual bool wait() { return false; }
    void setBus(ISA8BitBus* bus) { _bus = bus; }
    virtual UInt8 memory(UInt32 address) { return 0xff; }
    bool active() const { return _active; }
    virtual void read() { }
	void requestInterrupt(UInt8 data)
	{
		_bus->_interruptnum = data & 7;
		_bus->_interrupt = true;
	}
	virtual void handleInterrupt() = 0;
protected:
    void set(UInt8 data) { _bus->_data = data; }
    ISA8BitBus* _bus;
    bool _active;
};

typedef ISA8BitComponentTemplate<void> ISA8BitComponent;

class ISA8BitBus : public Component
{
public:
	ISA8BitBus() : _interrupt(false), _interruptrdy(false)
	{
	}
    void simulateCycle()
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
        {
			(*i)->simulateCycle();
			if(_interrupt)
			{
				(*i)->handleInterrupt();
			}
		}
    }
    void addComponent(ISA8BitComponent* component)
    {
        _components.add(component);
        component->setBus(this);
    }
    void setAddress(UInt32 address)
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->setAddress(address);
    }
    void write(UInt8 data)
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->active())
                (*i)->write(data);
    }
    UInt8 read() const
    {
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->active())
                (*i)->read();
        return _data;
    }
    UInt8 memory(UInt32 address)
    {
        UInt8 data = 0xff;
        for (auto i = _components.begin(); i != _components.end(); ++i)
            data &= (*i)->memory(address);
        return data;
    }
    String save()
    {
        String s("bus: {");
        bool needComma = false;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            if ((*i)->name().empty())
                continue;
            if (needComma)
                s += ", ";
            needComma = true;
            s += (*i)->save();
        }
        return s + "}";
    }
    Type type()
    {
        List<StructuredType::Member> members;
        for (auto i = _components.begin(); i != _components.end(); ++i) {
            Type type = (*i)->type();
            if (!type.valid())
                continue;
            members.add(StructuredType::Member((*i)->name(), (*i)->type()));
        }
        return StructuredType("Bus", members);
    }
    void load(const TypedValue& value)
    {
        Value<HashTable<String, TypedValue> > object =
            value.value<Value<HashTable<String, TypedValue> > >();
        for (auto i = _components.begin(); i != _components.end(); ++i)
            (*i)->load((*object)[(*i)->name()]);
    }
    String name() { return "bus"; }
    TypedValue initial()
    {
        Value<HashTable<String, TypedValue> > object;
        for (auto i = _components.begin(); i != _components.end(); ++i)
            if ((*i)->type().valid())
                object->operator[]((*i)->name()) = (*i)->initial();
        return TypedValue(type(), object);
    }

	UInt8 _interruptnum;
	bool _interrupt;
	bool _interruptrdy;

private:
    List<ISA8BitComponent*> _components;
    UInt8 _data;

    template<class T> friend class ISA8BitComponentTemplate;
};

#endif