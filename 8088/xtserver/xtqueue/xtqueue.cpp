#include "alfe/main.h"
#include "alfe/thread.h"
#include "alfe/stack.h"
#include "alfe/linked_list.h"
#include "alfe/email.h"
#include "alfe/com.h"
#include "alfe/config_file.h"
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
        if (_command == 0)
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
        console.write("Starting work item" + _logFile + ": " + _fileName +
            "for" + _email + "\n");
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
        _needArduinoReset(false), _needReboot(false), _diskBytes(0x10000)
    {
        String quickBootPort = configFile->get<String>("quickBootPort");
        int quickBootBaudRate = configFile->get<int>("quickBootBaudRate");
        String serialPort = configFile->get<String>("serialPort");
        int serialBaudRate = configFile->get<int>("serialBaudPrate");
        _fromAddress = configFile->get<String>("fromAddress");
        _lamePath = configFile->get<String>("lamePath");
        _lameOptions = configFile->get<String>("lameOptions");
        _adminAddress = configFile->get<String>("adminAddress");
        _htdocsPath = configFile->get<String>("htdocsPath");
        _captureFieldPath = configFile->get<String>("captureFieldPath");

        // Open handle to Arduino for rebooting machine
#if 1
        NullTerminatedWideString quickBootPath(quickBootPort);
        _arduinoCom = AutoHandle(CreateFile(
            quickBootPath,
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            0,              // not overlapped I/O
            NULL),          // hTemplate must be NULL for comm devices
            String("Quickboot COM port"));

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        IF_ZERO_THROW(GetCommState(_arduinoCom, &deviceControlBlock));
        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = quickBootBaudRate;
        deviceControlBlock.fBinary = TRUE;
        deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = FALSE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_DISABLE;
        deviceControlBlock.fDsrSensitivity = FALSE;
        deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = TRUE;
        deviceControlBlock.fInX = TRUE;
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

        IF_ZERO_THROW(SetCommMask(_arduinoCom, EV_RXCHAR));


        // Open handle to serial port for data transfer
        NullTerminatedWideString serialPath(serialPort);
        _com = AutoHandle(CreateFile(
            serialPath,
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            0,              // not overlapped I/O
            NULL),          // hTemplate must be NULL for comm devices
            String("COM port"));

        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        IF_ZERO_THROW(GetCommState(_com, &deviceControlBlock));
        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = serialBaudRate;
        deviceControlBlock.fBinary = TRUE;
        deviceControlBlock.fParity = FALSE;
        deviceControlBlock.fOutxCtsFlow = FALSE;
        deviceControlBlock.fOutxDsrFlow = TRUE;
        //deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;
        deviceControlBlock.fDtrControl = DTR_CONTROL_HANDSHAKE;
        deviceControlBlock.fDsrSensitivity = FALSE; //TRUE;
        deviceControlBlock.fTXContinueOnXoff = TRUE;
        deviceControlBlock.fOutX = FALSE;
        deviceControlBlock.fInX = FALSE;
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
        IF_ZERO_THROW(SetCommState(_com, &deviceControlBlock));
        IF_ZERO_THROW(ClearCommError(_com, NULL, NULL));

        IF_ZERO_THROW(SetCommMask(_com, EV_RXCHAR));

        COMMTIMEOUTS timeOuts;
        SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
        timeOuts.ReadIntervalTimeout = 10*1000;
        timeOuts.ReadTotalTimeoutMultiplier = 0;
        timeOuts.ReadTotalTimeoutConstant = 10*1000;
        timeOuts.WriteTotalTimeoutConstant = 10*1000;
        timeOuts.WriteTotalTimeoutMultiplier = 0;
        IF_ZERO_THROW(SetCommTimeouts(_com, &timeOuts));
#endif 

        //_imager = File("C:\\imager.bin", true).contents();

        _packet.allocate(0x101);

        _emailThread.start();
    }
    ~XTThread() { _ending = true; _ready.signal(); }
    void run(AutoHandle pipe)
    {
        QueueItem* item = new QueueItem(pipe, _fromAddress);

        switch (item->command()) {
            case 0:
                {
                    // Run a program
                    Lock lock(&_mutex);
                    _queue.add(item);
                    item->writeNoEmail("<form action='http://reenigne.dyndns.o"
                        "rg/cgi-bin/xtcancel.exe' method='post'>\n"
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
                    if (n == 0)
                        _killed = true;
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
        if (!_needReboot)
            false;
        bothWrite("Resetting\n");

        if (_needArduinoReset) {
            // Reset the Arduino
            EscapeCommFunction(_arduinoCom, CLRDTR);
            EscapeCommFunction(_arduinoCom, CLRRTS);
            Sleep(250);
            EscapeCommFunction(_arduinoCom, SETDTR);
            EscapeCommFunction(_arduinoCom, SETRTS);
            // The Arduino bootloader waits a bit to see if it needs to
            // download a new program.
            Sleep(2000);
            _needArduinoReset = false;
        }

        // Reset the machine
        _arduinoCom.write<Byte>(0x7f);
        _arduinoCom.write<Byte>(0x77);
        IF_ZERO_THROW(FlushFileBuffers(_arduinoCom));
        _needReboot = false;
    }
    void bothWrite(String s)
    {
        console.write(s);
        if (_item != 0)
            _item->write(s);
    }
    bool upload(String program)
    {
        int retry = 1;
        bool error;
        do {
            IF_ZERO_THROW(PurgeComm(_com, PURGE_RXCLEAR | PURGE_TXCLEAR));

            error = false;
            reboot();

            bothWrite("Transferring attempt " + String::Decimal(retry) + "\n");

            int l = program.length();

            Byte checksum;

            int p = 0;
            int bytes;
            int timeouts = 10;
            do {
                int p0 = p;
                bytes = min(l, 0xff);
                _packet[0] = bytes;
                checksum = 0;
                for (int i = 0; i < bytes; ++i) {
                    Byte d = program[p];
                    ++p;
                    _packet[i + 1] = d;
                    checksum += d;
                }
                _packet[bytes + 1] = checksum;
                _com.write(&_packet[0], 2 + _packet[0]);
                IF_ZERO_THROW(FlushFileBuffers(_com));
                Byte b = _com.tryReadByte();
                bothWrite(String::Decimal(b));
                if (b == 255) {
                    --timeouts;
                    if (timeouts == 0)
                        error = true;
                    else {
                        p = p0;
                        continue;
                    }
                }
                else {
                    timeouts = 10; 
                    if (b != 'K')
                        error = true;
                }
                l -= bytes;
                if (_killed || _cancelled) {
                    _needReboot = true;
                    break;
                }
            } while (bytes != 0);

            if (error) {
                ++retry;
                _needReboot = true;
                if (retry >= 3)
                    _needArduinoReset = true;
            }
            else
                break;
        } while (retry < 10);
        return error;
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
                    reboot();
                    // We have nothing to do - stop the the thread until we do.
                    console.write("XT Thread going idle\n");
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
                int sectorsPerTrack = 9;
                int heads = 2;
                int bytesPerSector = 512;
                if (extension.equalsIgnoreCase(".img")) {
                    program = _imager;
                    switch (program.length()) {
                        case 320*1024:
                            sectorsPerTrack = 8;
                            break;
                        case 160*1024:
                            sectorsPerTrack = 8;
                            heads = 1;
                            break;
                        case 180*1024:
                            heads = 1;
                            break;
                    }
                }
                else
                    //if (extension.equalsIgnoreCase(".com")) {
                    //    String::Buffer header(0x100);
                    //    Byte* p = header.data();
                    //    p[0] = 0xe9;
                    //    p[1] = 0xfd;
                    //    p[2] = 0x00;
                    //    for (int i = 3; i < 0x100; ++i)
                    //        p[i] = 0x90;
                    //    program = String(header, 0, 0x100) + _item->data();
                    //}
                    //else
                        program = _item->data();
                bool error = upload(program);
                if (error) {
                    console.write("Failed to upload!\n");
                    _item->write("Could not transfer the program to the XT. "
                        "The machine may be offline for maintainance - please "
                        "try again later.\n");
                    _needReboot = true;
                    delete _item;
                    continue;
                }

                bothWrite("Upload complete.\n");
                if (_item->aborted()) {
                    _needReboot = true;
                    delete _item;
                    continue;
                }
                // Dump bytes from COM port to pipe until we receive ^Z or we
                // time out.

                DWORD startTime = GetTickCount();
                bool timedOut = false;
                bool escape = false;
                bool audio = false;
                int fileState = 0;
                int fileSize = 0;
                //String fileName;
                bool complete = false;
                int filePointer;
                Array<Byte> file;
                int fileCount = 0;
                int imageCount = 0;
                int audioCount = 0;
                int hostBytesRemaining = 0;
                int diskByteCount = 0;
                int diskDataPointer;
                Reference<AudioCapture> audioCapture;
                Byte hostBytes[18];
                do {
                    DWORD elapsed = GetTickCount() - startTime;
                    DWORD timeout = 5*60*1000 - elapsed;
                    if (timeout == 0 || timeout > 5*60*1000) {
                        timedOut = true;
                        break;
                    }
                    if (_killed || _cancelled)
                        break;

                    int c = _com.tryReadByte();
                    if (c == -1)
                        continue;
                    if (!escape && fileState != 5) {
                        bool processed = false;
                        switch (c) {
                            case 0x00:
                                // Transfer following byte directly, don't
                                // interpret it as an action.
                                escape = true;
                                processed = true;
                                break;
                            case 0x01:
                                // Take screenshot
                                {
                                    processed = true;
                                    String fileName = _item->secret() +
                                        imageCount + ".png";
                                    String path = htDocsPath(fileName);

                                    String commandLine = "\"" +
                                        _captureFieldPath + "\" \"" + path +
                                        "\"";
                                    NullTerminatedWideString data(commandLine);

                                    PROCESS_INFORMATION pi;
                                    ZeroMemory(&pi,
                                        sizeof(PROCESS_INFORMATION));

                                    STARTUPINFO si;
                                    ZeroMemory(&si, sizeof(STARTUPINFO));
                                    si.cb = sizeof(STARTUPINFO);

                                    IF_FALSE_THROW(CreateProcess(NULL, data,
                                        NULL, NULL, FALSE, 0, NULL, NULL, &si,
                                        &pi) != 0);
                                    CloseHandle(pi.hThread);
                                    AutoHandle hLame = pi.hProcess;
                                    IF_FALSE_THROW(
                                        WaitForSingleObject(hLame, 3*60*1000)
                                        == WAIT_OBJECT_0);

                                    _item->write("\n<img src=\"../" +
                                        fileName + "\"/>\n");
                                    ++imageCount;
                                }
                                break;
                            case 0x02:
                                // Start recording audio
                                {
                                    audioCapture = new AudioCapture;
                                    processed = true;
                                    audio = true;
                                }
                                break;
                            case 0x03:
                                // Stop recording audio
                                {
                                    processed = true;
                                    if (!audio)
                                        break;
                                    String rawName = _item->secret() +
                                        audioCount;
                                    String baseName = htDocsPath(rawName);
                                    String waveName = baseName + ".pcm";
                                    File wave(waveName, true);
                                    audioCapture->finish(wave);
                                    audioCapture = 0;
                                    String commandLine = "\"" + _lamePath +
                                        "\" \"" + waveName + "\" \"" +
                                        fileName + ".mp3\" " + _lameOptions;
                                    NullTerminatedWideString data(commandLine);

                                    PROCESS_INFORMATION pi;
                                    ZeroMemory(&pi,
                                        sizeof(PROCESS_INFORMATION));

                                    STARTUPINFO si;
                                    ZeroMemory(&si, sizeof(STARTUPINFO));
                                    si.cb = sizeof(STARTUPINFO);

                                    IF_FALSE_THROW(CreateProcess(NULL, data,
                                        NULL, NULL, FALSE, 0, NULL, NULL, &si,
                                        &pi) != 0);
                                    CloseHandle(pi.hThread);
                                    AutoHandle hLame = pi.hProcess;
                                    IF_FALSE_THROW(
                                        WaitForSingleObject(hLame, 3*60*1000)
                                        == WAIT_OBJECT_0);

                                    wave.remove();

                                    _item->write("\n<embed height=\"50\" "
                                        "width=\"100\" src=\"../" + rawName +
                                        ".mp3\"><a href=\"../" + rawName +
                                        ".mp3\">Recorded audio</a></embed>\n");
                                    ++audioCount;
                                }
                                break;
                            case 0x04:
                                // Transfer file
                                fileState = 1;
                                processed = true;
                                break;
                            case 0x05:
                                // Host interrupt
                                fileState = 5;
                                hostBytesRemaining = 18;
                                processed = true;
                                break;
                            case 0x1a:
                                complete = true;
                                processed = true;
                                break;
                        }
                        if (c != 0)
                            escape = false;
                        if (complete)
                            break;
                        if (processed)
                            continue;
                    }
                    escape = false;
                    switch (fileState) {
                        case 0:
                            // No file operation in progress - output to HTTP
                            if (c == '<')
                                _item->write("&lt;");
                            else
                                if (c == '&')
                                    _item->write("&amp;");
                                else
                                    _item->write(c);
                            if ((c < 32 || c > 126) &&
                                (c != 9 && c != 10 && c != 13))
                                console.write<Byte>('.');
                            else
                                console.write<Byte>(c);
                            break;
                        case 1:
                            // Get first byte of size
                            fileSize = c;
                            fileState = 2;
                            break;
                        case 2:
                            // Get second byte of size
                            fileSize |= (c << 8);
                            fileState = 3;
                            break;
                        case 3:
                            // Get third byte of size
                            fileSize |= (c << 16);
                            fileState = 4;
                            filePointer = 0;
                            file.allocate(fileSize);
                            break;
                        case 4:
                            // Get file data
                            file[filePointer++] = c;
                            if (filePointer == fileSize) {
                                fileState = 0;
                                String fileName = _item->secret() + fileCount +
                                    ".dat";
                                File(htDocsPath(fileName), true).save(file);
                                _item->write("\n<a href=\"../" + fileName +
                                    "\">Captured file</a>\n");
                                ++fileCount;
                            }
                            break;
                        //case 5:
                        //    // Get host interrupt data
                        //    hostBytes[17 - hostBytesRemaining] = c;
                        //    --hostBytesRemaining;
                        //    if (hostBytesRemaining != 0)
                        //        break;
                        //    fileState = 0;
                        //    if (hostBytes[0] != 0x13) {
                        //        bothWrite("Unknown host interrupt " +
                        //            String::Hex(hostBytes[0], 2, true));
                        //        break;
                        //    }
                        //    // The host bytes are as follows:
                        //    Byte sectorCount = hostBytes[1];
                        //    Byte operation = hostBytes[2];
                        //    Byte sector = hostBytes[3] & 0x3f;
                        //    int error = 0;
                        //    if (sector >= 9)
                        //        error = 4;
                        //    Word track = hostBytes[4] |
                        //        ((hostBytes[3] & 0xc0) << 2);
                        //    if (track >= 40)
                        //        error = 4;
                        //    Byte drive = hostBytes[5];
                        //    if (drive != 0)
                        //        error = 4;
                        //    Byte head = hostBytes[6];
                        //    if (head >= 2)
                        //        error = 4;
                        //    //  7 == step rate time / head unload time
                        //    //  8 == head load time / DMA mode
                        //    //  9 == motor shutoff time
                        //    int sectorSize = 128 << hostBytes[10];
                        //    if (sectorSize != bytesPerSector)
                        //        error = 4;
                        //    Byte sectorsPerTrack = hostBytes[11];
                        //    // 12 == gap length for read/write/verify
                        //    // 13 == data length
                        //    // 14 == gap length for format
                        //    // 15 == fill byte for format
                        //    // 16 == head settle time
                        //    // 17 == motor startup time

                        //    int start = ((track*heads + head)*sectorsPerTrack + 
                        //        sector)*bytesPerSector;
                        //    int length = sectorCount*bytesPerSector;
                        //    String image = _item->data();
                        //    if (operation != 5 &&
                        //        start + length > image.length())
                        //        error = 4;
                        //    Byte status[3];
                        //    status[0] = (error != 0 ? sectorCount : 0);
                        //    status[1] = error;
                        //    status[2] = (error != 0 ? 3 : 2);
                        //    switch (operation) {
                        //        case 2:
                        //            // Read disk sectors
                        //            if (error != 0)
                        //                upload("");
                        //            else
                        //                upload(image.
                        //                    subString(start, length));
                        //            break;
                        //        case 3:
                        //            // Write disk sectors
                        //        case 4:
                        //            // Verify disk sectors
                        //            diskByteCount = length;
                        //            diskDataPointer = 0;
                        //            fileState = 6;
                        //            break;
                        //        case 5:
                        //            // Format disk sectors
                        //            diskByteCount = 
                        //            break;
                        //    }
                        //    upload(String(reinterpret_cast<const char*>
                        //        (&status[0]), 3));
                        //    break;
                        //case 6:
                        //    // Get disk data
                        //    _diskBytes[diskDataPointer] = c;
                        //    ++diskDataPointer;
                        //    if (diskDataPointer != diskByteCount)
                        //        break;
                        //    fileState = 0;

                            
                    }
                    if (_item->aborted())
                        break;
                } while (true);
                console.write("\n");
                _item->write("\n");
                if (_item->aborted()) {
                    _needReboot = true;
                    delete _item;
                    continue;
                }

                if (_killed) {
                    _item->kill();
                    _needReboot = true;
                    continue;
                }
                if (_cancelled) {
                    _item->cancel();
                    _needReboot = true;
                    continue;
                }

                if (timedOut) {
                    bothWrite("The program did not complete within 5 "
                        "minutes and has been\nterminated. If you really need "
                        "to run a program for longer,\n"
                        "please send email to " + _adminAddress + ".");
                    _needReboot = true;
                }
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
    String _fromAddress;
    String _lamePath;
    String _lameOptions;
    String _adminAddress;
    String _htdocsPath;
    String _captureFieldPath;
    LinkedList<QueueItem> _queue;
    int _queuedItems;

    volatile bool _processing;
    volatile bool _ending;
    Mutex _mutex;
    Event _ready;
    AutoHandle _arduinoCom;
    AutoHandle _com;
    Array<Byte> _packet;
    QueueItem* _item;
    EmailThread _emailThread;
    bool _killed;
    bool _cancelled;
    bool _needArduinoReset;
    bool _needReboot;
    Array<Byte> _diskBytes;

    String _imager;
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
        configFile.addDefaultOption("serialPort", String("COM1"));
        configFile.addDefaultOption("serialBaudRate", 115200);
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