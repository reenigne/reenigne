#include "alfe/main.h"
#include "alfe/thread.h"
#ifdef WIN32
#define ZLIB_WINAPI
#endif
#include "zlib.h"
#include <conio.h>

static const int samplesPerFrame = 450*1024;

class Program;

template<class T> class CompressThreadTemplate : public Thread
{
public:
    void setProgram(Program* program) { _program = program; }
private:
    void threadProc()
    {
        bool ending;
        do {
            ending = _program->compress();
        } while (!ending);
    }

    Program* _program;
};

typedef CompressThreadTemplate<void> CompressThread;

template<class T> class WriteThreadTemplate : public Thread
{
public:
    void setProgram(Program* program) { _program = program; }
private:
    void threadProc()
    {
        bool ending;
        do {
            ending = _program->write();
        } while (!ending);
    }

    Program* _program;
};

typedef WriteThreadTemplate<void> WriteThread;

class Frame : public ReferenceCounted
{
    Byte* getData() { }
    Array<Byte> _data;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        String name = "captured.zdr";
        if (_arguments.count() >= 2)
            name = _arguments[1];
        Handle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);

        AutoHandle oh = File(name, true).openWrite();

        Array<Byte> samples(samplesPerFrame*2);
        int k = 0;
        int bytes = 0;
        int frames = 0;
        int offset = 0;
        Byte* newSamples = &samples[offset];
        memset(&samples[0], 0, samplesPerFrame);
        //Array<Byte> delta(samplesPerFrame);

        z_stream zs;
        memset(&zs, 0, sizeof(z_stream));
        int r = deflateInit(&zs, 4);  // or Z_DEFAULT_COMPRESSION?
        if (r != Z_OK)
            throw Exception("deflateInit failed");

        int outputSize = deflateBound(&zs, samplesPerFrame);
        Array<Byte> compressed(outputSize);
        do {
            //Byte* oldSamples = &samples[offset];
            //offset = samplesPerFrame - offset;
            //Byte* newSamples = &samples[offset];

            int remaining = samplesPerFrame;
            do {
                DWORD bytesRead;
                BOOL r = ReadFile(h, newSamples + samplesPerFrame - remaining,
                    remaining, &bytesRead, NULL);
                if (r == 0)
                    throw Exception::systemError("Reading VBICap pipe");
                remaining -= bytesRead;
            } while (remaining > 0);

            //for (int i = 0; i < samplesPerFrame; ++i)
            //    delta[i] = newSamples[i] - oldSamples[i];

            zs.avail_in = samplesPerFrame;
            zs.next_in = newSamples; //&delta[0];
            zs.avail_out = outputSize;
            zs.next_out = &compressed[0];

            r = deflate(&zs, Z_NO_FLUSH);
            if (r != Z_OK)
                throw Exception("deflate failed");

            oh.write(&compressed[0], outputSize - zs.avail_out);
            
            ++frames;
            if (frames % 60 == 0)
                printf(".");
            if (_kbhit())
                k = _getch();
        } while (k != 27);

        do {
            zs.avail_in = 0;
            zs.next_in = newSamples; //&delta[0];
            zs.avail_out = outputSize;
            zs.next_out = &compressed[0];

            r = deflate(&zs, Z_FINISH);
            if (r == Z_STREAM_END)
                break;
            if (r != Z_OK)
                throw Exception("deflate failed");

            oh.write(&compressed[0], outputSize - zs.avail_out);
        } while (true);
        r = deflateEnd(&zs);
        if (r != Z_OK)
            throw Exception("defaultEnd failed");
    }

    bool compress()
    {
        return true;
    }

    bool write()
    {
        return true;
    }

    Mutex _mutex;
    Array<Reference<Frame>> _uncompressedFrames;
    Array<Reference<Frame>> _compressedFrames; 
    int _nextToRead;
    int _nextToCompress;
    int _nextToWrite;
    WriteThread _writeThread;
    CompressThread _compressThreads[8];
};
