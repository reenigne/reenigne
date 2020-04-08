#ifndef INCLUDED_IO_DISPATCH_H
#define INCLUDED_IO_DISPATCH_H

class IODispatch : Uncopyable
{
public:
    IODispatch() : _n(0) { }
    class Task : Uncopyable
    {
    public:
        virtual void signalled() = 0;
        virtual WindowsHandle handle() = 0;
        void remove() { _dispatch->remove(this); }
    private:
        void setDispatch(IODispatch* dispatch) { _dispatch = dispatch; }
        IODispatch* _dispatch;
        friend class IODispatch;
    };

    void run()
    {
        while (_n > 0) {
            int r = WaitForMultipleObjects(_n, &_handles[0], FALSE, INFINITE);
            _tasks[r]->signalled();
        }
    }
    void add(Task* task)
    {
        if (_handles.count() > _n) {
            _handles[_n] = task->handle();
            _tasks[_n] = task;
        }
        else {
            _handles.append(task->handle());
            _tasks.append(task);
        }
        task->setDispatch(this);
        ++_n;
    }
    void remove(Task* task)
    {
        for (int i = 0; i < _n; ++i) {
            if (_tasks[i] == task) {
                swap(_tasks[i], _tasks[_n - 1]);
                swap(_handles[i], _handles[_n - 1]);
                --_n;
                break;
            }
        }
    }
private:
    AppendableArray<HANDLE> _handles;
    AppendableArray<Task*> _tasks;
    int _n;
};

#endif // INCLUDED_IO_DISPATCH_H
