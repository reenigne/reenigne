#include "alfe/main.h"
#include "alfe/thread.h"
#include "alfe/stack.h"
#include "alfe/linked_list.h"
#include "alfe/email.h"

#include <WinCrypt.h>

class QueueItem : public LinkedListMember<QueueItem>
{
public:
    QueueItem(AutoHandle pipe) : _pipe(pipe),
        _broken(false), _aborted(false), _lastNotifiedPosition(-1)
    {
        _emailLength = pipe.read<int>();
        Array<Byte> email(_emailLength);
        pipe.read(&email[0], _emailLength);
        email.swap(_email);
        _emailValid = true;
        if (_emailLength < 4)
            _emailValid = false;
        else {
            int i;
            for (i = 0; i < _emailLength; ++i) {
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
            if (i != _emailLength)
                _emailValid = false;
        }

        _fileNameLength = pipe.read<int>();
        Array<Byte> fileName(_fileNameLength);
        pipe.read(&fileName[0], _fileNameLength);
        fileName.swap(_fileName);

        _dataLength = pipe.read<int>();
        Array<Byte> data(_dataLength);
        pipe.read(&data[0], _dataLength);
        data.swap(_data);

        _serverPId = pipe.read<DWORD>();

        _logFileLength = pipe.read<int>();
        Array<Byte> logFile(_logFileLength);
        pipe.read(&logFile[0], _logFileLength);
        logFile.swap(_logFile);

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
                _secret[i] = logFile[(_logFileLength - 1) - i];
        }
    }
    void printInfo()
    {
        console.write(&_fileName[0], _fileNameLength);
        console.write(" for ");
        console.write(&_email[0], _emailLength);
    }

    ~QueueItem()
    {
        try {
            try {
                if (_command == 0)
                    write("</pre>\n");

                FlushFileBuffers(_pipe); 
                DisconnectNamedPipe(_pipe);

                if (!_broken || !_emailValid)
                    return;

                if (WaitForSingleObject(_serverProcess, 0) == WAIT_TIMEOUT) {
                    // Server process is still running - we don't need to send
                    // email.
                    return;
                }

                console.write("Sending email");

                sendMail("XT Server <xtserver@reenigne.org>", 
                    String(reinterpret_cast<const char*>(&_email[0]),
                        _emailLength),
                    "Your XT Server results",
                    "A program was sent to the XT Server at\n"
                    "http://www.reenigne.org/xtserver but the browser was\n"
                    "disconnected before the program completed. The results of this\n"
                    "program are below. If you did not request this information,\n"
                    "please ignore it.\n\n"
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

    int length() { return _dataLength; }

    int command() { return _command; }

    Byte data(int p) { return _data[p]; }

    String secret()
    {
        return String(reinterpret_cast<const char*>(&_secret[0]), 16);
    }

    String fileName()
    {
        return String(
            reinterpret_cast<const char*>(&_fileName[0]),
            _fileNameLength);
    }

    void kill()
    {
        write("Your program was terminated by an administrator.\n");
        delete this;
    }

    void cancel()
    {
        write("Your program was cancelled.\n");
        delete this;
    }

    void setFinishTime(DWORD finishTime) { _finishTime = finishTime; }
    DWORD getFinishTime() { return _finishTime; }

    bool serverRunning()
    {
    }
private:
    AutoHandle _pipe;

    int _emailLength;
    Array<Byte> _email;
    int _fileNameLength;
    Array<Byte> _fileName;
    int _dataLength;
    Array<Byte> _data;
    int _logFileLength;
    Array<Byte> _logFile;

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
    void add(QueueItem* item)
    {
        item->setFinishTime(GetTickCount());
        Lock lock(&_mutex);
        _queue.add(item);
        _ready.signal();
    }
    void threadProc()
    {
        do {
            QueueItem* item;
            {
                Lock lock(&_mutex);
                item = _queue.getNext();
                if (item != 0)
                    item->remove();
            }
            if (item == 0) {
                // We have nothing to do - stop the the thread until we do.
                _ready.wait();
                if (_ending)
                    break;
                continue;
            }
            DWORD sleepTime = 5000 - (GetTickCount() - item->getFinishTime());
            if (sleepTime <= 5000)
                Sleep(sleepTime);
            delete item;
        } while (true);
    }
private:
    LinkedList<QueueItem> _queue;
    Mutex _mutex;
    Event _ready;
    bool _ending;
};

class XTThread : public Thread
{
public:
    XTThread() : _queuedItems(0), _ending(false), _processing(false)
    {
        // Open handle to Arduino for rebooting machine
#if 1
        _arduinoCom = AutoHandle(CreateFile(
            L"COM3",
            GENERIC_READ | GENERIC_WRITE,
            0,              // must be opened with exclusive-access
            NULL,           // default security attributes
            OPEN_EXISTING,  // must use OPEN_EXISTING
            0,              // not overlapped I/O
            NULL),          // hTemplate must be NULL for comm devices
            String("Arduino COM port"));

        DCB deviceControlBlock;
        SecureZeroMemory(&deviceControlBlock, sizeof(DCB));
        IF_ZERO_THROW(GetCommState(_arduinoCom, &deviceControlBlock));
        deviceControlBlock.DCBlength = sizeof(DCB);
        deviceControlBlock.BaudRate = 19200;
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
        _com = AutoHandle(CreateFile(
            L"COM1",
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
        deviceControlBlock.BaudRate = 115200; //57600; //115200; //38400; //
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

        _packet.allocate(0x101);

        _emailThread.start();
    }
    ~XTThread() { _ending = true; _ready.signal(); }
    void run(AutoHandle pipe)
    {
        QueueItem* item = new QueueItem(pipe);

        switch (item->command()) {
            case 0:
                {
                    // Run a program
                    Lock lock(&_mutex);
                    _queue.add(item);
                    item->write("<form action='http://reenigne.dyndns.org/cgi-bin/xtcancel.exe' method='post'>\n"
                        "<input type='hidden'>" + item->secret() + "</input>\n"
                        "<button type='submit'>Cancel</button>\n"
                        "</form>\n<pre>");
                    item->notifyQueuePosition(_queuedItems + (_processing ? 1 : 0));
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
                    item->write(String("Online, with ") +
                        (_queuedItems + (_processing ? 1 : 0)) +
                        " items in the queue");
                    delete item;
                }
                break;
            case 3:
                {
                    // Cancel a job
                    Lock lock(&_mutex);
                    if (item->secret() == _item->secret())
                        _cancelled = true;
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
        console.write("Resetting\n");
        // Reset the machine
        _arduinoCom.write<Byte>(0x7f);
        _arduinoCom.write<Byte>(0x77);
        IF_ZERO_THROW(FlushFileBuffers(_arduinoCom));
    }
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
                console.write("Starting work item ");
                _item->printInfo();
                console.write("\n");
                _processing = true;

                int retry = 1;
                bool error;
                do {
                    IF_ZERO_THROW(PurgeComm(_com,
                        PURGE_RXCLEAR | PURGE_TXCLEAR));

                    error = false;
                    reboot();

                    console.write("Transferring attempt " +
                        String::Decimal(retry) + "\n");

                    int l = _item->length();

                    Byte checksum;

                    int p = 0;
                    int bytes;
                    do {
                        bytes = min(l, 0xff);
                        _packet[0] = bytes;
                        checksum = 0;
                        for (int i = 0; i < bytes; ++i) {
                            Byte d = _item->data(p);
                            ++p;
                            _packet[i + 1] = d;
                            checksum += d;
                        }
                        _packet[bytes + 1] = checksum;
                        _com.write(&_packet[0], 2 + _packet[0]);
                        IF_ZERO_THROW(FlushFileBuffers(_com));
                        if (_com.tryReadByte() != 'K')
                            error = true;
                        l -= bytes;
                        if (_killed || _cancelled)
                            break;
                    } while (bytes != 0);

                    if (error)
                        ++retry;
                    else
                        break;
                } while (retry < 10);
                if (error) {
                    console.write("Failed to upload!\n");
                    _item->write("Could not transfer the program to the XT. "
                        "The machine may be offline for maintainance - please "
                        "try again later.\n");
                    delete _item;
                    continue;
                }

                _item->write("Upload complete.\n");
                if (_item->aborted()) {
                    delete _item;
                    continue;
                }
                console.write("\nUpload complete.\n");
                // Dump bytes from COM port to pipe until we receive ^Z or we
                // time out.

                DWORD startTime = GetTickCount();
                bool timedOut = false;
                do {
                    DWORD elapsed = GetTickCount() - startTime;
                    DWORD timeout = 5*60*1000 - elapsed;
                    if (timeout == 0 || timeout > 5*60*1000) {
                        timedOut = true;
                        break;
                    }

                    int c = _com.tryReadByte();
                    if (c == -1)
                        continue;
                    if (c == 26)
                        break;
                    if (c == '<')
                        _item->write("&lt;");
                    else
                        if (c == '&')
                            _item->write("&amp;");
                        else
                            _item->write(c);
                    if ((c < 32 || c > 126) && (c != 9 && c != 10 && c != 13))
                        console.write<Byte>('.');
                    else
                        console.write<Byte>(c);
                    if (_item->aborted())
                        break;
                    if (_killed || _cancelled)
                        break;
                } while (true);
                console.write("\n");
                _item->write("\n");
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

                if (timedOut) {
                    _item->write("The program did not complete within 5 minutes and has been\n"
                        "terminated. If you really need to run a program for longer,\n"
                        "please send email to andrew@reenigne.org.");
                }
                else
                    _item->write("Program ended normally.");

                if (_item->needSleep())
                    _emailThread.add(_item);
            }
            catch (const Exception& e)
            {
                console.write(e);
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
    LinkedList<QueueItem> _queue;
    int _queuedItems;

    volatile bool _processing;
    volatile bool _ending;  // TODO: check that volatile is appropriate here
    Mutex _mutex;
    Event _ready;
    AutoHandle _arduinoCom;
    AutoHandle _com;
    Array<Byte> _packet;
    QueueItem* _item;
    EmailThread _emailThread;
    bool _killed;
    bool _cancelled;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        XTThread xtThread;
        xtThread.start();
        while (true)
        {
            console.write("Waiting for connection\n");
            AutoHandle h = File("\\\\.\\pipe\\xtserver", true).createPipe();

            bool connected = (ConnectNamedPipe(h, NULL) != 0) ? true : 
                (GetLastError() == ERROR_PIPE_CONNECTED); 

            if (connected) {
                console.write("Connected\n");
                xtThread.run(h);
            }
        }
    }
};