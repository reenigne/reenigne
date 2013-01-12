#include "alfe/main.h"

#ifndef INCLUDED_LOCK_H
#define INCLUDED_LOCK_H

// TODO: Posix version

class Event : public Handle
{
public:
    Event()
    {
        HANDLE handle = CreateEvent(NULL, FALSE, FALSE, NULL);
        IF_NULL_THROW(handle);
        Handle::operator=(AutoHandle(handle));
    }
    void signal() { IF_ZERO_THROW(SetEvent(operator HANDLE())); }
    void wait()
    {
        IF_FALSE_THROW(WaitForSingleObject(operator HANDLE(), INFINITE) == 0);
    }
    void reset()
    {
        IF_ZERO_THROW(ResetEvent(operator HANDLE()));
    }
};

class Thread : public AutoHandle
{
public:
    Thread() : _started(false), _error(false)
    {
        HANDLE handle = CreateThread(
            NULL, 0, threadStaticProc, this, CREATE_SUSPENDED, NULL);
        IF_NULL_THROW(handle);
        Handle::operator=(AutoHandle(handle));
    }
    ~Thread() { noFailJoin(); }
    void setPriority(int nPriority)
    {
        IF_ZERO_THROW(SetThreadPriority(operator HANDLE(), nPriority));
    }
    void noFailJoin()
    {
        if (!_started)
            return;
        _started = false;
        WaitForSingleObject(operator HANDLE(), INFINITE);
    }
    void join()
    {
        if (!_started)
            return;
        _started = false;
        IF_FALSE_THROW(
            WaitForSingleObject(operator HANDLE(), INFINITE) == WAIT_OBJECT_0);
        if (_error)
            throw _exception;
    }
    void start() { IF_MINUS_ONE_THROW(ResumeThread(operator HANDLE())); }

private:
    static DWORD WINAPI threadStaticProc(LPVOID lpParameter)
    {
        reinterpret_cast<Thread*>(lpParameter)->process();
        return 0;
    }
    void process()
    {
        _started = true;
        BEGIN_CHECKED {
            threadProc();
        } END_CHECKED(Exception& e) {
            _exception = e;
            _error = true;
        }
    }

    virtual void threadProc() { }

    bool _started;
    bool _error;
    Exception _exception;
};

class Mutex : Uncopyable
{
public:
    Mutex() { InitializeCriticalSection(&_cs); }
    ~Mutex() { DeleteCriticalSection(&_cs); }
    void lock() { EnterCriticalSection(&_cs); }
    void unlock() { LeaveCriticalSection(&_cs); }
    bool tryLock() { return TryEnterCriticalSection(&_cs) != 0; }
private:
    CRITICAL_SECTION _cs;
};

class Lock : Uncopyable
{
public:
    Lock() : _mutex(0) { }
    Lock(Mutex* mutex) : _mutex(mutex) { _mutex->lock(); }

    ~Lock()
    {
        if (_mutex)
            _mutex->unlock();
    }

    bool tryAcquire(Mutex* mutex)
    {
        if (mutex->tryLock()) {
            _mutex = mutex;
            return true;
        }
        return false;
    }

private:
    Mutex* _mutex;
};


#endif // INCLUDED_LOCK_H
