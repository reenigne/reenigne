#ifndef INCLUDED_LOCK_H
#define INCLUDED_LOCK_H

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

4class Event : public AutoHandle
{
public:
    Event() : AutoHandle(CreateEvent(NULL, TRUE, FALSE, NULL))
    { }
    void set() { IF_ZERO_THROW(SetEvent(_handle)); }
    void reset() { IF_ZERO_THROW(ResetEvent(_handle)); }
    void wait() { IF_FALSE_THROW(WaitForSingleObject(_handle, INFINITE)==0); }
};

#endif // INCLUDED_LOCK_H
