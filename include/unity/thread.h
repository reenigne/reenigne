#ifndef INCLUDED_LOCK_H
#define INCLUDED_LOCK_H

// TODO: Posix version

class Event : public AutoHandle
{
public:
    Event() : AutoHandle(CreateEvent(NULL, FALSE, FALSE, NULL))
    { }
    void signal() { IF_ZERO_THROW(SetEvent(_handle)); }
    void wait() { IF_FALSE_THROW(WaitForSingleObject(_handle, INFINITE)==0); }
};

class Thread : public AutoHandle
{
public:
    Thread()
      : _started(false),
        _error(false)
    {
        AutoHandle::set(CreateThread(
            NULL, 0, threadStaticProc, this, CREATE_SUSPENDED, NULL));  // TODO: Make sure our AutoHandle is sufficient for this
    }
    void setPriority(int nPriority)
    {
        IF_ZERO_THROW(SetThreadPriority(_handle, nPriority));
    }
    void join()
    {
        if (!_started)
            return;
        IF_FALSE_THROW(
            WaitForSingleObject(_handle, INFINITE) == WAIT_OBJECT_0);
        if (_error)
            throw _exception;
    }
    void start() { IF_MINUS_ONE_THROW(ResumeThread(_handle)); }

private:
    static DWORD WINAPI threadStaticProc(LPVOID lpParameter)
    {
        BEGIN_CHECKED {
            Thread* thread = reinterpret_cast<Thread*>(lpParameter);
            thread->_started = true;
            thread->threadProc();
        } END_CHECKED(Exception& e) {
            _exception = e;  // TODO: Make sure Exception can be copied safely
            _error = true;
        }
        return 0;
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
