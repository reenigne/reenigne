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

class ThreadPool;

template<class T> class TaskT;
typedef TaskT<void> Task;

template<class T> class TaskThreadT;
typedef TaskThreadT<void> TaskThread;

template<class T> class TaskT : public LinkedListMember<Task>
{
public:
    TaskT() : _state(completed) { }
    ~TaskT() { join(); }
    void setPool(ThreadPool* threadPool)
    {
        _threadPool = threadPool;
        _threadPool->addCompleted(this);
    }

    // Cancel task and remove from pool as quickly as possible.
    void cancel() { _threadPool->cancel(this); }

    // Wait for task to complete.
    void join() { _threadPool->join(this); }

    // If task is not running, start it. If task is running, cancel it and then
    // start it again.
    void restart() { _threadPool->restart(this); }

    // Same as restart(), but waits for previous instance of the task to stop
    // running before continuing.
    void restartSynchronous() { _threadPool->restartSynchronous(this); }

protected:
    bool cancelling() { return _threadPool->cancelling(this); }
private:
    virtual void run() = 0;

    ThreadPool* _threadPool;
    TaskThread* _thread;
    enum State {
        waiting,
        running,
        cancelPending,
        restartPending,
        completed
    };
    State _state;

    template<class U> friend class TaskThreadT;
    friend class ThreadPool;
};

template<class T> class TaskThreadT : public Thread
{
public:
    TaskThreadT() : _next(0) { }
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

    TaskThread* _next;
    ThreadPool* _threadPool;
    Task* _task;
    Event _go;
    friend class ThreadPool;
};

class ThreadPool
{
public:
    ThreadPool(int threads = 0)
    {
        if (threads == 0) {
            // Count available threads
            DWORD_PTR pam, sam;
            IF_ZERO_THROW(
                GetProcessAffinityMask(GetCurrentProcess(), &pam, &sam));
            for (DWORD_PTR p = 1; p != 0; p <<= 1)
                if ((pam&p) != 0)
                    ++threads;
        }
        _threads.allocate(threads);
        for (int i = 0; i < threads; ++i) {
            _threads[i]._threadPool = this;
            startTask(&_threads[i], 0);
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
            _done.wait();
        } while (true);
        // End all the threads
        for (int i = 0; i < _threads.count(); ++i) {
            _threads[i].go();
            _threads[i].join();
        }
    }

    // Removes all queued tasks and cancels all running tasks,
    void abandon()
    {
        Lock lock(&_mutex);
        Task* t = _waiting.getNext();
        while (t != 0) {
            Task* next = _waiting.getNext(t);
            t->remove();
            t = next;
        }
        for (int i = 0; i < _threads.count(); ++i) {
            Task* task = _threads[i]._task;
            if (task != 0)
                task->_state = Task::cancelPending;
        }
    }

    // Waits for task to complete.
    void join(Task* task)
    {
        do {
            {
                Lock lock(&_mutex);
                if (task->_state == Task::completed)
                    return;
            }
            _done.wait();
        } while (true);
    }

    void restart(Task* task)
    {
        Lock lock(&_mutex);
        if (task->_state == Task::completed) {
            task->remove();
            addNoLock(task);
        }
        else {
            if (task->_state != Task::waiting)
                task->_state = Task::restartPending;
        }
    }

    void restartSynchronous(Task* task)
    {
        restart(task);
        do {
            {
                Lock lock(&_mutex);
                if (task->_state != Task::restartPending)
                    return;
            }
            _done.wait();
        } while (true);
    }

    void cancel(Task* task)
    {
        Lock lock(&_mutex);
        if (task->_state == Task::waiting)
            task->remove();
        else {
            if (task->_state != Task::completed)
                task->_state = Task::cancelPending;
        }
    }

    bool cancelling(Task* task)
    {
        Lock lock(&_mutex);
        return task->_state == Task::cancelPending;
    }

    // Called by thread when it has completed its task
    void taskCompleted(TaskThread* thread)
    {
        Lock lock(&_mutex);
        Task* task = thread->_task;
        if (task->_state != task->Task::restartPending) {
            task->_state = Task::completed;
            _completed.add(task);
            task = _waiting.getNext();
            task->remove();
        }
        startTask(thread, task);
    }

    Task* getCompletedTask()
    {
        Lock lock(&_mutex);
        Task* task = _completed.getNext();
        if (task != 0)
            task->remove();
        return task;
    }

    void setPriority(int nPriority)
    {
        for (int i = 0; i < _threads.count(); ++i)
            _threads[i].setPriority(nPriority);
    }

    void addCompleted(Task* task) { _completed.add(task); }
private:
    void addNoLock(Task* task)
    {
        TaskThread* thread = _idle;
        if (thread == 0) {
            _waiting.add(task);
            return;
        }
        _idle = thread->_next;
        startTask(thread, task);
    }
    void startTask(TaskThread* thread, Task* task)
    {
        thread->_task = task;
        if (task == 0) {
            thread->_next = _idle;
            _idle = thread;
        }
        else {
            task->_state = Task::running;
            thread->go();
        }
        _done.signal();
    }

    TaskThread* _idle;
    Mutex _mutex;
    Event _done;
    LinkedList<Task> _waiting;
    LinkedList<Task> _completed;
    Array<TaskThread> _threads;
};

// A ThreadTask has a single thread all to itself.
class ThreadTask : public Task
{
public:
    ThreadTask() : _threadPool(1) { setPool(&_threadPool); }
    void setPriority(int nPriority) { _threadPool.setPriority(nPriority); }
private:
    ThreadPool _threadPool;
};

#endif // INCLUDED_LOCK_H
