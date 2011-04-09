#ifndef INCLUDED_AUDIO_H
#define INCLUDED_AUDIO_H

// TODO: Posix port

#include <mmreg.h>
#include <dsound.h>
#include <xaudio2.h>
#include "unity/thread.h"

// TODO: Allow setting of the buffer length, bits-per-sample, sample rate and number of channels in the constructors

//static const double bufferLength = .04; // .04
//static const int sampleBytes = sampleBits/8;
////static const int bufferBytes = static_cast<int>(
////    sampleRate*sampleBytes*bufferLength);
//static const int bufferBytes = 7057;  // 4096 bad  6144 bad  6631 bad  6874 bad  6996 bad  7118 good  8192 good
//static const int nBlockAlign = sampleBits*channels/8;

template<class Sample> class DirectSoundSink : public Sink<Sample>
{
    class ProcessingThread : public Thread
    {
    public:
        void setSink(DirectSoundSink* sink) { _sink = sink; }

        void threadProc()
        {
            HANDLE hWaitHandles[2];
            hWaitHandles[0] = _eventBuffer;
            hWaitHandles[1] = _eventEnd;

            bool done = false;
            while (!done)
                switch (WaitForMultipleObjects(2, hWaitHandles, FALSE,
                    INFINITE)) {
                    case WAIT_OBJECT_0 + 0:
		                _sound->fillNextHalfBuffer();
                        _eventBuffer.reset();
                        break;
                    case WAIT_OBJECT_0 + 1:
                        done = true;
                        break;
	                default:
                        throw Exception();
                }
        }

        HANDLE getBufferEvent() { return _eventBuffer; }

        void end()  // TODO: I like the
        {
            _eventEnd.set();
            join();
        }
    private:
        Event _eventBuffer;
        Event _eventEnd;
        DirectSoundSink* _sink;
    };

public:
    DirectSoundSink(HWND hWnd, int sampleRate) : _hWnd(hWnd)
    {
        IF_ERROR_THROW(DirectSoundCreate8(NULL, &_directSound, NULL));

        // Set priority cooperative level
        IF_ERROR_THROW(
            _directSound->SetCooperativeLevel(hWnd, DSSCL_PRIORITY));

        WAVEFORMATEX wfx;
        ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = 1;
        wfx.nSamplesPerSec = sampleRate;
        wfx.wBitsPerSample = 16;
        _bytesPerSample = wfx.wBitsPerSample/8;
        wfx.nBlockAlign = _bytesPerSample;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec*_bytesPerSample;

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

            IF_ERROR_THROW(spDSBPrimary->SetFormat(&wfx));
        }

        // Set background thread priority
        _thread.setPriority(THREAD_PRIORITY_TIME_CRITICAL);
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

        int samplesPerBuffer = _waveform.samplesPerBuffer();  // TODO
        int bufferBytes = wfx.nBlockAlign*samplesPerBuffer;

        DSBUFFERDESC dsbd = {0};
        dsbd.dwSize = sizeof(DSBUFFERDESC);
        dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE |
            DSBCAPS_CTRLPOSITIONNOTIFY;
        dsbd.dwBufferBytes   = bufferBytes;
        dsbd.guid3DAlgorithm = GUID_NULL;
        dsbd.lpwfxFormat     = &wfx;

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
        aPosNotify[0].hEventNotify = _thread.getBufferEvent();
        aPosNotify[1].dwOffset = bufferBytes - 1;
        aPosNotify[1].hEventNotify = _thread.getBufferEvent();
        IF_ERROR_THROW(spDSN->SetNotificationPositions(2, aPosNotify));

        _next = 0;
        fillNextHalfBuffer();
        fillNextHalfBuffer();

        IF_ERROR_THROW(_directSoundBuffer->SetCurrentPosition(0));

        _thread.setSink(this);
        _thread.start();
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

    ~DirectSoundSound()
    {
        _directSoundBuffer->Stop();
        _thread.end();
    }

private:
    void fillBuffer(void* data, int length)
    {
        Accessor<Sample> r = reader(length);
        length /= _bytesPerSample;
        Sample* sample = reinterpret_cast<signed short*>(data);
        for (int i = 0; i < length; ++i)
            *(sample++) = r.item();
        read(length);
        _event.signal();
    }

    void consume(int n) { _event.wait(); }

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
};

template<class Sample> class XAudio2Sink : public Sink<Sample>
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
        XAudio2Sink* _sound;
    };

    class ProcessingThread : public Thread
    {
    public:
        ProcessingThread() : _ending(false) { }

        void setSink(XAudio2Sink* sink) { _sink = sink; }

        void threadProc()
        {
            BEGIN_CHECKED {
                while (true) {
                    _sink->_event.wait();
                    if (_ending)
                        return;
      	            _sink->fillNextBuffer();
                }
            } END_CHECKED(Exception& e) {
                e.display();  // TODO: this probably isn't right - can we marshall the exception over to the main thread and rethrow it there?
            }
        }

        HANDLE getBufferEvent() { return m_eventBuffer; }

        void end()
        {
            _ending = true;
            _sound->_event.set();
            join();
        }
    private:
        XAudio2Sink* _sink;
        bool _ending;
    };

public:
    XAudio2Sound() : _next(0)
    {
        _callback.setSink(this);
        _thread.setSink(this);
        _thread.start();

        IF_ERROR_THROW(XAudio2Create(&_xAudio2, 0));

        IF_ERROR_THROW(
            _xAudio2->CreateMasteringVoice(&_xAudio2MasteringVoice));

        WAVEFORMATEX waveFormatEx;
        waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
        waveFormatEx.nChannels = 1;
        waveFormatEx.nSamplesPerSec = _waveform.samplesPerSecond();
        waveFormatEx.nBlockAlign = 2;
        waveFormatEx.nAvgBytesPerSec =
            waveFormatEx.nSamplesPerSec*waveFormatEx.nBlockAlign;
        waveFormatEx.wBitsPerSample = 16;
        waveFormatEx.cbSize = 0;

        IF_ERROR_THROW(
            _xAudio2->CreateSourceVoice(
                &_xAudio2SourceVoice,
                &waveFormatEx,
                0,                     // Flags
                1.0f,                  // MaxFrequencyRatio
                &_callback));

        _samplesPerBuffer = _waveform.samplesPerBuffer();  // TODO
        _data.allocate(_samplesPerBuffer*2);
        _bytesPerBuffer = waveFormatEx.nBlockAlign*_samplesPerBuffer;
        fillNextBuffer();
        fillNextBuffer();
        IF_ERROR_THROW(_xAudio2SourceVoice->Start(0));
    }
    ~XAudio2Sound() { _thread.end(); }
private:
    void bufferEnded() { _event.set(); }
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
            _consumeEvent.set();
            IF_ERROR_THROW(_xAudio2SourceVoice->SubmitSourceBuffer(&buffer));
            _next = 1 - _next;
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

    friend class ProcessingThread;
    friend class Callback;
};


#endif // INCLUDED_AUDIO_H
