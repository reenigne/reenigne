#include "alfe/main.h"
#include "alfe/thread.h"
#ifdef WIN32
#define ZLIB_WINAPI
#endif
#include "zlib.h"
#include <conio.h>

static const int samplesPerFrame = 450*1024;

class Program;

class Frame : public ReferenceCounted
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
    void add(Reference<Frame> frame)
    {
        Lock l(&_mutex);
        int index = frame->index() - _addIndex;
        int items = (_addOffset + _data.count() - _removeOffset) & mask();
        while (_data.count() <= items + index + 1) {
            Array<Reference<Frame>> data(_data.count() * 2);
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
    Reference<Frame> remove()
    {
        Lock l(&_mutex);
        if (_removeOffset == _addOffset)
            return Reference<Frame>();
        if (!_data[_removeOffset].valid())
            return Reference<Frame>();
        Reference<Frame> frame = _data[_removeOffset];
        _data[_removeOffset] = Reference<Frame>();
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

    Array<Reference<Frame>> _data;
    int _addIndex;     // Index of frame at _addOffset
    int _addOffset;    // Offset into _data
    int _removeOffset; // Offset into _data
    Mutex _mutex;
    int _count;
};

template<class T> class CompressThreadTemplate : public Thread
{
public:
    CompressThreadTemplate() : _ending(false), _waiting(false) { }
    void setProgram(Program* program) { _program = program; }
    void go() { _go.signal(); }
    void end() { _ending = true; go(); }
    bool waiting() { return _waiting; }
private:
    void threadProc()
    {
        z_stream zs;
        memset(&zs, 0, sizeof(z_stream));
        int r = deflateInit(&zs, 4);  // or Z_DEFAULT_COMPRESSION?
        if (r != Z_OK)
            throw Exception("deflateInit failed");
        int bufferSize = deflateBound(&zs, samplesPerFrame);
        Array<Byte> buffer(bufferSize);

        while (!_ending) {
            _waiting = true;
            _go.wait();
            _waiting = false;
            while (!_ending) {
                Reference<Frame> frame = _program->getUncompressedFrame();
                if (!frame.valid())
                    break;

                zs.avail_in = samplesPerFrame;
                zs.next_in = frame->data();
                zs.avail_out = bufferSize;
                zs.next_out = &buffer[0];

                r = deflate(&zs, Z_FINISH);
                if (r != Z_STREAM_END)
                    throw Exception("deflate failed");

                r = deflateReset(&zs);
                if (r != Z_OK)
                    throw Exception("deflateReset failed");

                int compressedSize = bufferSize - zs.avail_out;
                Reference<Frame> compressed =
                    new Frame(frame->index(), compressedSize);
                memcpy(compressed->data(), &buffer[0], compressedSize);
                _program->putCompressedFrame(compressed);
            }
        }
        r = deflateEnd(&zs);
        if (r != Z_OK)
            throw Exception("defaultEnd failed");
    }

    Program* _program;
    Event _go;
    bool _ending;
    bool _waiting;
};

typedef CompressThreadTemplate<void> CompressThread;

template<class T> class WriteThreadTemplate : public Thread
{
public:
    WriteThreadTemplate() : _ending(false) { }
    void setProgram(Program* program) { _program = program; }
    void go() { _go.signal(); }
    void end() { _ending = true; go(); }
private:
    void threadProc()
    {
        while (!_ending) {
            _go.wait();
            while (!_ending) {
                Reference<Frame> frame = _program->getCompressedFrame();
                if (!frame.valid())
                    break;
                _program->write(frame);
            }
        }
    }

    Program* _program;
    Event _go;
    bool _ending;
};

typedef WriteThreadTemplate<void> WriteThread;

class Program : public ProgramBase
{
public:
    void run()
    {
        // Count available threads
        DWORD_PTR pam, sam;
        int nThreads = 0;
        IF_ZERO_THROW(GetProcessAffinityMask(GetCurrentProcess(), &pam, &sam));
        for (DWORD_PTR p = 1; p != 0; p <<= 1)
            if ((pam&p) != 0)
                ++nThreads;
        _compressThreads.allocate(nThreads);
        for (int i = 0; i < nThreads; ++i) {
            _compressThreads[i].setProgram(this);
            _compressThreads[i].start();
            _compressThreads[i].go();
        }
        _writeThread.setProgram(this);
        _writeThread.start();
        _writeThread.go();

        String name = "captured.zdr";
        if (_arguments.count() >= 2)
            name = _arguments[1];
        Handle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);

        _outputHandle = File(name, true).openWrite();

        int k = 0;
        int index = 0;

        do {
            Reference<Frame> uncompressed = new Frame(index, samplesPerFrame);
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

            _uncompressedFrames.add(uncompressed);
            wakeACompressionThread();
            
            ++index;
            if (index % 60 == 0)
                printf(".");
            if (_kbhit())
                k = _getch();
        } while (k != 27);

        while (_uncompressedFrames.count() > 0)
            wakeACompressionThread();
        while (_compressedFrames.count() > 0)
            _writeThread.go();

        _writeThread.end();
        for (int i = 0; i < nThreads; ++i)
            _compressThreads[i].end();
    }
    void write(Reference<Frame> frame)
    {
        _outputHandle.write(frame->data(), frame->bytes());
    }
    Reference<Frame> getUncompressedFrame()
    {
        return _uncompressedFrames.remove();
    }
    Reference<Frame> getCompressedFrame()
    {
        return _compressedFrames.remove();
        //if (frame.valid())
        //    console.write(String("Writing frame ") + decimal(frame->index()) + "\n");
        //else
        //    console.write(String("No frame\n"));
        //_compressedFrames.dump();
        //return frame;
    }
    void putCompressedFrame(Reference<Frame> frame)
    {
        //console.write(String("Completed frame ") + decimal(frame->index()) + "\n");
        _compressedFrames.add(frame);
        //_compressedFrames.dump();
        _writeThread.go();
    }
private:
    void wakeACompressionThread()
    {
        for (int i = 0; i < _compressThreads.count(); ++i)
            if (_compressThreads[i].waiting()) {
                _compressThreads[i].go();
                break;
            }
    }
    Queue _uncompressedFrames;
    Queue _compressedFrames; 
    WriteThread _writeThread;
    Array<CompressThread> _compressThreads;
    AutoHandle _outputHandle;
};
