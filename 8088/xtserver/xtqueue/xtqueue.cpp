#include "alfe/main.h"
#include "alfe/thread.h"
#include "alfe/stack.h"
#include "alfe/linked_list.h"
#include "alfe/email.h"

class XTThread : public Thread
{
public:
    XTThread() : _queuedItems(0), _ending(false)
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
#endif 

        _packet.allocate(0x101);
    }
    ~XTThread() { _ending = true; _ready.signal(); }
    void run(AutoHandle pipe)
    {
        QueueItem* item = new QueueItem(pipe);

        Lock lock(&_mutex);
        _queue.add(item);
        item->notifyQueuePosition(_queuedItems);
        ++_queuedItems;

        _ready.signal();
    }
    void threadProc()
    {
        do {
            QueueItem* item = 0;
            try {
                // TODO: There might be some threading issues here:
                //   1 - _queue is not volatile - will this thread notice the
                //       change from the other thread?
                //   2 - if the other thread signals _ready after we check for
                //       getNext() == 0 and before we call _ready.wait(), will
                //       we stall?
                if (_queue.getNext() == 0) {
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
                    item = _queue.getNext();
                    if (item != 0) {
                        item->remove();
                        --_queuedItems;
                    }
                }
                if (item == 0)
                    continue;
                console.write("Starting work item ");
                item->printInfo();
                console.write("\n");

                // Reset the machine
                _arduinoCom.write<Byte>(0x7f);
                _arduinoCom.write<Byte>(0x77);
                IF_ZERO_THROW(FlushFileBuffers(_arduinoCom));

                int l = item->length();

                Byte checksum;
                bool error = false;

                int p = 0;
                int bytes;
                do {
                    bytes = min(l, 0xff);
                    _packet[0] = bytes;
                    checksum = 0;
                    for (int i = 0; i < bytes; ++i) {
                        Byte d = item->data(p);
                        ++p;
                        _packet[i + 1] = d;
                        checksum += d;
                    }
                    _packet[bytes + 1] = checksum;
                    if (sendPacket())
                        error = true;
                    l -= bytes;
                } while (bytes != 0);

                if (error) {
                    item->write("Warning: The program may not have "
                        "transferred correctly. Please resubmit if it "
                        "does not work as expected.\n");
                    if (item->aborted()) {
                        delete item;
                        continue;
                    }
                }

                item->write("Upload complete.\n");
                if (item->aborted()) {
                    delete item;
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
                    if (timeout > 5*60*1000) {
                        timedOut = true;
                        break;
                    }

                    COMMTIMEOUTS timeOuts;
                    SecureZeroMemory(&timeOuts, sizeof(COMMTIMEOUTS));
                    timeOuts.ReadIntervalTimeout = timeout;
                    timeOuts.ReadTotalTimeoutMultiplier = 0;
                    timeOuts.ReadTotalTimeoutConstant = 0;
                    IF_ZERO_THROW(SetCommTimeouts(_com, &timeOuts));

                    int c = _com.tryReadByte();
                    if (c == -1) {
                        timedOut = true;
                        break;
                    }
                    if (c == 26)
                        break;
                    if (c == '<')
                        item->write("&lt;");
                    else
                        if (c == '&')
                            item->write("&amp;");
                        else
                            item->write(c);
                    if (item->aborted())
                        break;
                } while (true);
                if (item->aborted()) {
                    delete item;
                    continue;
                }

                if (timedOut) {
                    item->write("The program did not complete within 5 minutes"
                        " and has been terminated. If you really need to run a"
                        " program for longer, please send email to "
                        "andrew@reenigne.org.\n");
                }
                else
                    item->write("Program ended normally.\n");
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
            if (item != 0)
                delete item;
            console.write("Work item complete\n");
        } while (true);
    }
private:
    class QueueItem : public LinkedListMember<QueueItem>
    {
    public:
        QueueItem(AutoHandle pipe) : _pipe(pipe), _notified(false),
            _broken(false), _aborted(false)
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
                        (c >= '0' && c <= '9') || c == '!' || c == '!' || 
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
                    FlushFileBuffers(_pipe); 
                    DisconnectNamedPipe(_pipe);

                    if (!_broken || !_emailValid)
                        return;
                    sendMail("XT Server <xtserver@reenigne.org>", 
                        String(reinterpret_cast<const char*>(&_email[0]),
                            _emailLength),
                        "Your XT Server results",
                        "A program was sent to the XT Server at\n"
                        "http://www.reenigne.org/xtserver but the browser was\n"
                        "disconnected before the program completed. The results of this\n"
                        "program are below. If you did not request this information,\n"
                        "please ignore it.\n\n"
                        "Program name: " +
                            String(
                                reinterpret_cast<const char*>(&_fileName[0]),
                                _fileNameLength) + "\n\n" + _log);
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
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    _broken = true;
                    if (!_emailValid)
                        _aborted = true;
                }
            }
        }

        void notifyQueuePosition(int position)
        {
            if (_notified)
                return;
            if (position == 0)
                write("Your program is starting\n");
            else
                if (position == 1)
                    write("Your program is next in the queue.\n");
                else
                    write(String("Your program is at position ") + 
                        String::Decimal(position) + " in the queue.\n");
            _notified = true;
        }

        bool aborted() { return _aborted; }

        int length() { return _dataLength; }

        Byte data(int p) { return _data[p]; }

    private:
        AutoHandle _pipe;

        int _emailLength;
        Array<Byte> _email;
        int _fileNameLength;
        Array<Byte> _fileName;
        int _dataLength;
        Array<Byte> _data;

        String _log;
        bool _broken;
        bool _aborted;
        bool _emailValid;

        bool _notified;
    };
    LinkedList<QueueItem> _queue;
    int _queuedItems;

    bool sendPacket()
    {
        bool error = false;
        bool ok;
        do {
            ok = true;
            _com.write(&_packet[0], 2 + _packet[0]);
            IF_ZERO_THROW(FlushFileBuffers(_com));
            int c;
            do {
                c = _com.tryReadByte();
                switch (c) {
                    case 'K':
                        if (!ok) {
                            console.write("\nXT returned OK but lost data\n");
                            error = true;
                        }
                        else
                            console.write("K");
                        break;
                    case 'F':
                        console.write("F");
                        ok = false;
                        break;
                    case -1:
                        console.write(".");
                        ok = false;
                        _com.write<Byte>(0xa5);
                        break;
                    default:
                        console.write("\nUnrecognized byte " + hex(c, 2) +
                            " from XT\n");
                        error = true;
                        break;
                }
            } while (c == -1);
        } while (!ok);
        return error;
    }
private:

    volatile bool _ending;  // TODO: check that volatile is appropriate here
    AppendableArray<AutoHandle> _pipes;
    AppendableArray<int> _freePipes;
    AppendableArray<int> _connectedPipes;
    Mutex _mutex;
    Event _ready;
    AutoHandle _arduinoCom;
    AutoHandle _com;
    Array<Byte> _packet;

    AutoHandle _pipe;
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