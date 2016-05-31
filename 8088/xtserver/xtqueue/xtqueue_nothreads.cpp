#include "alfe/main.h"
#include "alfe/thread.h"
#include "alfe/stack.h"
#include "alfe/linked_list.h"
#include "alfe/email.h"
#include "alfe/com.h"
#include "alfe/config_file.h"
#include "alfe/rdif.h"
#include <MMReg.h>
#include <dsound.h>

#include <WinCrypt.h>

class QueueItem : public LinkedListMember<QueueItem>
{
public:
    QueueItem(AutoHandle pipe, String fromAddress) : _pipe(pipe),
        _fromAddress(fromAddress), _broken(false), _aborted(false),
        _lastNotifiedPosition(-1)
    {
        _email = pipe.readLengthString();
        _emailValid = true;
        if (_email.length() < 4)
            _emailValid = false;
        else {
            int i;
            for (i = 0; i < _email.length(); ++i) {
                int c = _email[i];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '@' || c == '!' ||
                    c == '#' || c == '$' || c == '%' || c == '&' ||
                    c == '\'' || c == '*' || c == '+' || c == '-' ||
                    c == '/' || c == '=' || c == '?' || c == '^' ||
                    c == '_' || c == '`'  || c == '{' || c == '|' ||
                    c == '}' || c == '~' || c == '.')
                    continue;
                break;
            }
            if (i != _email.length())
                _emailValid = false;
        }

        _fileName = pipe.readLengthString();
        _data = pipe.readLengthString();
        _serverPId = pipe.read<DWORD>();
        _logFile = pipe.readLengthString();

        _command = pipe.read<int>();
        if (_command == 0 || _command == 4)
            _serverProcess = OpenProcess(SYNCHRONIZE, FALSE, _serverPId);

        HCRYPTPROV hCryptProv;
        bool gotSecret = false;
        if (CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, 0)) {
            if(CryptGenRandom(hCryptProv, 16, _secret)) {
                for (int i = 0; i < 16; ++i)
                    _secret[i] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGH"
                        "IJKLMNOPQRSTUVWXYZ_-"[_secret[i] & 0x3f];
                gotSecret = true;
            }
            CryptReleaseContext(hCryptProv, 0);
        }
        if (!gotSecret) {
            // Less secure as CryptGenRandom, but better than nothing.
            for (int i = 0; i < 16; ++i)
                _secret[i] = _logFile[(_logFile.length() - 1) - i];
        }
    }
    void printInfo()
    {
        console.write("Starting work item " + _logFile + ": " + _fileName +
            " for " + _email + "\n");
    }

    ~QueueItem()
    {
        try {
            try {
                if (_command == 0)
                    writeNoEmail("</pre>\n");

                FlushFileBuffers(_pipe);
                DisconnectNamedPipe(_pipe);

                if (!_emailValid)
                    return;

                if (WaitForSingleObject(_serverProcess, 0) == WAIT_TIMEOUT) {
                    // Server process is still running - we don't need to send
                    // email.
                    console.write("Server still running.\n");
                    return;
                }

                console.write("Sending email\n");

                sendMail(_fromAddress, _email, "Your XT Server results",
                    "A program was sent to the XT Server at\n"
                    "http://www.reenigne.org/xtserver but the browser was\n"
                    "disconnected before the program completed. The results of"
                    " this\nprogram are below. If you did not request this "
                    "information,\nplease ignore it.\n\n"
                    "Program name: " + fileName() + "\n\n" + _log);
            }
            catch (Exception& e)
            {
                console.write(e);
            }
        }
        catch (...)
        {
            // Don't let any errors escape from the destructor.
        }
    }

    void write(const char* string)
    {
        _log += string;
        write(static_cast<const void*>(string), strlen(string));
    }
    void write(int value)
    {
        _log += String::Byte(value);
        write(static_cast<const void*>(&value), 1);
    }
    void write(const String& s)
    {
        _log += s;
        write(&s[0], s.length());
    }
    void writeNoEmail(const String& s)
    {
        write(&s[0], s.length());
    }
    void write(const void* buffer, int bytes)
    {
        if (bytes == 0)
            return;
        DWORD bytesWritten;
        if (WriteFile(_pipe, buffer, bytes, &bytesWritten, NULL) == 0) {
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA) {
                _broken = true;
                if (!_emailValid)
                    _aborted = true;
            }
        }
    }

    void notifyQueuePosition(int position)
    {
        if (position == _lastNotifiedPosition)
            return;
        if (position == 0)
            write("Your program is starting\n");
        else
            if (position == 1)
                write("Your program is next in the queue.\n");
            else
                write(String("Your program is at position ") +
                    String::Decimal(position) + " in the queue.\n");
        _lastNotifiedPosition = position;
    }

    bool needSleep() { return _emailValid && !_broken; }

    bool aborted() { return _aborted; }

    String data() { return _data; }

    int command() { return _command; }

    HANDLE serverProcess() { return _serverProcess; }

    Byte data(int p) { return _data[p]; }

    String secret()
    {
        return String(reinterpret_cast<const char*>(&_secret[0]), 16);
    }

    String fileName() { return _fileName; }

    void kill()
    {
        write("Your program was terminated by an administrator.\n");
        delete this;
    }

    void cancel()
    {
        write("Your program was cancelled.\n");
        _emailValid = false;
        delete this;
    }

    void setFinishTime(DWORD finishTime) { _finishTime = finishTime; }
    DWORD getFinishTime() { return _finishTime; }

private:
    AutoHandle _pipe;

    String _fromAddress;
    String _email;
    String _fileName;
    String _data;
    String _logFile;

    String _log;
    bool _broken;
    bool _aborted;
    bool _emailValid;

    int _lastNotifiedPosition;

    DWORD _serverPId;

    int _command;

    Byte _secret[16];

    DWORD _finishTime;

    AutoHandle _serverProcess;
};


class AudioCapture : public ReferenceCounted
{
public:
    AudioCapture()
    {
        IF_ERROR_THROW(DirectSoundCaptureCreate8(NULL, &_capture, NULL));
        WAVEFORMATEX format;
        ZeroMemory(&format, sizeof(WAVEFORMATEX));
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 2;
        format.nSamplesPerSec = 44100;
        format.nAvgBytesPerSec = 44100*2*2;
        format.nBlockAlign = 2*2;
        format.wBitsPerSample = 16;
        format.cbSize = 0;
        DSCBUFFERDESC desc;
        ZeroMemory(&desc, sizeof(DSCBUFFERDESC));
        desc.dwSize = sizeof(DSCBUFFERDESC);
        desc.dwBufferBytes = 6*60*44100*2*2;
        desc.lpwfxFormat = &format;
        COMPointer<IDirectSoundCaptureBuffer> buffer;
        IF_ERROR_THROW(_capture->CreateCaptureBuffer(&desc, &buffer, NULL));
        _buffer = COMPointer<IDirectSoundCaptureBuffer8>(buffer,
            &IID_IDirectSoundCaptureBuffer8);
        IF_ERROR_THROW(_buffer->Start(0));
    }
    void finish(File file)
    {
        IF_ERROR_THROW(_buffer->Stop());
        DWORD readPosition;
        IF_ERROR_THROW(_buffer->GetCurrentPosition(NULL, &readPosition));
        Lock lock(_buffer, 0, readPosition);
        lock.write(file);
    }
private:
    class Lock
    {
    public:
        Lock(IDirectSoundCaptureBuffer8* buffer, DWORD offset, DWORD bytes)
          : _buffer(buffer), _bytes(bytes)
        {
            IF_ERROR_THROW(_buffer->Lock(offset, _bytes, &_audioPointer1,
                &_audioBytes1, &_audioPointer2, &_audioBytes2, 0));
        }
        ~Lock()
        {
            _buffer->Unlock(_audioPointer1, _bytesRead1, _audioPointer2,
                _bytesRead2);
        }
        void write(File file)
        {
            AutoHandle handle = file.openWrite();
            handle.write("RIFF");
            handle.write<UInt32>(_bytes + 0x24);
            handle.write("WAVEfmt ");
            handle.write<UInt32>(0x10);
            handle.write<UInt16>(1);
            handle.write<UInt16>(2);
            handle.write<UInt32>(44100);
            handle.write<UInt32>(44100*2*2);
            handle.write<UInt16>(2*2);
            handle.write<UInt16>(16);
            handle.write("data");
            handle.write<UInt32>(_bytes);
            if (_bytes < _audioBytes1) {
                handle.write(_audioPointer1, _bytes);
                _bytesRead1 = _bytes;
                _bytesRead2 = 0;
            }
            else {
                handle.write(_audioPointer1, _audioBytes1);
                _bytesRead1 = _audioBytes1;
                handle.write(_audioPointer2, _bytes - _audioBytes1);
                _bytesRead2 = _bytes - _audioBytes1;
            }
        }
    private:
        DWORD _bytes;
        LPVOID _audioPointer1;
        DWORD _audioBytes1;
        LPVOID _audioPointer2;
        DWORD _audioBytes2;
        DWORD _bytesRead1;
        DWORD _bytesRead2;
        IDirectSoundCaptureBuffer8* _buffer;
    };

    COMPointer<IDirectSoundCapture8> _capture;
    COMPointer<IDirectSoundCaptureBuffer8> _buffer;
};

class OverlappedHandle : public AutoHandle
{
public:
    OverlappedHandle() { init(); }
    OverlappedHandle(Handle h) : AutoHandle(h) { init(); }
    HANDLE event() { return _event; }
    DWORD transferred()
    {
        DWORD bytes;
        BOOL result = GetOverlappedResult(*this, &_overlapped, &bytes, TRUE);
        if (!result)
            throw Exception::systemError("Getting overlapped result");
        return bytes;
    }
protected:
    OVERLAPPED* overlapped() { return &_overlapped; }

    void init()
    {
        memset(&_overlapped, 0, sizeof(OVERLAPPED));
        _event = Event(CreateEvent(NULL, TRUE, FALSE, NULL));
        _overlapped.hEvent = _event;
    }
private:
    OVERLAPPED _overlapped;
    Event _event;
};

class COMPortHandle : public OverlappedHandle
{
public:
    COMPortHandle(String port, int baudRate, String name) : _buffer(1024)
    {
        NullTerminatedWideString path(port);
        Handle::operator=(AutoHandle(CreateFile(
            path,
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            FILE_FLAG_OVERLAPPED,
            NULL),          // hTemplate must be NULL for comm devices
            name));

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        IF_ZERO_THROW(GetCommState(*this, &deviceControlBlock));
        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = baudRate;
        deviceControlBlock.fBinary = TRUE;
        deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = FALSE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;
        deviceControlBlock.fDsrSensitivity = FALSE;
        deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = FALSE; //TRUE;
        deviceControlBlock.fInX = FALSE; //TRUE;
        deviceControlBlock.fErrorChar = FALSE;
        deviceControlBlock.fNull = FALSE;
        deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        deviceControlBlock.fAbortOnError = TRUE;
        deviceControlBlock.wReserved = 0;
        deviceControlBlock.ByteSize = 8;
        deviceControlBlock.Parity = NOPARITY;
        deviceControlBlock.StopBits = ONESTOPBIT;
        deviceControlBlock.XonChar = 17;
        deviceControlBlock.XoffChar = 19;
        IF_ZERO_THROW(SetCommState(*this, &deviceControlBlock));

        COMMTIMEOUTS timeOuts;
        SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
        timeOuts.ReadIntervalTimeout = 0;
        timeOuts.ReadTotalTimeoutMultiplier = 0;
        timeOuts.ReadTotalTimeoutConstant = 0;
        timeOuts.WriteTotalTimeoutConstant = 0;
        timeOuts.WriteTotalTimeoutMultiplier = 0;
        IF_ZERO_THROW(SetCommTimeouts(*this, &timeOuts));

        initRead();
    }
    void process()
    {
        int bytes = transferred();
        // TODO
        initRead();
    }
private:
    void initRead()
    {
        if (!ReadFile(*this, &_buffer[0], 1024, NULL, overlapped())) {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
                throw Exception::systemError("Reading serial port");
        }
    }
    Array<Byte> _buffer;
};

class PipeHandle : public OverlappedHandle
{
public:
    PipeHandle(String path)
    {
        Handle::operator=(File(path, true).createPipe(true));
        ConnectNamedPipe(*this, overlapped());
        DWORD error = GetLastError();
        if (error != ERROR_PIPE_CONNECTED && error != ERROR_IO_PENDING)
            throw Exception::systemError("Connecting named pipe");
    }

};

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <config file name>\n");
            return;
        }

        ConfigFile configFile;
        configFile.addDefaultOption("quickBootPort", String("COM1"));
        configFile.addDefaultOption("quickBootBaudRate", 115200);
        configFile.addDefaultOption("fromAddress",
            String("XT Server <xtserver@reenigne.org>"));
        configFile.addDefaultOption("pipe", String("\\\\.\\pipe\\xtserver"));
        configFile.addDefaultOption("lamePath",
            String("C:\\Program Files\\LAME\\lame.exe"));
        configFile.addDefaultOption("lameOptions", String("-r -s 44100 -m l"));
        configFile.addDefaultOption("adminAddress",
            String("andrew@reenigne.org"));
        configFile.addDefaultOption("htdocsPath",
            String("C:\\Program Files\\Apache Software Foundation\\Apache2.2\\"
            "htdocs"));
        configFile.addDefaultOption("captureFieldPath",
            String("C:\\capture_field.exe"));
        configFile.addDefaultOption("imagerPath",
            String("C:\\imager.bin"));
        configFile.addDefaultOption("vDosPath",
            String("C:\\vdos.bin"));
        configFile.addDefaultOption("snifferPort", String("COM3"));
        configFile.addDefaultOption("snifferBaudRate", 115200);
        configFile.load(File(_arguments[1], CurrentDirectory(), true));

        COMInitializer com;

        _queuedItems = 0;
        _processing = false;
        _diskBytes.allocate(0x10000);
        _item = 0;

        _fromAddress = configFile.get<String>("fromAddress");
        _lamePath = configFile.get<String>("lamePath");
        _lameOptions = configFile.get<String>("lameOptions");
        _adminAddress = configFile.get<String>("adminAddress");
        _htdocsPath = configFile.get<String>("htdocsPath");
        _captureFieldPath = configFile.get<String>("captureFieldPath");

        COMPortHandle quickBoot(configFile.get<String>("quickBootPort"),
            configFile.get<int>("quickBootBaudRate"),
            "Quickboot COM port");

        //reboot();

        //_imager = File(configFile->get<String>("imagerPath"), true).contents();
        _imager = configFile.get<String>("imagerPath");
        _vDos = configFile.get<String>("vDosPath");

        _packet.allocate(0x106);

        _hInput = GetStdHandle(STD_INPUT_HANDLE);

        _data.allocate(2048*48);

        COMPortHandle sniffer(configFile.get<String>("snifferPort"),
            configFile.get<int>("snifferBaudRate"),
            "Sniffer COM port");

        _snifferActive = false;

        String pipePath = configFile.get<String>("pipe");
        PipeHandle pipe(pipePath);

        Event stubServerProcessEvent;

        // main thread (pipe)
        while (true) {
            // Determine time to timeout
            //DWORD elapsed = GetTickCount() - _startTime;
            //int handleCount = 3;
            //if (_item->command() == 4) {
            //    elapsed = 0;
            //    handleCount = 4;
            //}
            //DWORD timeout = 5*60*1000 - elapsed;
            //if (timeout == 0 || timeout > 5*60*1000)
            //    return 0;
            //if (_killed || _cancelled)
            //    return 0;
            DWORD timeout = 5*60*1000;

            // Create a new pipe if needed
            if (!pipe.valid()) {
                DWORD error = GetLastError();
                if (error != ERROR_PIPE_CONNECTED && error != ERROR_IO_PENDING)
                    throw Exception::systemError("Connecting named pipe");
            }

            // Create array of event handles
            HANDLE handles[5] = {quickBoot.event(), sniffer.event(), _hInput, pipe.event(), stubServerProcessEvent};
            if (_item != 0)
                handles[4] = _item->serverProcess();

            // Wait for something to happen
            DWORD result = WaitForMultipleObjects(5, handles, FALSE, timeout);
            switch (result) {
                case WAIT_TIMEOUT:
                    // TODO
                    break;
                case WAIT_OBJECT_0:
                    quickBoot.process();
                    break;
                case WAIT_OBJECT_0 + 1:
                    sniffer.process();
                    break;
                case WAIT_OBJECT_0 + 2:
                    IF_ZERO_THROW(ReadConsoleInput(_hInput, _inputBuf, 128, &_inputCount));
                    // TODO: process console input
                    break;
                case WAIT_OBJECT_0 + 3:
                    pipe.transferred();
                    enqueue(pipe);
                    pipe = PipeHandle(pipePath);
                    break;
                case WAIT_OBJECT_0 + 4:
                    // TODO: server process terminated
                    break;
                default:
                    IF_FALSE_THROW(false);
                    break;
            }
        }

        // SnifferThread
        while (true) {
                Byte item;
                int remaining;
                do {
                    remaining = tryRead(&item, 1);
                    if (remaining != 0) {
                        console.write("Returning to wait\n");
                        break;
                    }
                    if ((item & 0x80) != 0)
                        break;
                } while (true);
                if (remaining != 0)
                    break;
                item &= 0x7f;

                Byte lengthBytes[2];
                remaining = tryRead(lengthBytes, 2);
                if (remaining != 0) {
                    _item->write(String("Warning: sniffer length truncated: ") + decimal(2 - remaining) + " bytes received.\n");
                    console.write("Returning to wait\n");
                    break;
                }

                int length = 1 + (lengthBytes[0] | (lengthBytes[1] << 7));
                if (((lengthBytes[0] | lengthBytes[1]) & 0x80) != 0)
                    _item->write(String("Warning: sniffer length corrupted.\n"));
                console.write(String("Item: ") + decimal(item) + " length: " + decimal(length) + "\n");
                if (length > 2048) {
                    _item->write(String("Warning: invalid sniffer run length ") + decimal(length) + ".\n");
                    length = 2048;
                }
                if (item > 47) {
                    _item->write(String("Warning: invalid sniffer item ") + decimal(item) + ".\n");
                    item = 47;
                }
                if (_lengths[item] != -1)
                    _item->write(String("Warning: repeated sniffer item ") + decimal(item) + ".\n");
                _lengths[item] = length;
                remaining = tryRead(&_data[item*2048], length);
                if (remaining != 0) {
                    _item->write(String("Warning: sniffer data truncated: ") + decimal(length - remaining) + " bytes received.\n");
                    console.write("Returning to wait\n");
                    break;
                }
        }

        // XTThread
        while (true) {
            _item = 0;
            try {
                QueueItem* i;
                {
                    _processing = false;
                    i = _queue.getNext();
                }
                if (i == 0) {
                    // We have nothing to do - stop the the thread until we do.
                    console.write("XT Thread going idle\n");
                    reboot();
                }
                {
                    // We have something to do. Tell all the threads their
                    // position in the queue.
                    int p = 0;
                    for (auto& i : _queue) {
                        i.notifyQueuePosition(p);
                        if (i.aborted()) {
                            i.remove();
                            --_queuedItems;
                            delete &i;
                        }
                        ++p;
                    }
                    _item = _queue.getNext();
                    if (_item != 0) {
                        _item->remove();
                        --_queuedItems;
                    }
                    _cancelled = false;
                    _killed = false;
                    if (_item == 0)
                        continue;
                    _item->printInfo();
                    _processing = true;
                }

                String fileName = _item->fileName();
                int fileNameLength = fileName.length();
                String extension;
                if (fileNameLength >= 4)
                    extension = fileName.subString(fileNameLength - 4, 4);
                String program;
                _diskImage.clear();
                bool comFile = false;
                bool exeFile = false;
                if (extension.equalsIgnoreCase(".img")) {
                    program = File(_imager, true).contents();
                    _diskImage.load(_item->data());
                }
                else
                    if (extension.equalsIgnoreCase(".com")) {
                        program = File(_vDos, true).contents();
                        comFile = true;
                    }
                    else
                        if (extension.equalsIgnoreCase(".exe")) {
                            program = File(_vDos, true).contents();
                            exeFile = true;
                        }
                        else
                            program = _item->data();

                int retry = 1;
                bool error;
                _startTime = GetTickCount();
                for (int retry = 0; retry < 10; ++retry) {
                    IF_ZERO_THROW(
                        PurgeComm(_quickBoot, PURGE_RXCLEAR | PURGE_TXCLEAR));

                    reboot();
                    bothWrite("Transferring attempt " + String::Decimal(retry) + "\n");

                    error = upload(program);
                    if (!error)
                        break;
                }
                if (comFile || exeFile)
                    upload(_item->data());

                if (error) {
                    console.write("Failed to upload!\n");
                    _item->write("Could not transfer the program to the XT. "
                        "The machine may be offline for maintainance - please "
                        "try again later.\n");
                    delete _item;
                    continue;
                }

                bothWrite("Upload complete.\n");
                if (_item->aborted()) {
                    delete _item;
                    continue;
                }
                bool timedOut = stream();
                stopRecording();
                if (_snifferActive) {
                    _snifferActive = false;
                    _snifferThread.endCapture();
                }
                bothWrite("\n");
                if (_item->aborted()) {
                    delete _item;
                    continue;
                }

                if (_killed) {
                    _item->kill();
                    continue;
                }
                if (_cancelled) {
                    _item->cancel();
                    continue;
                }

                if (timedOut)
                    bothWrite("The program did not complete within 5 "
                        "minutes and has been\nterminated. If you really need "
                        "to run a program for longer,\n"
                        "please send email to " + _adminAddress + ".");
                else
                    bothWrite("Program ended normally.");

                if (_item->needSleep()) {
                    _emailThread.add(_item);
                    _item = 0;
                }
            }
            catch (const Exception& e)
            {
                try {
                    _item->write(
                        "Sorry, something went wrong with your program.\n");
                }
                catch (...) { }
                try {
                    console.write(e);
                }
                catch (...) { }
            }
            catch (...)
            {
                // If an exception is thrown processing one item, don't let
                // bring the whole process down.
            }
            if (_item != 0)
                delete _item;
            console.write("Work item complete\n");
        }

        // EmailThread
// We want to send an email to the user if and only if the HTTP connection was
// terminated before all the information was sent. However, the only way to
// know if the connection was terminated is to send data across it and wait for
// a few seconds. If the connection was terminated the CGI process will be
// terminated when Apache notices that the last transmission was not received.
// So this thread just waits for 5 seconds after the last transmission and then
// deletes the item - the item then checks to see if the server process went
// away, and if so sends email.
        while (true) {
                QueueItem* item;
                {
                    item = _emailQueue.getNext();
                    if (item != 0)
                        item->remove();
                }
                if (item == 0) {
                    console.write("Email thread entering waiting state.\n");
                    // We have nothing to do - stop the the thread until we do.
                    console.write("Email thread unblocked.\n");
                    continue;
                }
                DWORD sleepTime = 5000 -
                    (GetTickCount() - item->getFinishTime());
                console.write("Email thread sleeping for " +
                    String::Decimal(sleepTime) + "ms.\n");
                if (sleepTime <= 5000)
                    Sleep(sleepTime);
                console.write("Email thread deleting item.\n");
                delete item;
        }
    }
private:
    void enqueue(AutoHandle pipe)
    {
        QueueItem* item = new QueueItem(pipe, _fromAddress);

        switch (item->command()) {
            case 0:
            case 4:  // Run via xtrun (no timeout)
                {
                    // Run a program
                    _queue.add(item);
                    item->writeNoEmail("<form action='http://reenigne.homenet."
                        "org/cgi-bin/xtcancel.exe' method='post'>\n"
                        "<input type='hidden' name='secret' value='" +
                        item->secret() + "'/>\n"
                        "<button type='submit'>Cancel</button>\n"
                        "</form>\n<pre>");
                    item->notifyQueuePosition(_queuedItems +
                        (_processing ? 1 : 0));
                    ++_queuedItems;
                }
                break;
            case 1:
                {
                    // Kill a running or queued program
                    int n = 0;
                    CharacterSource s(item->fileName());
                    do {
                        int c = s.get();
                        if (c < '0' || c > '9')
                            break;
                        n = n*10 + c - '0';
                    } while (true);
                    if (n == 0)
                        _killed = true;
                    else {
                        --n;
                        for (auto& i : _queue) {
                            if (n == 0) {
                                --_queuedItems;
                                i.remove();
                                i.kill();
                                break;
                            }
                            --n;
                        }
                    }
                    delete item;
                }
                break;
            case 2:
                {
                    // Return status
                    item->write(String("online, with ") +
                        (_queuedItems + (_processing ? 1 : 0)) +
                        " items in the queue");
                    delete item;
                }
                break;
            case 3:
                {
                    // Cancel a job
                    String newSecret = item->fileName().subString(7, 16);
                    if (_item != 0 && newSecret == _item->secret()) {
                        _cancelled = true;
                        item->cancel();
                    }
                    else {
                        for (auto& i : _queue) {
                            if (i.secret() == item->secret()) {
                                --_queuedItems;
                                i.remove();
                                i.cancel();
                                item->cancel();
                                return;
                            }
                        }
                        item->write("Could not find item to cancel.");
                    }
                }
                break;
        }
    }
    void reboot()
    {
        bothWrite("Resetting\n");

        //IF_ZERO_THROW(FlushFileBuffers(_quickBoot));
        //IF_ZERO_THROW(PurgeComm(_quickBoot, PURGE_RXCLEAR | PURGE_TXCLEAR));

        // Reset the Arduino
        EscapeCommFunction(_quickBoot, CLRDTR);
        Sleep(250);
        _serialThread.character();
        EscapeCommFunction(_quickBoot, SETDTR);

        // The Arduino bootloader waits a bit to see if it needs to
        // download a new program.

        //int i = 0;
        //do {
        //    Byte b = _quickBoot.tryReadByte();
        //    if (b == 'R')
        //        break;
        //    if (b != -1)
        //        i = 0;
        //    ++i;
        //} while (i < 10);

        //if (i == 10)
        //    throw Exception("No response from QuickBoot");
    }
    void bothWrite(String s)
    {
        console.write(s);
        if (_item != 0)
            _item->write(s);
    }
    // returns true if an error occurred and the caller should try again after
    // rebooting.
    bool upload(String program)
    {
        int i = 0;
        do {
            int x = waitInput();
            if (x == 0)
                return false;
            if (x == 2)
                continue;
            int b = _serialThread.character();
            console.write<Byte>(b);
//            console.write(String("(") + debugByte(b) + ")");
            if (b == 'R')
                break;
            if (b != -1)
                i = 0;
            ++i;
        } while (i < 10);

        //console.write('u');

        //IF_ZERO_THROW(FlushFileBuffers(_quickBoot));
        //IF_ZERO_THROW(PurgeComm(_quickBoot, PURGE_RXCLEAR | PURGE_TXCLEAR));

        int l = program.length();
        Byte checksum;
        int p = 0;
        int bytes;
        do {
            bytes = min(l, 0xff);
//            _packet[0] = 0x76;  // clear keyboard buffer
//            _packet[1] = 0x7a;  // block real keyboard
            _packet[2] = 0x75;  // raw mode
            _packet[3] = bytes;
            _packet[4] = 'X';  // Marker
            _packet[5] = bytes;
            checksum = 0;
            for (int i = 0; i < bytes; ++i) {
                Byte d = program[p];
                ++p;
                _packet[i + 6] = d;
                checksum += d;
            }
            _packet[bytes + 6] = checksum;
            int tries = 0;
            do {
                //console.write('p');
                writeSerial(&_packet[2], 5 + bytes);
                //IF_ZERO_THROW(FlushFileBuffers(_quickBoot));

                int b;
                int x;
                do {
                    x = waitInput();
                    if (x == 0)
                        return false;
                    if (x == 2)
                        continue;
                    break;
                } while (true);

                b = _serialThread.character();
                console.write<Byte>(b);
                //console.write(String("<") + debugByte(b) + ">");
                if (b == 'K')
                    break;
                if (b != 'R')
                    ++tries;
            } while (tries < 10);
            if (tries == 10)
                return true;
            l -= bytes;
        } while (bytes != 0);
        //IF_ZERO_THROW(FlushFileBuffers(_quickBoot));
        return false;
    }
    String htDocsPath(String name) { return _htdocsPath + "\\" + name; }
    void threadProc()
    {
        do {
        } while (true);
    }
    void sendByte(int byte)
    {
        console.write(String("(") + debugByte(byte) + ")");
        writeSerial(reinterpret_cast<const Byte*>(&byte), 1);
        //_quickBoot.write<Byte>(byte);
    }
    String debugByte(int b)
    {
        if (b >= 32 && b < 127 && !(b >= 48 && b < 58))
            return codePoint(b);
        return decimal(b);
    }
    // Dump bytes from COM port to pipe until we receive ^Z or we time out.
    // Also process any commands from the XT. Return true for timeout, false
    // for normal exit.
    bool stream()
    {
        bool escape = false;
        bool audio = false;
        _fileState = 0;
        int fileSize = 0;
        //String fileName;
        int filePointer;
        Array<Byte> file;
        int fileCount = 0;
        _imageCount = 0;
        int audioCount = 0;
        int hostBytesRemaining = 0;
        _diskByteCount = 0;
        do {
            int x = waitInput();
            if (x == 0)
                return true;

            if (x == 2) {
                for (int i = 0; i < static_cast<int>(_inputCount); ++i) {
                    //console.write(String(decimal(buf[i].EventType)) + " ");
                    if (_inputBuf[i].EventType == KEY_EVENT) {
                        int key = _inputBuf[i].Event.KeyEvent.wVirtualKeyCode;
                        bool up = !_inputBuf[i].Event.KeyEvent.bKeyDown;
                        int scanCode = 0;
                        switch (key) {
                            case VK_BACK:     scanCode = 0x0e; break;
                            case VK_TAB:      scanCode = 0x0f; break;
                            case VK_RETURN:   scanCode = 0x1c; break;
                            case VK_CONTROL:  scanCode = 0x1d; break;
                            case VK_MENU:     scanCode = 0x38; break;
                            case VK_CAPITAL:  scanCode = 0x3a; break;
                            case VK_ESCAPE:   scanCode = 0x01; break;
                            case VK_SPACE:    scanCode = 0x39; break;
                            case VK_PRIOR:    scanCode = 0x49; break;
                            case VK_NEXT:     scanCode = 0x51; break;
                            case VK_END:      scanCode = 0x4f; break;
                            case VK_HOME:     scanCode = 0x47; break;
                            case VK_LEFT:     scanCode = 0x4b; break;
                            case VK_UP:       scanCode = 0x48; break;
                            case VK_RIGHT:    scanCode = 0x4d; break;
                            case VK_DOWN:     scanCode = 0x50; break;
                            case VK_SNAPSHOT: scanCode = 0x37; break;
                            case VK_INSERT:   scanCode = 0x52; break;
                            case VK_DELETE:   scanCode = 0x53; break;
                            case '0':         scanCode = 0x0b; break;
                            case '1':         scanCode = 0x02; break;
                            case '2':         scanCode = 0x03; break;
                            case '3':         scanCode = 0x04; break;
                            case '4':         scanCode = 0x05; break;
                            case '5':         scanCode = 0x06; break;
                            case '6':         scanCode = 0x07; break;
                            case '7':         scanCode = 0x08; break;
                            case '8':         scanCode = 0x09; break;
                            case '9':         scanCode = 0x0a; break;
                            case 'A':         scanCode = 0x1e; break;
                            case 'B':         scanCode = 0x30; break;
                            case 'C':         scanCode = 0x2e; break;
                            case 'D':         scanCode = 0x20; break;
                            case 'E':         scanCode = 0x12; break;
                            case 'F':         scanCode = 0x21; break;
                            case 'G':         scanCode = 0x22; break;
                            case 'H':         scanCode = 0x23; break;
                            case 'I':         scanCode = 0x17; break;
                            case 'J':         scanCode = 0x24; break;
                            case 'K':         scanCode = 0x25; break;
                            case 'L':         scanCode = 0x26; break;
                            case 'M':         scanCode = 0x32; break;
                            case 'N':         scanCode = 0x31; break;
                            case 'O':         scanCode = 0x18; break;
                            case 'P':         scanCode = 0x19; break;
                            case 'Q':         scanCode = 0x10; break;
                            case 'R':         scanCode = 0x13; break;
                            case 'S':         scanCode = 0x1f; break;
                            case 'T':         scanCode = 0x14; break;
                            case 'U':         scanCode = 0x16; break;
                            case 'V':         scanCode = 0x2f; break;
                            case 'W':         scanCode = 0x11; break;
                            case 'X':         scanCode = 0x2d; break;
                            case 'Y':         scanCode = 0x15; break;
                            case 'Z':         scanCode = 0x2c; break;
                            case VK_NUMPAD0:  scanCode = 0x52; break;
                            case VK_NUMPAD1:  scanCode = 0x4f; break;
                            case VK_NUMPAD2:  scanCode = 0x50; break;
                            case VK_NUMPAD3:  scanCode = 0x51; break;
                            case VK_NUMPAD4:  scanCode = 0x4b; break;
                            case VK_NUMPAD5:  scanCode = 0x4c; break;
                            case VK_NUMPAD6:  scanCode = 0x4d; break;
                            case VK_NUMPAD7:  scanCode = 0x47; break;
                            case VK_NUMPAD8:  scanCode = 0x48; break;
                            case VK_NUMPAD9:  scanCode = 0x49; break;
                            case VK_MULTIPLY: scanCode = 0x37; break;
                            case VK_ADD:      scanCode = 0x4e; break;
                            case VK_SUBTRACT: scanCode = 0x4a; break;
                            case VK_DECIMAL:  scanCode = 0x53; break;
                            case VK_DIVIDE:   scanCode = 0x35; break;
                            case VK_F1:       scanCode = 0x3b; break;
                            case VK_F2:       scanCode = 0x3c; break;
                            case VK_F3:       scanCode = 0x3d; break;
                            case VK_F4:       scanCode = 0x3e; break;
                            case VK_F5:       scanCode = 0x3f; break;
                            case VK_F6:       scanCode = 0x40; break;
                            case VK_F7:       scanCode = 0x41; break;
                            case VK_F8:       scanCode = 0x42; break;
                            case VK_F9:       scanCode = 0x43; break;
                            case VK_F10:      scanCode = 0x44; break;
                            case VK_F11:      // Ctrl+Alt+Del
                                if (up) {
                                    sendByte(0x1d);
                                    sendByte(0x38);
                                    sendByte(0x53);
                                }
                                else {
                                    sendByte(0x9d);
                                    sendByte(0xb8);
                                    sendByte(0xd3);
                                }
                                return false;
                            case VK_F12:      // Power cycle
                                if (up)
                                    return false;
                                scanCode = 0x77;
                                break;
                            case VK_NUMLOCK:  scanCode = 0x45; break;
                            case VK_SCROLL:   scanCode = 0x46; break;
                            case VK_SHIFT:
                                if (_inputBuf[i].Event.KeyEvent.wVirtualScanCode ==
                                    0x2a)
                                    scanCode = 0x2a;
                                else
                                    scanCode = 0x36;
                                break;
                            case VK_OEM_1:    scanCode = 0x27; break;
                            case VK_OEM_PLUS: scanCode = 0x0d; break;
                            case VK_OEM_COMMA: scanCode = 0x33; break;
                            case VK_OEM_MINUS: scanCode = 0x0c; break;
                            case VK_OEM_PERIOD: scanCode = 0x34; break;
                            case VK_OEM_2:    scanCode = 0x35; break;
                            case VK_OEM_3:    scanCode = 0x29; break;
                            case VK_OEM_4:    scanCode = 0x1a; break;
                            case VK_OEM_5:    scanCode = 0x2b; break;
                            case VK_OEM_6:    scanCode = 0x1b; break;
                            case VK_OEM_7:    scanCode = 0x28; break;
                            default: return false;
                        }
                        sendByte(scanCode | (up ? 0x80 : 0));
                    }
                }
                continue;
            }

            int c = _serialThread.character();
//            console.write(String("{") + debugByte(c) + "}");
            if (c == -1)
                continue;
            if (!escape && _fileState == 0) {
                bool processed = false;
                switch (c) {
                    case 0x00:
                        // Transfer following byte directly, don't
                        // interpret it as an action.
                        escape = true;
                        processed = true;
                        break;
                    case 0x01:
                        takeScreenshot();
                        processed = true;
                        break;
                    case 0x02:
                        // Start recording audio
                        _audioCapture = new AudioCapture;
                        processed = true;
                        break;
                    case 0x03:
                        processed = true;
                        stopRecording();
                        break;
                    case 0x04:
                        // Transfer file
                        _fileState = 1;
                        processed = true;
                        break;
                    case 0x05:
                        // Host interrupt
                        _fileState = 5;
                        hostBytesRemaining = 18;
                        processed = true;
                        break;
                    case 0x06:
                        // Start recording sniffer data
                        _snifferThread.beginCapture(_item);
                        sendByte('K');
                        _snifferActive = true;
                        processed = true;
                        break;
                    case 0x07:
                        // Stop recording sniffer data
                        if (_snifferActive) {
                            _snifferActive = false;
                            _snifferThread.endCapture();
                        }
                        processed = true;
                        break;
                    case 0x1a:
                        return false;
                        processed = true;
                        break;
                }

                if (c != 0)
                    escape = false;
                if (processed) {
                    if (_fileState != 0)
                        console.write(String("[") + debugByte(c) + String("]"));
                    continue;
                }
            }
            escape = false;
            //if (_fileState != 0)
            //    console.write(String("[") + debugByte(c) + String("]"));
            switch (_fileState) {
                case 0:
                    // No file operation in progress - output to HTTP
                    if (c == '<')
                        _item->write("&lt;");
                    else
                        if (c == '&')
                            _item->write("&amp;");
                        else
                            _item->write(c);
                    if ((c < 32 || c > 126) && c != 9 && c != 10 && c != 13)
                        console.write<Byte>('.');
                    else
                        console.write<Byte>(c);
                    break;
                case 1:
                    // Get first byte of size
                    fileSize = c;
                    _fileState = 2;
                    break;
                case 2:
                    // Get second byte of size
                    fileSize |= (c << 8);
                    _fileState = 3;
                    break;
                case 3:
                    // Get third byte of size
                    fileSize |= (c << 16);
                    _fileState = 4;
                    filePointer = 0;
                    file.allocate(fileSize);
                    break;
                case 4:
                    // Get file data
                    file[filePointer++] = c;
                    if (filePointer == fileSize) {
                        _fileState = 0;
                        String fileName = _item->secret() + fileCount + ".dat";
                        File(htDocsPath(fileName), true).save(file);
                        _item->write("\n<a href=\"../" + fileName +
                            "\">Captured file</a>\n");
                        ++fileCount;
                    }
                    break;
                case 5:
                    // Get host interrupt data
                    _hostBytes[18 - hostBytesRemaining] = c;
                    --hostBytesRemaining;
                    if (hostBytesRemaining != 0)
                        break;
                    _fileState = 0;
                    if (_hostBytes[0] != 0x13) {
                        bothWrite("Unknown host interrupt " +
                            String::Hex(_hostBytes[0], 2, true));
                        break;
                    }
                    diskHostInterruptStart();
                    break;
                case 6:
                    // Get disk data
                    _diskBytes[_diskDataPointer] = c;
                    ++_diskDataPointer;
                    if (_diskDataPointer != _diskByteCount)
                        break;
                    _fileState = 0;
                    _diskImage.bios(&_diskData, _hostBytes);
                    sendDiskResult();
                    break;
            }
            if (_item->aborted())
                return false;
        } while (true);
    }
    void takeScreenshot()
    {
        String fileName = _item->secret() + _imageCount + ".png";
        String path = htDocsPath(fileName);

        String commandLine = "\"" + _captureFieldPath + "\" \"" + path + "\"";
        NullTerminatedWideString data(commandLine);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);

        IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE, 0, NULL,
            NULL, &si, &pi) != 0);
        CloseHandle(pi.hThread);
        AutoHandle hLame = pi.hProcess;
        IF_FALSE_THROW(WaitForSingleObject(hLame, 3*60*1000) == WAIT_OBJECT_0);

        _item->write("\n<img src=\"../" + fileName + "\"/>\n");
        ++_imageCount;
    }
    void stopRecording()
    {
        if (!_audioCapture.valid())
            return;
        String rawName = _item->secret() + _audioCount;
        String baseName = htDocsPath(rawName);
        String waveName = baseName + ".wav";
        File wave(waveName, true);
        _audioCapture->finish(wave);
        _audioCapture = 0;
        String commandLine = "\"" + _lamePath + "\" \"" + waveName + "\" \"" +
            baseName + ".mp3\" " + _lameOptions;
        NullTerminatedWideString data(commandLine);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);

        IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE, 0, NULL,
            NULL, &si, &pi) != 0);
        CloseHandle(pi.hThread);
        AutoHandle hLame = pi.hProcess;
        IF_FALSE_THROW(WaitForSingleObject(hLame, 3*60*1000) == WAIT_OBJECT_0);

        wave.remove();

        _item->write("\n<embed height=\"50\" width=\"100\" src=\"../" +
            rawName + ".mp3\"><a href=\"../" + rawName +
            ".mp3\">Recorded audio</a></embed>\n");
        ++_audioCount;
    }
    void diskHostInterruptStart()
    {
        _diskSectorCount = _hostBytes[1];
        _hostInterruptOperation = _hostBytes[2];
        int sectorSize = 128 << _hostBytes[10];
        int sectorsPerTrack = _hostBytes[11];
        _hostBytes[18] = 0;
        _hostBytes[19] = (_hostBytes[5] != 0 ? 0x80 : 0);
        _hostBytes[20] = 3;  // 3 for error, 2 for success
        switch (_hostInterruptOperation) {
            case 2:
                {
                    Byte sector = _hostBytes[3] & 0x3f;
                    Word track = _hostBytes[4] | ((_hostBytes[3] & 0xc0) << 2);
                    Byte head = _hostBytes[6];
                    console.write(String("Read ") + decimal(_hostBytes[1]) + " sectors at " + decimal(sector) + ", " + decimal(head) + ", " + decimal(track) + "\n");
                }
                // Read disk sectors
                _diskImage.bios(&_diskData, _hostBytes);
                {
                    bool error = upload(String(_diskData));
                    //if (error)
                    //    console.write("Data upload failed!\n");
                }
                console.write("Data upload complete\n");
                break;
            case 3:
                // Write disk sectors
            case 4:
                // Verify disk sectors
                _diskByteCount = _diskSectorCount << (_hostBytes[10] + 7);
                _diskDataPointer = 0;
                _fileState = 6;
                return;
            case 5:
                // Format track
                _diskByteCount = 4*sectorsPerTrack;
                _diskDataPointer = 0;
                _fileState = 6;
                return;
        }
        sendDiskResult();
        console.write("Result upload complete\n");
    }
    void sendDiskResult()
    {
        console.write(String("Sending result bytes ") + decimal(_hostBytes[18]) + ", " + decimal(_hostBytes[19]) + ", " + decimal(_hostBytes[20]) + "\n");
        upload(String(reinterpret_cast<const char*>(&_hostBytes[18]), 3));
    }
    void writeSerial(const Byte* data, int bytes)
    {
        WriteFile(_quickBoot, data, bytes, NULL, &_quickBootOverlapped);
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING)
            throw Exception::systemError("Writing serial port");
        DWORD bytesWritten;
        IF_ZERO_THROW(GetOverlappedResult(_quickBoot, &_quickBootOverlapped,
            &bytesWritten, TRUE));
        if (bytes != bytesWritten)
            throw Exception::systemError("Writing serial port");
    }


    // EmailThread
    void add(QueueItem* item)
    {
        console.write("Adding item to email thread.\n");
        item->setFinishTime(GetTickCount());
        Lock lock(&_mutex);
        _emailQueue.add(item);
        _ready.signal();
        console.write("Item added to email thread.\n");
    }

    // SerialThread
    int character()
    {
        int c = _c;
        _event.reset();
        _reset.signal();
        return c;
    }

    // SnifferThread
    int tryRead(Byte* buffer, int bytes)
    {
        do {
            if (_stop) {
                console.write("Signalling stop\n");
                _stopping.signal();
                _stop = false;
                return bytes;
            }
            int read = _handle.tryRead(buffer, bytes);
            bytes -= read;
            if (bytes == 0)
                return 0;
            buffer += bytes;
        } while (true);
    }
    void beginCapture(QueueItem* item)
    {
        _item = item;

        for (int i = 0; i < 48; ++i)
            _lengths[i] = -1;

        _stop = false;

        _event.signal();
    }
    void endCapture()
    {
        console.write("Requesting stop\n");
        _stop = true;
        _stopping.wait();
        console.write("Stop completed\n");

        int length = _lengths[0];
        for (int i = 0; i < 16; ++i) {
            int l = _lengths[i];
            if (l != length || l == -1) {
                length = max(length, l);
                if (l == -1)
                    _item->write(String("Warning: missing sniffer item ") + decimal(i) + "\n");
                else
                    _item->write(String("Warning: sniffer packets have inconsistent lengths.\n"));
            }
        }
        bool fastSampling = (_lengths[16] != -1);
        if (!fastSampling) {
            for (int i = 17; i < 48; ++i)
                if (_lengths[i] != -1)
                    _item->write(String("Warning: got some but not all fast-sampling sniffer packets.\n"));
        }
        else {
            for (int i = 16; i < 48; ++i)
                if (_lengths[i] != length) {
                    length = max(length, _lengths[i]);
                    _item->write(String("Warning: sniffer packets have inconsistent lengths.\n"));
                }
        }
        if (length == -1) {
            _item->write(String("Warning: sniffer activated but no data received.\n"));
            return;
        }

        for (int th = 0; th < length; ++th)
            for (int tl = 0; tl < (fastSampling ? 3 : 1); ++tl) {
                Byte p[16];
                for (int i = 0; i < 16; ++i) {
                    p[i] = _data[th + (i + tl*16)*2048];
                    if ((p[i] & 0x80) != 0)
                        _item->write(String("Warning: sniffer packet corrupted.\n"));
                    if (i < 8) {
                        if ((p[i] & 0x40) != 0)
                            _item->write(String("Warning: sniffer port C packet corrupted.\n"));
                    }
                    else
                        p[i] = ((p[i] & 0x40) >> 6) | ((p[i] & 0x3f) << 2);
                }
                UInt32 cpu =
                      ((p[0x4] & 0x08) != 0 ? 0x80000 : 0)   // 35 A19/S6        O ADDRESS/STATUS: During T1, these are the four most significant address lines for memory operations. During I/O operations, these lines are LOW. During memory and I/O operations, status information is available on these lines during T2, T3, Tw, and T4. S6 is always low.
                    | ((p[0x7] & 0x10) != 0 ? 0x40000 : 0)   // 36 A18/S5        O The status of the interrupt enable flag bit (S5) is updated at the beginning of each clock cycle.
                    | ((p[0x6] & 0x10) != 0 ? 0x20000 : 0)   // 37 A17/S4        O  S4*2+S3 0 = Alternate Data, 1 = Stack, 2 = Code or None, 3 = Data
                    | ((p[0x5] & 0x10) != 0 ? 0x10000 : 0)   // 38 A16/S3        O
                    | ((p[0x4] & 0x10) != 0 ? 0x08000 : 0)   // 39 A15           O ADDRESS BUS: These lines provide address bits 8 through 15 for the entire bus cycle (T1ñT4). These lines do not have to be latched by ALE to remain valid. A15ñA8 are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
                    | ((p[0x2] & 0x10) != 0 ? 0x04000 : 0)   //  2 A14
                    | ((p[0x1] & 0x10) != 0 ? 0x02000 : 0)   //  3 A13
                    | ((p[0x0] & 0x10) != 0 ? 0x01000 : 0)   //  4 A12
                    | ((p[0x3] & 0x08) != 0 ? 0x00800 : 0)   //  5 A11
                    | ((p[0x2] & 0x08) != 0 ? 0x00400 : 0)   //  6 A10
                    | ((p[0x1] & 0x08) != 0 ? 0x00200 : 0)   //  7 A9
                    | ((p[0x0] & 0x08) != 0 ? 0x00100 : 0)   //  8 A8
                    | ((p[0xb] & 0x04) != 0 ? 0x00080 : 0)   //  9 AD7          IO ADDRESS DATA BUS: These lines constitute the time multiplexed memory/IO address (T1) and data (T2, T3, Tw, T4) bus. These lines are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
                    | ((p[0xa] & 0x04) != 0 ? 0x00040 : 0)   // 10 AD6
                    | ((p[0x9] & 0x04) != 0 ? 0x00020 : 0)   // 11 AD5
                    | ((p[0x8] & 0x04) != 0 ? 0x00010 : 0)   // 12 AD4
                    | ((p[0xb] & 0x08) != 0 ? 0x00008 : 0)   // 13 AD3
                    | ((p[0xa] & 0x08) != 0 ? 0x00004 : 0)   // 14 AD2
                    | ((p[0x9] & 0x08) != 0 ? 0x00002 : 0)   // 15 AD1
                    | ((p[0x8] & 0x08) != 0 ? 0x00001 : 0);  // 16 AD0
                UInt8 qs =
                      ((p[0xe] & 0x08) != 0 ? 1 : 0)         // 25 QS0           O QUEUE STATUS: provide status to allow external tracking of the internal 8088 instruction queue. The queue status is valid during the CLK cycle after which the queue operation is performed.
                    | ((p[0xf] & 0x08) != 0 ? 2 : 0);        // 24 QS1           0 = No operation, 1 = First Byte of Opcode from Queue, 2 = Empty the Queue, 3 = Subsequent Byte from Queue
                char qsc[] = "-1ES";

                UInt8 s =
                      ((p[0xd] & 0x08) != 0 ? 1 : 0)         // 26 -S0           O STATUS: is active during clock high of T4, T1, and T2, and is returned to the passive state (1,1,1) during T3 or during Tw when READY is HIGH. This status is used by the 8288 bus controller to generate all memory and I/O access control signals. Any change by S2, S1, or S0 during T4 is used to indicate the beginning of a bus cycle, and the return to the passive state in T3 and Tw is used to indicate the end of a bus cycle. These signals float to 3-state OFF during ``hold acknowledge''. During the first clock cycle after RESET becomes active, these signals are active HIGH. After this first clock, they float to 3-state OFF.
                    | ((p[0xc] & 0x08) != 0 ? 2 : 0)         // 27 -S1           0 = Interrupt Acknowledge, 1 = Read I/O Port, 2 = Write I/O Port, 3 = Halt, 4 = Code Access, 5 = Read Memory, 6 = Write Memory, 7 = Passive
                    | ((p[0xf] & 0x04) != 0 ? 4 : 0);        // 28 -S2
                char sc[] = "ARWHCrwp";

                //bool lock      = ((p[0xe] & 0x04) == 0);     // 29 -LOCK    !87  O LOCK: indicates that other system bus masters are not to gain control of the system bus while LOCK is active (LOW). The LOCK signal is activated by the ``LOCK'' prefix instruction and remains active until the completion of the next instruction. This signal is active LOW, and floats to 3-state off in ``hold acknowledge''.
                bool rqgt0     = ((p[0xc] & 0x04) == 0);     // 31 -RQ/-GT0 !87 IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
                //bool rqgt1     = ((p[0xd] & 0x04) == 0);     // 30 -RQ/-GT1     IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
                bool ready     = ((p[0xd] & 0x10) != 0);     // 22 READY        I  READY: is the acknowledgement from the addressed memory or I/O device that it will complete the data transfer. The RDY signal from memory or I/O is synchronized by the 8284 clock generator to form READY. This signal is active HIGH. The 8088 READY input is not synchronized. Correct operation is not guaranteed if the set up and hold times are not met.
                bool test      = ((p[0xc] & 0x10) == 0);     // 23 -TEST        I  TEST: input is examined by the ``wait for test'' instruction. If the TEST input is LOW, execution continues, otherwise the processor waits in an ``idle'' state. This input is synchronized internally during each clock cycle on the leading edge of CLK.
                //bool rd        = ((p[0x7] & 0x08) == 0);     // 32 -RD      !87  O READ: Read strobe indicates that the processor is performing a memory or I/O read cycle, depending on the state of the IO/M pin or S2. This signal is used to read devices which reside on the 8088 local bus. RD is active LOW during T2, T3 and Tw of any read cycle, and is guaranteed to remain HIGH in T2 until the 8088 local bus has floated. This signal floats to 3-state OFF in ``hold acknowledge''.
                //bool intr      = ((p[0xa] & 0x10) != 0);     // 18 INTR     !87 I  INTERRUPT REQUEST: is a level triggered input which is sampled during the last clock cycle of each instruction to determine if the processor should enter into an interrupt acknowledge operation. A subroutine is vectored to via an interrupt vector lookup table located in system memory. It can be internally masked by software resetting the interrupt enable bit. INTR is internally synchronized. This signal is active HIGH.
                bool cpu_clk   = ((p[0x9] & 0x10) != 0);     // 19 CLK          I  CLOCK: provides the basic timing for the processor and bus controller. It is asymmetric with a 33% duty cycle to provide optimized internal timing.
                //bool nmi       = ((p[0xb] & 0x10) != 0);     // 17 NMI      !87 I  NON-MASKABLE INTERRUPT: is an edge triggered input which causes a type 2 interrupt. A subroutine is vectored to via an interrupt vector lookup table located in system memory. NMI is not maskable internally by software. A transition from a LOW to HIGH initiates the interrupt at the end of the current instruction. This input is internally synchronized.

                UInt32 address =
                      ((p[0x4] & 0x02) != 0 ? 0x80000 : 0)   // A12 +A19         O Address bits: These lines are used to address memory and I/O devices within the system. These lines are generated by either the processor or DMA controller.
                    | ((p[0x5] & 0x02) != 0 ? 0x40000 : 0)   // A13 +A18
                    | ((p[0x6] & 0x02) != 0 ? 0x20000 : 0)   // A14 +A17
                    | ((p[0x7] & 0x02) != 0 ? 0x10000 : 0)   // A15 +A16
                    | ((p[0x4] & 0x01) != 0 ? 0x08000 : 0)   // A16 +A15
                    | ((p[0x5] & 0x01) != 0 ? 0x04000 : 0)   // A17 +A14
                    | ((p[0x6] & 0x01) != 0 ? 0x02000 : 0)   // A18 +A13
                    | ((p[0x7] & 0x01) != 0 ? 0x01000 : 0)   // A19 +A12
                    | ((p[0xc] & 0x20) != 0 ? 0x00800 : 0)   // A20 +A11
                    | ((p[0xd] & 0x20) != 0 ? 0x00400 : 0)   // A21 +A10
                    | ((p[0xe] & 0x20) != 0 ? 0x00200 : 0)   // A22 +A9
                    | ((p[0xf] & 0x20) != 0 ? 0x00100 : 0)   // A23 +A8
                    | ((p[0xc] & 0x40) != 0 ? 0x00080 : 0)   // A24 +A7
                    | ((p[0xd] & 0x40) != 0 ? 0x00040 : 0)   // A25 +A6
                    | ((p[0xe] & 0x40) != 0 ? 0x00020 : 0)   // A26 +A5
                    | ((p[0xf] & 0x40) != 0 ? 0x00010 : 0)   // A27 +A4
                    | ((p[0xc] & 0x80) != 0 ? 0x00008 : 0)   // A28 +A3
                    | ((p[0xd] & 0x80) != 0 ? 0x00004 : 0)   // A29 +A2
                    | ((p[0xe] & 0x80) != 0 ? 0x00002 : 0)   // A30 +A1
                    | ((p[0xf] & 0x80) != 0 ? 0x00001 : 0);  // A31 +A0
                UInt8 data =
                      ((p[0x6] & 0x20) != 0 ? 0x80 : 0)      // A2  +D7         IO Data bits: These lines provide data bus bits 0 to 7 for the processor, memory, and I/O devices.
                    | ((p[0x7] & 0x20) != 0 ? 0x40 : 0)      // A3  +D6
                    | ((p[0xc] & 0x01) != 0 ? 0x20 : 0)      // A4  +D5
                    | ((p[0xd] & 0x01) != 0 ? 0x10 : 0)      // A5  +D4
                    | ((p[0xe] & 0x01) != 0 ? 0x08 : 0)      // A6  +D3
                    | ((p[0xf] & 0x01) != 0 ? 0x04 : 0)      // A7  +D2
                    | ((p[0x4] & 0x04) != 0 ? 0x02 : 0)      // A8  +D1
                    | ((p[0x5] & 0x04) != 0 ? 0x01 : 0);     // A9  +D0
                UInt8 dma =
                      ((p[0x0] & 0x01) != 0 ? 0x02 : 0)      // B18 +DRQ1       I  DMA Request: These lines are asynchronous channel requests used by peripheral devices to gain DMA service. They are prioritized with DRQ3 being the lowest and DRQl being the highest. A request is generated by bringing a DRQ line to an active level (high). A DRQ line must be held high until the corresponding DACK line goes active.
                    | ((p[0x8] & 0x01) != 0 ? 0x04 : 0)      // B6  +DRQ2
                    | ((p[0x2] & 0x01) != 0 ? 0x08 : 0)      // B16 +DRQ3
                    | ((p[0xb] & 0x20) == 0 ? 0x10 : 0)      // B19 -DACK0       O -DMA Acknowledge: These lines are used to acknowledge DMA requests (DRQ1-DRQ3) and to refresh system dynamic memory (DACK0). They are active low.
                    | ((p[0x1] & 0x01) == 0 ? 0x20 : 0)      // B17 -DACK1
                    | ((p[0x8] & 0x40) == 0 ? 0x40 : 0)      // B26 -DACK2
                    | ((p[0x3] & 0x01) == 0 ? 0x80 : 0);     // B15 -DACK3
                UInt8 irq =
                      ((p[0xa] & 0x01) != 0 ? 0x04 : 0)      // B4  +IRQ2       I  Interrupt Request lines: These lines are used to signal the processor that an I/O device requires attention. An Interrupt Request is generated by raising an IRQ line (low to high) and holding it high until it is acknowledged by the processor (interrupt service routine).
                    | ((p[0x9] & 0x40) != 0 ? 0x08 : 0)      // B25 +IRQ3
                    | ((p[0xa] & 0x40) != 0 ? 0x10 : 0)      // B24 +IRQ4
                    | ((p[0xb] & 0x40) != 0 ? 0x20 : 0)      // B23 +IRQ5
                    | ((p[0x8] & 0x20) != 0 ? 0x40 : 0)      // B22 +IRQ6
                    | ((p[0x9] & 0x20) != 0 ? 0x80 : 0);     // B21 +IRQ7

                bool ior       = ((p[0x0] & 0x02) == 0);     // B14 -IOR         O -I/O Read Command: This command line instructs an I/O device to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool iow       = ((p[0x1] & 0x02) == 0);     // B13 -IOW         O -I/O Write Command: This command line instructs an I/O device to read the data on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool memr      = ((p[0x2] & 0x02) == 0);     // B12 -MEMR        O Memory Read Command: This command line instructs the memory to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool memw      = ((p[0x3] & 0x02) == 0);     // B11 -MEMW        O Memory Write Command: This command line instructs the memory to store the data present on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.

                //bool bus_reset = ((p[0x0] & 0x20) != 0);     // B2  +RESET DRV   O This line is used to reset or initialize system logic upon power-up or during a low line voltage outage. This signal is synchronized to the falling edge of clock and is active high.
                //bool iochchk   = ((p[0x5] & 0x20) == 0);     // A1  -I/O CH CK  I  -I/O Channel Check: This line provides the processor with parity (error) information on memory or devices in the I/O channel. When this signal is active low, a parity error is indicated.
                bool iochrdy   = ((p[0x6] & 0x04) != 0);     // A10 +I/O CH RDY I  I/O Channel Ready: This line, normally high (ready), is pulled low (not ready) by a memory or I/O device to lengthen I/O or memory cycles. It allows slower devices to attach to the I/O channel with a minimum of difficulty. Any slow device using this line should drive it low immediately upon detecting a valid address and a read or write command. This line should never be held low longer than 10 clock cycles. Machine cycles (I/O or memory) are extended by an integral number of CLK cycles (210 ns).
                bool aen       = ((p[0x7] & 0x04) != 0);     // A11 +AEN         O Address Enable: This line is used to de-gate the processor and other devices from the I/O channel to allow DMA transfers to take place. When this line is active (high), the DMA controller has control of the address bus, data bus, read command lines (memory and I/O), and the write command lines (memory and I/O).
                //bool bus_clk   = ((p[0xa] & 0x20) != 0);     // B20 CLOCK        O System clock: It is a divide-by-three of the oscillator and has a period of 210 ns (4.77 MHz). The clock has a 33% duty cycle.
                bool bus_ale   = ((p[0xa] & 0x80) != 0);     // B28 +ALE         O Address Latch Enable: This line is provided by the 8288 Bus Controller and is used on the system board to latch valid addresses from the processor. It is available to the I/O channel as an indicator of a valid processor address (when used with AEN). Processor addresses are latched with the failing edge of ALE.
                bool tc        = ((p[0xb] & 0x80) != 0);     // B27 +T/C         O Terminal Count: This line provides a pulse when the terminal count for any DMA channel is reached. This signal is active high.

                //UInt16 jumpers =  // Should always be 0
                //      ((p[0x0] & 0x04) != 0 ? 0x00001 : 0)    // JP4/4
                //    | ((p[0x1] & 0x04) != 0 ? 0x00002 : 0)    // JP4/3
                //    | ((p[0x1] & 0x20) != 0 ? 0x00004 : 0)    // JP9/4
                //    | ((p[0x2] & 0x04) != 0 ? 0x00008 : 0)    // JP4/2
                //    | ((p[0x2] & 0x20) != 0 ? 0x00010 : 0)    // JP9/3
                //    | ((p[0x3] & 0x04) != 0 ? 0x00020 : 0)    // JP4/1
                //    | ((p[0x3] & 0x10) != 0 ? 0x00040 : 0)    // JP5/1
                //    | ((p[0x3] & 0x20) != 0 ? 0x00080 : 0)    // JP9/2
                //    | ((p[0x4] & 0x20) != 0 ? 0x00100 : 0)    // JP9/1
                //    | ((p[0x8] & 0x80) != 0 ? 0x00200 : 0)    // JP7/2
                //    | ((p[0x9] & 0x01) != 0 ? 0x00400 : 0)    // JP3/1
                //    | ((p[0x9] & 0x80) != 0 ? 0x00800 : 0)    // JP7/1
                //    | ((p[0xf] & 0x10) != 0 ? 0x01000 : 0)    // JP6/1
                //    | ((p[0x6] & 0x08) == 0 ? 0x02000 : 0)    // 33 MN/-MX     I    MINIMUM/MAXIMUM: indicates what mode the processor is to operate in. The two modes are discussed in the following sections. (zero for maximum mode on PC/XT)
                //    | ((p[0x5] & 0x08) == 0 ? 0x04000 : 0)    // 34 -SS0        O   Pin 34 is always high in the maximum mode.
                //    | ((p[0xb] & 0x01) == 0 ? 0x08000 : 0)    // Serial - should always be 1
                //    | ((p[0x8] & 0x10) != 0 ? 0x10000 : 0)    // GND - should always be 0
                //    | ((p[0xe] & 0x10) != 0 ? 0x20000 : 0);   // RESET - should always be 0

                _item->write(String(hex(cpu, 5, false)) + " " +
                    codePoint(qsc[qs]) + codePoint(sc[s]) +
                    (rqgt0 ? "1" : ".") + (ready ? "R" : ".") +
                    (test ? "T" : ".") + (cpu_clk ? "C" : ".") + "  " +
                    hex(address, 5, false) + " " + hex(data, 2, false) + " " +
                    hex(dma, 2, false) + " " + hex(irq, 2, false) + " " +
                    (ior ? "R" : ".") + (iow ? "W" : ".") +
                    (memr ? "r" : ".") + (memw ? "w" : ".") +
                    (iochrdy ? "." : "z") + (aen ? "A" : ".") +
                    (bus_ale ? "a" : ".") + (tc ? "T" : ".") + "\n");
            }
    }



    String _fromAddress;
    String _lamePath;
    String _lameOptions;
    String _adminAddress;
    String _htdocsPath;
    String _captureFieldPath;
    LinkedList<QueueItem> _queue;
    int _queuedItems;

    bool _processing;
    Array<Byte> _packet;
    QueueItem* _item;
    bool _snifferActive;
    bool _killed;
    bool _cancelled;
    Array<Byte> _diskBytes;
    Byte _hostBytes[21];

    String _imager;
    String _vDos;

    int _imageCount;
    Reference<AudioCapture> _audioCapture;
    int _audioCount;
    Byte _hostInterruptOperation;
    Byte _diskError;
    int _fileState;
    Array<Byte> _diskData;
    DiskImage _diskImage;
    int _diskByteCount;
    int _diskDataPointer;
    Byte _diskSectorCount;

    DWORD _startTime;

    INPUT_RECORD _inputBuf[128];
    DWORD _inputCount;
    HANDLE _hInput;


    // EmailThread
    LinkedList<QueueItem> _emailQueue;

    // SnifferThread
    Array<Byte> _data;
    int _lengths[48];
};
