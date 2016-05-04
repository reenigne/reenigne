#include "alfe/main.h"
#include "alfe/thread.h"
#ifdef WIN32
#define ZLIB_WINAPI
#endif
#include "zlib.h"
#include <conio.h>

static const int samplesPerFrame = 450*1024;

class Program;

class Frame : public Handle::Body
{
public:
    Frame(int index, int bytes) : _data(bytes), _index(index) { }
    Byte* data() { return &_data[0]; }
    int bytes() { return _data.count(); }
    int index() { return _index; }
private:
    Array<Byte> _data;
    int _index;
};

class Queue
{
public:
    Queue()
      : _data(1), _addIndex(0), _addOffset(0), _removeOffset(0), _count(0) { }
    void add(Handle frame)
    {
        Lock l(&_mutex);
        int index = frame->index() - _addIndex;
        int items = (_addOffset + _data.count() - _removeOffset) & mask();
        while (_data.count() <= items + index + 1) {
            Array<Handle> data(_data.count() * 2);
            for (int i = 0; i < _data.count(); ++i)
                data[i] = _data[(i + _removeOffset) & mask()];
            _data.swap(data);
            _addOffset = items;
            _removeOffset = 0;
        }
        _data[(_addOffset + index) & mask()] = frame;
        while (_data[_addOffset].valid()) {
            _addOffset = (_addOffset + 1) & mask();
            ++_addIndex;
        }
        ++_count;
    }
    Handle remove()
    {
        Lock l(&_mutex);
        if (_removeOffset == _addOffset)
            return Handle();
        if (!_data[_removeOffset].valid())
            return Handle();
        Handle frame = _data[_removeOffset];
        _data[_removeOffset] = Handle();
        _removeOffset = (_removeOffset + 1) & mask();
        --_count;
        return frame;
    }
    //void doDump()
    //{
    //    for (int i = 0; i < _data.count(); ++i)
    //        if (_data[i].valid())
    //            console.write(String(decimal(_data[i]->index())) + " ");
    //        else
    //            console.write(String("- "));
    //    console.write(String(" = ") + decimal(_addIndex) + ", " + decimal(_addOffset) + ", " + decimal(_removeOffset) + "\n");
    //}
    int count() { Lock l(&_mutex); return _count; }
private:
    int mask() { return _data.count() - 1; }

    Array<Handle> _data;
    int _addIndex;     // Index of frame at _addOffset
    int _addOffset;    // Offset into _data
    int _removeOffset; // Offset into _data
    Mutex _mutex;
    int _count;
};

template<class T> class CompressTaskT : public Task
{
public:
    CompressTaskT(Program* program) : _program(program)
    {
        memset(&_zs, 0, sizeof(z_stream));
        int r = deflateInit(&_zs, 4);  // or Z_DEFAULT_COMPRESSION?
        if (r != Z_OK)
            throw Exception("deflateInit failed");
        int bufferSize = deflateBound(&_zs, samplesPerFrame);
        buffer.allocate(bufferSize);
    }
    ~CompressThreadT() { deflateEnd(&_zs); }
    void setProgram(Program* program) { _program = program; }
    void setFrame(Handle frame) { _frame = frame; restart(); }
private:
    void run()
    {
        int bufferSize = _buffer.size();
        _zs.avail_in = samplesPerFrame;
        _zs.next_in = _frame->data();
        _zs.avail_out = bufferSize;
        _zs.next_out = &_buffer[0];

        r = deflate(&_zs, Z_FINISH);
        if (r != Z_STREAM_END)
            throw Exception("deflate failed");

        r = deflateReset(&_zs);
        if (r != Z_OK)
            throw Exception("deflateReset failed");

        int compressedSize = bufferSize - _zs.avail_out;
        Handle compressed = new Frame(_frame->index(), compressedSize);
        memcpy(compressed->data(), &_buffer[0], compressedSize);
        _program->putCompressedFrame(compressed);
    }

    Handle _frame;
    z_stream _zs;
    Array<Byte> _buffer;
    Program* _program;
};

typedef CompressTaskT<void> CompressTask;

template<class T> class WriteThreadT : public ThreadTask
{
public:
    WriteThreadT() : _program(0) { }
    void setProgram(Program* program) { _program = program; restart(); }
private:
    void run()
    {
        do {
            Handle frame = _program->getCompressedFrame();
            if (!frame.valid())
                return;
            _program->write(frame);
        } while (!cancelling());
    }

    Program* _program;
    Event _go;
    bool _ending;
};

typedef WriteThreadT<void> WriteThread;

class Program : public ProgramBase
{
public:
    void run()
    {
        _writeThread.setProgram(this);

        String name = "captured.zdr";
        if (_arguments.count() >= 2)
            name = _arguments[1];
        Stream h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);

        _outputStream = File(name, true).openWrite();

        int k = 0;
        int index = 0;

        do {
            Handle uncompressed = new Frame(index, samplesPerFrame);
            Byte* data = uncompressed->data();
            int remaining = samplesPerFrame;
            do {
                DWORD bytesRead;
                BOOL r = ReadFile(h, data + samplesPerFrame - remaining,
                    remaining, &bytesRead, NULL);
                if (r == 0)
                    throw Exception::systemError("Reading VBICap pipe");
                remaining -= bytesRead;
            } while (remaining > 0);

            CompressTask* task = _compressPool.getCompletedTask();
            if (task == 0) {
                task = new CompressTask(this);
                task->setPool(&_compressPool);
                _tasks.add(task);
            }
            task->setFrame(uncompressed);

            ++index;
            if (index % 60 == 0)
                printf(".");
            if (_kbhit())
                k = _getch();
        } while (k != 27);
    }
    void write(Handle frame)
    {
        _outputStream.write(frame->data(), frame->bytes());
    }
    Handle getCompressedFrame()
    {
        return _compressedFrames.remove();
        //if (frame.valid())
        //    console.write(String("Writing frame ") + decimal(frame->index()) + "\n");
        //else
        //    console.write(String("No frame\n"));
        //_compressedFrames.dump();
        //return frame;
    }
    void putCompressedFrame(Handle frame)
    {
        //console.write(String("Completed frame ") + decimal(frame->index()) + "\n");
        _compressedFrames.add(frame);
        //_compressedFrames.dump();
        _writeThread.restart();
    }
    ~Program()
    {
        for (auto t : _tasks)
            delete t;
    }
private:
    Queue _compressedFrames;
    WriteThread _writeThread;
    ThreadPool _compressPool;
    AutoStream _outputStream;
    List<CompressTask*> _tasks;
};
