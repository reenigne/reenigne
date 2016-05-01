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
    Task() : _cancelling(true) { }
    ~Task() { join(); }
    void setPool(ThreadPool* threadPool) { _threadPool = threadPool; }

    // Cancel task and remove from pool as quickly as possible.
    void cancel() { _threadPool->cancel(this); }
    void join() { _threadPool->join(this); }

    // If task is not running, start it. If task is running, cancel it and then
    // start it.
    void restart() { _threadPool->restart(this); }

protected:
    bool cancelling() { return _threadPool->cancelling(this); }
private:
    virtual void run() = 0;

    ThreadPool* _threadPool;
    TaskThread* _thread;
    bool _cancelling;
    bool _restarting;
    friend class TaskThread;
    friend class ThreadPool;
};

class TaskThread : public Thread
{
public:
    TaskThread() : _nextIdle(0) { } // : _ending(false), _currentTask(0) { start(); }
private:
    void go() { _go.signal(); }
    void threadProc()
    {
        do {
            _go.wait();
            if (_task == 0)
                return;
            _task->run();
            _threadPool->taskCompleted(this);
        } while (true);
    }

    TaskThread* _nextIdle;
    ThreadPool* _threadPool;
    Task* _task;
    Event _go;
    friend class ThreadPool;
};

class ThreadPool
{
public:
    ThreadPool(int threads) : _threads(threads)
    {
        for (int i = 0; i < threads; ++i) {
            _threads[i]._threadPool = this;
            idlePush(&_threads[i]);
            _threads[i].start();
        }
    }
    ~ThreadPool()
    {
        // Wait until queue is empty and all threads are idle.
        do {
            {
                Lock lock(&_mutex);
                int i;
                for (i = 0; i < _threads.count(); ++i) {
                    if (_threads[i]._task != 0)
                        break;
                }
                if (i == _threads.count())
                    return;
            }
            _completed.wait();
        } while (true);
        // End all the threads
        for (int i = 0; i < _threads.count(); ++i) {
            _threads[i].go();
            _threads[i].join();
        }
    }

    // Cancels all tasks and removes all queued tasks.
    void abandon()
    {
        Lock lock(&_mutex);
        Task* t = _tasks.getNext();
        while (t != 0) {
            Task* next = _tasks.getNext(t);
            t->remove();
            t = next;
        }
        for (int i = 0; i < _threads.count(); ++i) {
            Task* task = _threads[i]._task;
            if (task != 0) {
                task->_cancelling = true;
                task->_restarting = false;
            }
        }
    }

    // Adds a task to the pool. If there is an idle thread it will begin
    // executing immediately.
    void add(Task* task)
    {
        Lock lock(&_mutex);
        TaskThread* thread = idlePop();
        if (thread == 0)
            _tasks.add(task);
        else {
            thread->_task = task;
            thread->go();
        }
    }

    // Waits for task to complete.
    void join(Task* task)
    {
        do {
            {
                Lock lock(&_mutex);
                if (task->)
                int i;
                for (i = 0; i < _threads.count(); ++i) {
                    if (_threads[i]._task == task)
                        break;
                }
                if (i == _threads.count()) {
                    Task* t = _tasks.getNext();
                    while (t != 0) {
                        if (t == task)
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

    void restart(Task* task)
    {
        Lock lock(&_mutex);

        // TODO
    }

    void cancel(Task* task)
    {
        Lock lock(&_mutex);
        if (task->_thread->_task == task) {
            task->_cancelling = true;
            task->_restarting = false;
        }
        else
            task->remove();
    }

    bool cancelling(Task* task)
    {
        Lock lock(&_mutex);
        return task->_cancelling;
    }

    // Called by thread when it has completed its task
    void taskCompleted(TaskThread* thread)
    {
        Lock lock(&_mutex);
        if (!thread->_task->_restarting) {
            Task* task = _tasks.getNext();
            task->remove();
            thread->_task = task;
            if (task != 0) {
                task->_thread = thread;
                thread->go();
            }
            else
                idlePush(thread);
            _completed.signal();
        }
        else {
            thread->_task->_restarting = false;
            thread->go();
        }
    }
private:
    TaskThread* idlePop()
    {
        if (_idle == 0)
            return 0;
        TaskThread* thread = _idle;
        _idle = thread->_nextIdle;
        return thread;
    }
    void idlePush(TaskThread* thread)
    {
        thread->_nextIdle = thread;
        _idle = thread;
    }

    TaskThread* _idle;
    Mutex _mutex;
    Event _completed;
    LinkedList<Task> _tasks;
    Array<TaskThread> _threads;
};

// A ThreadTask has a single thread all to itself.
class ThreadTask : public Task
{
public:
    ThreadTask() : _threadPool(1) { setPool(this); _threadPool.add(this); }
private:
    ThreadPool _threadPool;
};

#endif // INCLUDED_LOCK_H
