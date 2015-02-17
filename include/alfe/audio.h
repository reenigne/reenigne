#include "alfe/main.h"

#ifndef INCLUDED_AUDIO_H
#define INCLUDED_AUDIO_H

// TODO: Posix port

#include "alfe/thread.h"
#include <mmreg.h>
#include <dsound.h>
#include <xaudio2.h>
#include <mmsystem.h>
#include "alfe/com.h"
#include "alfe\pipes.h"

template<class Sample> class AudioSink : public Sink<Sample>
{
protected:
    AudioSink(int samplesPerSecond, int channels)
      : _channels(channels)
    {
        ZeroMemory(&_format, sizeof(WAVEFORMATEX));
        _format.wFormatTag = WAVE_FORMAT_PCM;
        _format.nChannels = channels;
        _format.nSamplesPerSec = samplesPerSecond;
        int nBlockAlign = channels*sizeof(Sample);
        _format.nAvgBytesPerSec = samplesPerSecond*nBlockAlign;
        _format.nBlockAlign = nBlockAlign;
        _format.wBitsPerSample = sizeof(Sample)*8;
        _format.cbSize = 0;
    }
    WAVEFORMATEX _format;
    int _channels;
};

template<class Sample> class DirectSoundSink : public AudioSink<Sample>
{
    class ProcessingThread : public Thread
    {
    public:
        ProcessingThread() : _ending(false) { }
        void setSink(DirectSoundSink* sink) { _sink = sink; }

        void threadProc()
        {
            while (true) {
                _sink->_event.wait();
                if (_ending)
                    return;
      	        _sink->fillNextHalfBuffer();
            }
        }

        void end()
        {
            _ending = true;
            _sink->_event.signal();
            join();
        }
    private:
        DirectSoundSink* _sink;
        bool _ending;
    };

public:
    // A smaller buffer would probably be preferred but the DirectSound
    // implementation in Vista fails with small buffers - see
    // http://www.reenigne.org/blog/what-happened-to-directsound/ .
    // TODO: Could try instead using a timer and IDirectSoundBuffer::GetCurrentPosition().
    // TODO: Change samplesPerBuffer parameter to secondsPerBuffer?
    DirectSoundSink(HWND hWnd, int samplesPerSecond = 44100,
        int samplesPerBuffer = 4096, int channels = 1)
      : AudioSink(samplesPerSecond, channels),
        _hWnd(hWnd)
    {
        IF_ERROR_THROW(DirectSoundCreate8(NULL, &_directSound, NULL));

        // Set priority cooperative level
        IF_ERROR_THROW(
            _directSound->SetCooperativeLevel(hWnd, DSSCL_PRIORITY));

        _bytesPerSample = sizeof(Sample);

        // Set primary buffer format
        {
            COMPointer<IDirectSoundBuffer> spDSBPrimary;

            DSBUFFERDESC dsbd;
            ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
            dsbd.dwSize = sizeof(DSBUFFERDESC);
            dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
            dsbd.dwBufferBytes = 0;
            dsbd.lpwfxFormat = NULL;

            IF_ERROR_THROW(_directSound->CreateSoundBuffer(
                &dsbd,
                &spDSBPrimary,
                NULL));

            IF_ERROR_THROW(spDSBPrimary->SetFormat(&_format));
        }

        // Set background thread priority
        _thread.setPriority(THREAD_PRIORITY_TIME_CRITICAL);
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

        int bufferBytes = _format.nBlockAlign*samplesPerBuffer;

        DSBUFFERDESC dsbd = {0};
        dsbd.dwSize = sizeof(DSBUFFERDESC);
        dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE |
            DSBCAPS_CTRLPOSITIONNOTIFY;
        dsbd.dwBufferBytes   = bufferBytes;
        dsbd.guid3DAlgorithm = GUID_NULL;
        dsbd.lpwfxFormat     = &_format;

        IF_ERROR_THROW(_directSound->
            CreateSoundBuffer(&dsbd, &_directSoundBuffer, NULL));

        COMPointer<IDirectSoundNotify>
            spDSN(_directSoundBuffer, &IID_IDirectSoundNotify);

        int midpoint = bufferBytes/2;
        midpoint -= midpoint % 2;

        DSBPOSITIONNOTIFY aPosNotify[2];
        _starts[0] = 0;
        _lengths[0] = midpoint;
        _starts[1] = midpoint;
        _lengths[1] = bufferBytes - midpoint;
        aPosNotify[0].dwOffset = midpoint - 1;
        aPosNotify[0].hEventNotify = _event;
        aPosNotify[1].dwOffset = bufferBytes - 1;
        aPosNotify[1].hEventNotify = _event;
        IF_ERROR_THROW(spDSN->SetNotificationPositions(2, aPosNotify));

        _next = 0;
        _thread.setSink(this);
        _thread.start();

        IF_ERROR_THROW(_directSoundBuffer->SetCurrentPosition(0));
    }
    void play()
    {
        fillNextHalfBuffer();
        fillNextHalfBuffer();
        IF_ERROR_THROW(_directSoundBuffer->Play(0, 0, DSBPLAY_LOOPING));
    }

    void fillNextHalfBuffer()
    {
        void* pDSLockedBuffer;
        void* pDSLockedBuffer2;
        DWORD dwDSLockedBufferSize;
        DWORD dwDSLockedBufferSize2;

        IF_ERROR_THROW(_directSoundBuffer->Lock(
            _starts[_next],
            _lengths[_next],
            &pDSLockedBuffer,
            &dwDSLockedBufferSize,
            &pDSLockedBuffer2,
            &dwDSLockedBufferSize2,
            0L));

        fillBuffer(pDSLockedBuffer, dwDSLockedBufferSize);
        fillBuffer(pDSLockedBuffer2, dwDSLockedBufferSize2);

        // Unlock the DirectSound buffer
        IF_ERROR_THROW(_directSoundBuffer->Unlock(
            pDSLockedBuffer,
            dwDSLockedBufferSize,
            NULL,
            0));

        _next = 1 - _next;
    }

    void consume(int n) { _consumeEvent.wait(); }

    ~DirectSoundSink()
    {
        _directSoundBuffer->Stop();
        _thread.end();
    }
    void wait() { _finish.wait(); }

private:
    void fillBuffer(void* data, int length)
    {
        Accessor<Sample> r = reader(length);
        length /= _bytesPerSample;
        Sample* sample = reinterpret_cast<signed short*>(data);
        for (int i = 0; i < length; ++i)
            *(sample++) = r.item();
        read(length);
        _consumeEvent.signal();
        if (finite() && remaining() <= 0)
            _finish.signal();
    }

    COMPointer<IDirectSound8> _directSound;
    ProcessingThread _thread;

    COMPointer<IDirectSoundBuffer> _directSoundBuffer;
    DWORD _dwNotifySize;
    DWORD _starts[2];
    DWORD _lengths[2];
    int _next;
    HWND _hWnd;
    UINT _msg;
    LPARAM _lparam;
    WPARAM _wparam;
    int _bytesPerSample;
    Event _event;
    Event _consumeEvent;
    Event _finish;

    friend class ProcessingThread;
};

template<class Sample> class XAudio2Sink : public AudioSink<Sample>
{
    class Callback : public IXAudio2VoiceCallback
    {
    public:
        void setSink(XAudio2Sink* sink) { _sink = sink; }
        virtual void __stdcall OnVoiceProcessingPassStart(UINT32) { }
        virtual void __stdcall OnVoiceProcessingPassEnd() { }
        virtual void __stdcall OnStreamEnd() { }
        virtual void __stdcall OnBufferStart(void*) { }
        virtual void __stdcall OnBufferEnd(void*) { _sink->bufferEnded(); }
        virtual void __stdcall OnLoopEnd(void*) { }
        virtual void __stdcall OnVoiceError(void*, HRESULT) { }
    private:
        XAudio2Sink* _sink;
    };

    class ProcessingThread : public Thread
    {
    public:
        ProcessingThread() : _ending(false) { }
        void setSink(XAudio2Sink* sink) { _sink = sink; }

        void threadProc()
        {
            while (true) {
                _sink->_event.wait();
                if (_ending)
                    return;
      	        _sink->fillNextBuffer();
            }
        }

        void end()
        {
            _ending = true;
            _sink->_event.signal();
            join();
        }
    private:
        XAudio2Sink* _sink;
        bool _ending;
    };

public:
    XAudio2Sink(int samplesPerSecond = 44100, int samplesPerBuffer = 512,
        int channels = 1)
      : AudioSink(samplesPerSecond, channels),
        _next(0)
    {
        _callback.setSink(this);
        _thread.setSink(this);
        _thread.start();

        IF_ERROR_THROW(XAudio2Create(&_xAudio2, 0));

        IF_ERROR_THROW(
            _xAudio2->CreateMasteringVoice(&_xAudio2MasteringVoice));

        IF_ERROR_THROW(
            _xAudio2->CreateSourceVoice(
                &_xAudio2SourceVoice,
                &_format,
                0,                     // Flags
                1.0f,                  // MaxFrequencyRatio
                &_callback));

        _samplesPerBuffer = samplesPerBuffer;
        _data.allocate(_samplesPerBuffer*2);
        _bytesPerBuffer = _format.nBlockAlign*_samplesPerBuffer;
    }
    void play()
    {
        fillNextBuffer();
        fillNextBuffer();
        IF_ERROR_THROW(_xAudio2SourceVoice->Start(0));
    }
    void wait() { _finish.wait(); }
    ~XAudio2Sink() { _thread.end(); }
private:
    void bufferEnded() { _event.signal(); }
    void fillNextBuffer()
    {
        do {
            XAUDIO2_VOICE_STATE state;
            _xAudio2SourceVoice->GetState(&state);
            if (state.BuffersQueued == 2)
                break;
            signed short* sample;
            if (_next == 0)
                sample = &_data[0];
            else
                sample = &_data[_samplesPerBuffer];
            XAUDIO2_BUFFER buffer = {0};
            buffer.AudioBytes = _bytesPerBuffer;
            buffer.pAudioData = reinterpret_cast<BYTE*>(sample);
            Accessor<Sample> r = reader(_samplesPerBuffer);
            for (int i = 0; i < _samplesPerBuffer; ++i)
                *(sample++) = r.item();
            read(_samplesPerBuffer);
            _consumeEvent.signal();
            IF_ERROR_THROW(_xAudio2SourceVoice->SubmitSourceBuffer(&buffer));
            _next = 1 - _next;
            if (finite() && remaining() <= 0)
                _finish.signal();
        } while (true);
    }

    void consume(int n) { _consumeEvent.wait(); }

    COMPointer<IXAudio2> _xAudio2;
    IXAudio2MasteringVoice* _xAudio2MasteringVoice;
    IXAudio2SourceVoice* _xAudio2SourceVoice;
    Callback _callback;
    ProcessingThread _thread;
    Event _event;
    int _next;
    Array<Sample> _data;
    int _samplesPerBuffer;
    int _bytesPerBuffer;
    Event _consumeEvent;
    Event _finish;

    friend class ProcessingThread;
    friend class Callback;
};

template<class Sample> class WaveOutSink : public AudioSink<Sample>
{
public:
    WaveOutSink(int samplesPerSecond = 44100,
        int samplesPerBufferChannel = 512, int channels = 1)
      : AudioSink(samplesPerSecond, channels)
    {
        IF_FALSE_THROW(waveOutOpen(&_device, WAVE_MAPPER, &_format,
            reinterpret_cast<DWORD_PTR>(waveOutProc),
            reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION)
            == MMSYSERR_NOERROR);

        _samplesPerBuffer = samplesPerBufferChannel * channels;
        _data.allocate(_samplesPerBuffer * 2);

        for (int i = 0; i < 2; ++i) {
            ZeroMemory(&_headers[i], sizeof(WAVEHDR));
            _headers[i].lpData =
                reinterpret_cast<LPSTR>(&_data[i*_samplesPerBuffer]);
            _headers[i].dwBufferLength = _samplesPerBuffer*sizeof(Sample);
        }
        _header = 0;
        _ending = false;
    }
    void play()
    {
        playBuffer();
        playBuffer();
    }
    ~WaveOutSink()
    {
        waveOutReset(_device);
        waveOutClose(_device);
    }
    void consume(int n) { _consumeEvent.wait(); }
    void wait() { _finish.wait(); }
private:
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg,
        DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
    {
        if (uMsg == WOM_DONE)
            reinterpret_cast<WaveOutSink*>(dwInstance)->nextBlock();
    }
    void nextBlock()
    {
        IF_FALSE_THROW(waveOutUnprepareHeader(_device, &_headers[_header],
            sizeof(WAVEHDR)) == MMSYSERR_NOERROR);
        if (!_ending)
            playBuffer();
    }
    void playBuffer()
    {
        Sample* p = &_data[_header*_samplesPerBuffer];
        Accessor<Sample> r = reader(_samplesPerBuffer);
        for (int i = 0; i < _samplesPerBuffer; ++i)
            *(p++) = r.item();
        read(_samplesPerBuffer);
        IF_FALSE_THROW(waveOutPrepareHeader(_device, &_headers[_header],
            sizeof(WAVEHDR)) == MMSYSERR_NOERROR);
        IF_FALSE_THROW(waveOutWrite(_device, &_headers[_header],
            sizeof(WAVEHDR)) == MMSYSERR_NOERROR);
        if (finite() && remaining() <= 0)
            _finish.signal();
        _header = 1 - _header;
    }
    int _samplesPerBuffer;
    Event _consumeEvent;
    HWAVEOUT _device;
    WAVEHDR _headers[2];
    int _header;
    Array<Sample> _data;
    bool _ending;
    Event _finish;
};

template<class Sample> class WaveFileSink : public AudioSink<Sample>
{
public:
    WaveFileSink(File file, int samplesPerSecond = 44100, int channels = 1,
        int samplesPerBufferChannel = 1024)
      : AudioSink(samplesPerSecond, channels),
        _samplesPerBuffer(samplesPerBufferChannel * channels),
        _bytes(0),
        _handle(file.openWrite())
    {
        // TODO: make endian-neutral. Posix port.
        _handle.write("RIFF", 4);
        DWORD t = 36;
        _handle.write(&t, 4);
        _handle.write("WAVE", 4);
        _handle.write("fmt ", 4);
        t = 16;
        _handle.write(&t, 4);
        _handle.write(&_format, 16);
        _handle.write("data", 4);
        t = 0;
        _handle.write(&t, 4);
    }
    void play()
    {
        do {
            consume(_samplesPerBuffer);
        } while (!finite() || remaining() > 0);
    }
    void consume(int n)
    {
        if (finite() && n > remaining())
            n = remaining();
        if (n > 0) {
            Accessor<Sample> r = reader(n);
            r.items(WriteTo<Sample>(&_handle), 0, n);
            _bytes += n*sizeof(Sample);
            read(n);
        }
        if (finite() && remaining() <= 0) {
            _handle.seek(4);
            DWORD t = _bytes + 36;
            _handle.write(&t, 4);
            _handle.seek(40);
            t = _bytes;
            _handle.write(&t, 4);
        }
    }
private:
    int _samplesPerBuffer;
    FileHandle _handle;
    int _bytes;
};

#endif // INCLUDED_AUDIO_H
