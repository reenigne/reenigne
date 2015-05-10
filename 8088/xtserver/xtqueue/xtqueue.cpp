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
        write(s.data(), s.length());
    }
    void writeNoEmail(const String& s)
    {
        write(s.data(), s.length());
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

// We want to send an email to the user if and only if the HTTP connection was
// terminated before all the information was sent. However, the only way to
// know if the connection was terminated is to send data across it and wait for
// a few seconds. If the connection was terminated the CGI process will be
// terminated when Apache notices that the last transmission was not received.
// So this thread just waits for 5 seconds after the last transmission and then
// deletes the item - the item then checks to see if the server process went
// away, and if so sends email.
class EmailThread : public Thread
{
public:
    EmailThread() : _ending(false) { }
    ~EmailThread() { _ending = true; _ready.signal(); }
    void add(QueueItem* item)
    {
        console.write("Adding item to email thread.\n");
        item->setFinishTime(GetTickCount());
        Lock lock(&_mutex);
        _queue.add(item);
        _ready.signal();
        console.write("Item added to email thread.\n");
    }
    void threadProc()
    {
        do {
            try {
                QueueItem* item;
                {
                    Lock lock(&_mutex);
                    item = _queue.getNext();
                    if (item != 0)
                        item->remove();
                }
                if (item == 0) {
                    console.write("Email thread entering waiting state.\n");
                    // We have nothing to do - stop the the thread until we do.
                    _ready.wait();
                    console.write("Email thread unblocked.\n");
                    if (_ending)
                        break;
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
            catch (Exception& e)
            {
                console.write("Exception in email thread: " + e.message() +
                    "\n");
            }
            catch (...)
            {
                console.write("Unknown exception in email thread.\n");
            }
        } while (true);
    }
private:
    LinkedList<QueueItem> _queue;
    Mutex _mutex;
    Event _ready;
    bool _ending;
};

class SerialThread : public Thread
{
public:
    void setHandle(Handle serialPort)
    {
        _serialPort = serialPort;
        memset(&_overlapped, 0, sizeof(OVERLAPPED));
        _overlappedEvent = Event(CreateEvent(NULL, TRUE, FALSE, NULL));
        _overlapped.hEvent = _overlappedEvent;
    }
    void threadProc()
    {
        do {
            Byte b;
            ReadFile(_serialPort, &b, 1, NULL, &_overlapped);
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
                throw Exception::systemError("Reading serial port");
            DWORD bytes;
            IF_ZERO_THROW(
                GetOverlappedResult(_serialPort, &_overlapped, &bytes, TRUE));
            if (bytes == 0)
                continue;
            if (bytes != 1)
                throw Exception::systemError("Reading serial port");
            //console.write<Byte>('%');
            _c = b;
            //console.write(String("[") + decimal(b) + "]");

            //DWORD mask = EV_RXFLAG;
            //if (WaitCommEvent(_serialPort, &mask, NULL) == 0)
            //    break;
            //_c = _serialPort.tryReadByte();
            //if (_c == -1)
            //    continue;
            _event.signal();
            _reset.wait();
        } while (true);
    }
    HANDLE eventHandle() { return _event; }
    int character()
    {
        int c = _c;
        _event.reset();
        _reset.signal();
        return c;
    }
private:
    Handle _serialPort;
    Event _overlappedEvent;
    Event _event;
    Event _reset;
    int _c;
    OVERLAPPED _overlapped;
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

class XTThread : public Thread
{
public:
    XTThread(ConfigFile* configFile)
      : _queuedItems(0), _ending(false), _processing(false),
        _diskBytes(0x10000), _item(0)
    {
        String quickBootPort = configFile->get<String>("quickBootPort");
        _quickBootBaudRate = configFile->get<int>("quickBootBaudRate");
        _fromAddress = configFile->get<String>("fromAddress");
        _lamePath = configFile->get<String>("lamePath");
        _lameOptions = configFile->get<String>("lameOptions");
        _adminAddress = configFile->get<String>("adminAddress");
        _htdocsPath = configFile->get<String>("htdocsPath");
        _captureFieldPath = configFile->get<String>("captureFieldPath");

        // Open handle to Arduino for rebooting machine
        NullTerminatedWideString quickBootPath(quickBootPort);
        _arduinoCom = AutoHandle(CreateFile(
            quickBootPath,
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            FILE_FLAG_OVERLAPPED,
            NULL),          // hTemplate must be NULL for comm devices
            String("Quickboot COM port"));

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        IF_ZERO_THROW(GetCommState(_arduinoCom, &deviceControlBlock));
        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = _quickBootBaudRate;
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
        IF_ZERO_THROW(SetCommState(_arduinoCom, &deviceControlBlock));

        //IF_ZERO_THROW(SetCommMask(_arduinoCom, EV_RXCHAR));

        //COMMTIMEOUTS timeOuts;
        //SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
        //timeOuts.ReadIntervalTimeout = 10*1000;
        //timeOuts.ReadTotalTimeoutMultiplier = 0;
        //timeOuts.ReadTotalTimeoutConstant = 10*1000;
        //timeOuts.WriteTotalTimeoutConstant = 10*1000;
        //timeOuts.WriteTotalTimeoutMultiplier = 0;
        //IF_ZERO_THROW(SetCommTimeouts(_arduinoCom, &timeOuts));

        _serialThread.setHandle(_arduinoCom);

        //reboot();

        //_imager = File(configFile->get<String>("imagerPath"), true).contents();
        _imager = configFile->get<String>("imagerPath");
        _vDos = configFile->get<String>("vDosPath");

        _packet.allocate(0x106);

        _emailThread.start();
        _serialThread.start();

        memset(&_overlapped, 0, sizeof(OVERLAPPED));
        _overlappedEvent = Event(CreateEvent(NULL, TRUE, FALSE, NULL));
        _overlapped.hEvent = _overlappedEvent;
    }
    ~XTThread() { _ending = true; _ready.signal(); }
    void run(AutoHandle pipe)
    {
        QueueItem* item = new QueueItem(pipe, _fromAddress);

        switch (item->command()) {
            case 0:
            case 4:  // Run via xtrun (no timeout)
                {
                    // Run a program
                    Lock lock(&_mutex);
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

                    _ready.signal();
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
                    Lock lock(&_mutex);
                    if (n == 0) {
                        _killed = true;
                        _interrupt.signal();
                    }
                    else {
                        --n;
                        QueueItem* i = _queue.getNext();
                        while (i != 0) {
                            QueueItem* nextItem = _queue.getNext(i);
                            if (n == 0) {
                                --_queuedItems;
                                i->remove();
                                i->kill();
                                break;
                            }
                            --n;
                            i = nextItem;
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
                    Lock lock(&_mutex);
                    String newSecret = item->fileName().subString(7, 16);
                    if (_item != 0 && newSecret == _item->secret()) {
                        _cancelled = true;
                        _interrupt.signal();
                        item->cancel();
                    }
                    else {
                        QueueItem* i = _queue.getNext();
                        while (i != 0) {
                            QueueItem* nextItem = _queue.getNext(i);
                            if (i->secret() == item->secret()) {
                                --_queuedItems;
                                i->remove();
                                i->cancel();
                                item->cancel();
                                return;
                            }
                            i = nextItem;
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

        //IF_ZERO_THROW(FlushFileBuffers(_arduinoCom));
        //IF_ZERO_THROW(PurgeComm(_arduinoCom, PURGE_RXCLEAR | PURGE_TXCLEAR));

        // Reset the Arduino
        EscapeCommFunction(_arduinoCom, CLRDTR);
        Sleep(250);
        _serialThread.character();
        EscapeCommFunction(_arduinoCom, SETDTR);

        // The Arduino bootloader waits a bit to see if it needs to
        // download a new program.

        //int i = 0;
        //do {
        //    Byte b = _arduinoCom.tryReadByte();
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
    // returns: 0 for timed out, killed or cancelled
    // 1 for input from serial inport
    // 2 for input from console keyboard
    int waitInput()
    {
        do {
            DWORD elapsed = GetTickCount() - _startTime;
            int handleCount = 3;
            if (_item->command() == 4) {
                elapsed = 0;
                handleCount = 4;
            }
            DWORD timeout = 5*60*1000 - elapsed;
            if (timeout == 0 || timeout > 5*60*1000)
                return 0;
            if (_killed || _cancelled)
                return 0;

            HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
            HANDLE handles[4] = {_serialThread.eventHandle(), hInput,
                _interrupt, _item->serverProcess()};
            DWORD result = WaitForMultipleObjects(handleCount, handles, FALSE,
                timeout);
            IF_FALSE_THROW(result != WAIT_FAILED);
            if (result == WAIT_TIMEOUT && _item->command() == 0)
                return 0;
            if (result == WAIT_OBJECT_0)
                return 1;
            if (result == WAIT_OBJECT_0 + 1)
                return 2;
            if (result == WAIT_OBJECT_0 + 2) {
                _interrupt.reset();
                return 0;
            }
            if (result == WAIT_OBJECT_0 + 3)
                return 0;
        } while (true);
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

        //IF_ZERO_THROW(FlushFileBuffers(_arduinoCom));
        //IF_ZERO_THROW(PurgeComm(_arduinoCom, PURGE_RXCLEAR | PURGE_TXCLEAR));

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
                //IF_ZERO_THROW(FlushFileBuffers(_arduinoCom));

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
        //IF_ZERO_THROW(FlushFileBuffers(_arduinoCom));
        return false;
    }
    String htDocsPath(String name) { return _htdocsPath + "\\" + name; }
    void threadProc()
    {
        do {
            _item = 0;
            try {
                _processing = false;
                // TODO: There might be some threading issues here:
                //   1 - _queue is not volatile - will this thread notice the
                //       change from the other thread?
                if (_queue.getNext() == 0) {
                    // We have nothing to do - stop the the thread until we do.
                    console.write("XT Thread going idle\n");
                    reboot();
                    _ready.wait();
                }
                if (_ending) {
                    console.write("XT Thread ending\n");
                    break;
                }
                {
                    Lock lock(&_mutex);
                    // We have something to do. Tell all the threads their
                    // position in the queue.
                    QueueItem* i = _queue.getNext();
                    int p = 0;
                    while (i != 0) {
                        i->notifyQueuePosition(p);
                        bool aborted = i->aborted();
                        QueueItem* nextItem = _queue.getNext(i);
                        if (aborted) {
                            i->remove();
                            --_queuedItems;
                            delete i;
                        }
                        ++p;
                        i = nextItem;
                    }
                    _item = _queue.getNext();
                    if (_item != 0) {
                        _item->remove();
                        --_queuedItems;
                    }
                    _interrupt.reset();
                    _cancelled = false;
                    _killed = false;
                }
                if (_item == 0)
                    continue;
                _item->printInfo();
                _processing = true;

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
                        PurgeComm(_arduinoCom, PURGE_RXCLEAR | PURGE_TXCLEAR));

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
        } while (true);
    }
private:
    void sendByte(int byte)
    {
        console.write(String("(") + debugByte(byte) + ")");
        writeSerial(reinterpret_cast<const Byte*>(&byte), 1);
        //_arduinoCom.write<Byte>(byte);
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
                INPUT_RECORD buf[128];
                DWORD count;
                HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
                IF_ZERO_THROW(ReadConsoleInput(hInput, buf, 128, &count));
                for (int i = 0; i < static_cast<int>(count); ++i) {
                    //console.write(String(decimal(buf[i].EventType)) + " ");
                    if (buf[i].EventType == KEY_EVENT) {
                        int key = buf[i].Event.KeyEvent.wVirtualKeyCode;
                        bool up = !buf[i].Event.KeyEvent.bKeyDown;
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
                                if (buf[i].Event.KeyEvent.wVirtualScanCode ==
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
            if (_fileState != 0)
                console.write(String("[") + debugByte(c) + String("]"));
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
        WriteFile(_arduinoCom, data, bytes, NULL, &_overlapped);
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING)
            throw Exception::systemError("Writing serial port");
        DWORD bytesWritten;
        IF_ZERO_THROW(GetOverlappedResult(_arduinoCom, &_overlapped,
            &bytesWritten, TRUE));
        if (bytes != bytesWritten)
            throw Exception::systemError("Writing serial port");
    }

    String _fromAddress;
    String _lamePath;
    String _lameOptions;
    String _adminAddress;
    String _htdocsPath;
    String _captureFieldPath;
    int _quickBootBaudRate;
    LinkedList<QueueItem> _queue;
    int _queuedItems;

    volatile bool _processing;
    volatile bool _ending;
    Mutex _mutex;
    Event _ready;
    Event _interrupt;
    AutoHandle _arduinoCom;
    AutoHandle _com;
    Array<Byte> _packet;
    QueueItem* _item;
    EmailThread _emailThread;
    SerialThread _serialThread;
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

    Event _overlappedEvent;
    OVERLAPPED _overlapped;
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
        configFile.addDefaultOption("quickBootPort", String("COM4"));
        configFile.addDefaultOption("quickBootBaudRate", 19200);
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
        configFile.load(File(_arguments[1], CurrentDirectory(), true));

        COMInitializer com;
        XTThread xtThread(&configFile);
        xtThread.start();
        while (true)
        {
            console.write("Waiting for connection\n");
            AutoHandle h =
                File(configFile.get<String>("pipe"), true).createPipe();

            bool connected = (ConnectNamedPipe(h, NULL) != 0) ? true :
                (GetLastError() == ERROR_PIPE_CONNECTED);

            if (connected) {
                console.write("Connected\n");
                xtThread.run(h);
            }
        }
    }
};
