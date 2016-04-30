#include "alfe/main.h"

#ifndef INCLUDED_LOCK_H
#define INCLUDED_LOCK_H

#include "alfe/windows_handle.h"
#include "alfe/linked_list.h"

class Event : public WindowsHandle
{
public:
    Event()
    {
        HANDLE handle = CreateEvent(NULL, FALSE, FALSE, NULL);
        IF_NULL_THROW(handle);
        WindowsHandle::operator=(handle);
    }
    Event(HANDLE handle) : WindowsHandle(handle) { }
    void signal() { IF_ZERO_THROW(SetEvent(*this)); }
    bool wait(DWORD time = INFINITE)
    {
        DWORD r = WaitForSingleObject(*this, time);
        if (r == 0)
            return true;
        IF_FALSE_THROW(r == WAIT_TIMEOUT);
        return false;
    }
    void reset() { IF_ZERO_THROW(ResetEvent(*this)); }
};

class Thread : public WindowsHandle
{
public:
    Thread() : _started(false), _error(false)
    {
        HANDLE handle = CreateThread(
            NULL, 0, threadStaticProc, this, CREATE_SUSPENDED, NULL);
        IF_NULL_THROW(handle);
        WindowsHandle::operator=(handle);
    }
    ~Thread() { noFailJoin(); }
    void setPriority(int nPriority)
    {
        IF_ZERO_THROW(SetThreadPriority(*this, nPriority));
    }
    void noFailJoin()
    {
        if (!_started)
            return;
        _started = false;
        WaitForSingleObject(*this, INFINITE);
    }
    void join()
    {
        if (!_started)
            return;
        _started = false;
        IF_FALSE_THROW(WaitForSingleObject(*this, INFINITE) == WAIT_OBJECT_0);
        if (_error)
            throw _exception;
    }
    void start() { IF_MINUS_ONE_THROW(ResumeThread(*this)); }

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

class Task : public LinkedListMember<Task>
{
public:
    void enqueue(TaskThread* thread)
    {
        _thread = thread;
        thread->enqueueTask(this);
    }
    void cancel()
    {
        if (_thread == 0) {
            // We're not enqueued on a thread - return
            return;
        }
        _thread->cancelTask(this);
    }
    void join()
    {
        if (_thread == 0) {
            // We're not enqueued on a thread - already complete
            return;
        }
        _thread->waitForTaskComplete(this);
    }

protected:
    bool cancelling() { return _thread->cancelling(); }
private:
    virtual void run() = 0;
    void doRun() { run(); _thread = 0; }

    TaskThread* _thread;
    friend class TaskThread;
};

class TaskThread : public Thread
{
public:
    TaskThread() : _ending(false), _currentTask(0) { start(); }
    void enqueueTask(Task* task)
    {
        Lock lock(&_mutex);
        _tasks.add(task);
        go();
    }
    void cancelTask(Task* task)
    {
        {
            Lock lock(&_mutex);
            if (task != _currentTask) {
                task->remove();
                return;
            }
            _cancelling = true;
        }
        _completed.wait();
    }
    void waitForTaskComplete(Task* task)
    {
        do {
            {
                Lock lock(&_mutex);
                if (task != _currentTask) {
                    Task* t = _tasks.getNext();
                    while (t != 0) {
                        if (t == _currentTask)
                            break;
                        t = _tasks.getNext(t);
                    }
                    if (t == 0)
                        return;
                }
            }
            _completed.wait();
        } while (true);
    }
    void end()
    {
        {
            Lock lock(&_mutex);
            _ending = true;
            _cancelling = true;
            go();
        }
        join();
    }
    bool cancelling() { return _cancelling; }  // For use by Task::cancelling()
private:
    void go()
    {
        if (_currentTask != 0)
            _go.signal();
    }
    void threadProc()
    {
        do {
            _go.wait();
            do {
                {
                    Lock lock(&_mutex);
                    _completed.signal();
                    if (_ending)
                        return;
                    _currentTask = _tasks.getNext();
                    if (_currentTask != 0) {
                        _currentTask->remove();
                        _cancelling = false;
                    }
                    else
                        break;
                }
                _currentTask->doRun();
            } while (true);
        } while (true);
    }
    LinkedList<Task> _tasks;
    Task* _currentTask;
    Mutex _mutex;
    Event _completed;
    Event _go;
    bool _ending;
    bool _cancelling;
};

class ThreadPool
{
public:
    ThreadPool(int threads) { }

};

#endif // INCLUDED_LOCK_H
