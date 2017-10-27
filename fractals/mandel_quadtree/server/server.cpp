// Fractal computation server.
//
// Because the networking overhead should be small compared to the computation
// time, we only use a single thread for IO. A more network-bound server would
// want to do IO completion processing on multiple threads.

#include "def.h"
#include "../long_fixed.cpp"
#include "socket.h"
#include "linkedlist.h"
#include "vectors.h"
#include <vector>
#include "complex.h"

class Server : public Mutex
{
    class BoxEntry
    {
    public:
        Digit* zx() { return reinterpret_cast<Digit*>(this + 1); }
        Digit* zy(int precision) { return zx() + precision; }
        Digit* zSx(int precision) { return zy(precision) + precision; }
        Digit* zSy(int precision) { return zSx(precision) + precision; }
        static int bytes(int precision)
        {
            return sizeof(BoxEntry) + 4*precision*sizeof(Digit);
        }

        Vector _texel;
        int _iterations;
        // Data follows in "struct hack" fashion:
        //   _precision words for z.x value
        //   _precision words for z.y value
        //   _precision words for zS.x value
        //   _precision words for zS.y value
    };

    class AcceptSocket;
    friend class AcceptSocket;

    class Box : public LinkedListMember<Box>
    {
    public:
        Digit* cx() { return reinterpret_cast<Digit*>(this + 1); }
        Digit* cy() { return cx() + _precision; }
        BoxEntry* entry(int n)
        {
            return &reinterpret_cast<BoxEntry*>(cy() + _precision)[n];
        }
        int bytes()
        {
            return (7 + _precision*2 + _points*(3 + 4*_precision))*4;
        }
        char* buffer() { return reinterpret_cast<char*>(&_points); }
        // Our boxes need to be big enough to hold a single point at
        // maximum precision.
        static int bufferSize() { return (1024*6 + 10)*4; }
        bool valid()
        {
            return _points > 0 && _points <= (bufferSize() - 9*4)/(7*4) &&
                _precision > 0 && _precision <= 1024 &&
                bytes() <= bufferSize();
        }

        Server::AcceptSocket* _socket;

        int _points;
        int _precision;         // ignored in outgoing boxes
        int _pointGeneration;  // Not actually used by server, just passed back
        int _bailoutRadius2;    // ignored in outgoing boxes
        int _logDelta;          // ignored in outgoing boxes
        int _maximumIterations; // client should use value returned by server
        int _logUnitsPerTexel;  // ignored in outgoing boxes
        // Data follows in "struct hack" fashion:
        //   _precision words for c.x value at texel 0, 0 (ignored in outgoing)
        //   _precision words for c.y value at texel 0, 0 (ignored in outgoing)
        //   _points BoxEntry objects
    };

    class AcceptSocket
      : public Socket, public LinkedListMember<AcceptSocket>
    {
        class BoxCompletion : public IOCompletion
        {
        public:
            void setSocket(AcceptSocket* socket) { _socket = socket; }
            virtual void process(DWORD bytesTransferred,
                OVERLAPPED* overlapped, bool success)
            {
                _socket->boxCompleted();
            }
        private:
            AcceptSocket* _socket;
        };

        class AcceptCompletion : public IOCompletion
        {
        public:
            void setSocket(AcceptSocket* socket) { _socket = socket; }
            virtual void process(DWORD bytesTransferred,
                OVERLAPPED* overlapped, bool success)
            {
                _socket->acceptCompleted(bytesTransferred, overlapped,
                    success);
            }
        private:
            AcceptSocket* _socket;
        };

    public:
        AcceptSocket(Server* server)
          : Socket(&server->_addressInformation),
            _addressInformation(&server->_addressInformation),
            _server(server),
            _buffer(2*(_addressInformation->addressLength() + 16))
        {
            _acceptCompletion.setSocket(this);
            _boxCompletion.setSocket(this);
            ZeroMemory(&_acceptOverlapped, sizeof(OVERLAPPED));
            ZeroMemory(&_sendOverlapped, sizeof(OVERLAPPED));
        }

        void accept()
        {
            // Set zero TCP stack receive and send buffer, so that the TCP
            // stack will perform IO directly into/out of our buffers.
            int bufferSize = 0;
            IF_NONZERO_THROW(setsockopt(*this, SOL_SOCKET, SO_SNDBUF,
                reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize)));
            IF_NONZERO_THROW(setsockopt(*this, SOL_SOCKET, SO_RCVBUF,
                reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize)));

            // We give a receive buffer size here of 0 so that we are notified
            // as soon as a connection occurs, in order that we can start
            // another accept immediately. We could get slightly better
            // performance by specifying enough buffer for an initial box as
            // well, but then we would need extra code to guard against a
            // hostile client that connects but then sends no data (this can be
            // done by calling WSAEventSelect() to register for FD_ACCEPT
            // events). The complexity is not worth the insignificant
            // performance improvement for this application.
            BOOL result = _server->_acceptEx(_server->_listenSocket, *this,
                &_buffer[0], 0, _addressInformation->addressLength() + 16,
                _addressInformation->addressLength() + 16, &_bytesReceived,
                &_acceptOverlapped);
            if (result == FALSE)
                if (GetLastError() != ERROR_IO_PENDING)
                    handleError();
        }

        void moveToOutgoing(Box* box)
        {
            _outgoingBoxes.add(box);
            _server->_ioCompletionPort.post(&_boxCompletion);
        }

        void initiateReceive()
        {
            // Each time a box is iterated and there are insufficient boxes
            // queued up, we'll download at most a couple of boxes. If we only
            // downloaded one there would be no chance of getting the queue
            // filled.
            _boxesToReceive += 2;
            extract();
        }

    private:
        bool checkClose()
        {
            if (!_closing)
                return false;
            if (!_sending && !_receiving) {
                open();
                _server->moveToReserve(this);
            }
            return true;
        }

        bool handleError()
        {
            DWORD error = GetLastError();
            if (error == WSAECONNABORTED || error == WSAECONNRESET ||
                error == WSAEDISCON || error == WSAENETDOWN ||
                error == WSAENETRESET || error == WSAETIMEDOUT ||
                error == WSA_OPERATION_ABORTED) {
                _accepted = false;
                _closing = true;
                return checkClose();
            }
            IF_FALSE_THROW(false);
            // TODO: retry on WSAEINPROGRESS, WSAENOBUFS and WSAEWOUDLBLOCK?
            // TODO: tear down socket on WSAENOTCONN?
        }

        void acceptCompleted(DWORD bytesTransferred, OVERLAPPED* overlapped,
            bool success)
        {
            if (overlapped == &_sendOverlapped) {
                _sending = false;
                if (checkClose())
                    return;
                if (!success)
                    handleError();
                else
                    sendNextBox();
                return;
            }

            if (_accepted) {
                // Must have been a receive
                _receiving = false;
                if (checkClose())
                    return;
                _receiveOffset += bytesTransferred;
                if (!success)
                    handleError();
                else
                    extract();
                return;
            }

            if (!success) {
                handleError();
                return;
            }

            // Allow other clients to connect.
            _server->openNewConnection();

            // Inherit socket properties from the listening socket.
            IF_NONZERO_THROW(setsockopt(*this, SOL_SOCKET,
                SO_UPDATE_ACCEPT_CONTEXT,
                reinterpret_cast<char *>(&_server->_listenSocket._handle),
                sizeof(SOCKET)));

            // Associate this socket with the IO completion port.
            _server->_ioCompletionPort.associate(this, &_acceptCompletion);

            // Start things moving
            _boxesToReceive = 0;
            _receiveOffset = 0;
            _incomingBox = _server->getEmptyBox();
            _incomingBox->_socket = this;
            _server->startThreads();
            _sending = false;
            _receiving = false;
            _accepted = true;
        }

        void boxCompleted()
        {
            if (!_sending)
                sendNextBox();
        }

        // Attempt to extract boxes from the buffer until we've extracted
        // enough. If we run out of buffered data, initiate a receive.
        void extract()
        {
            while (_boxesToReceive > 0) {
                if (_receiveOffset >= 8) {
                    // We have enough to determine the expected size of the box
                    if (!_incomingBox->valid()) {
                        // TODO: close connection? Break up box into smaller
                        // ones?
                    }
                    int bytes = _incomingBox->bytes();
                    if (_receiveOffset >= bytes) {
                        // Received the entire box.
                        --_boxesToReceive;
                        Box* box = _incomingBox;
                        _incomingBox = _server->getEmptyBox();
                        _incomingBox->_socket = this;
                        // Move any residual data to the new incoming box.
                        _receiveOffset -= bytes;
                        memcpy(_incomingBox->buffer(), box->buffer(),
                            _receiveOffset);
                        _server->moveToUniterated(box);
                        continue;
                    }
                }
                // Insufficient data - kick off another read
                WSABUF bufferDescriptor;
                bufferDescriptor.len = Box::bufferSize() - _receiveOffset;
                bufferDescriptor.buf = _incomingBox->buffer() + _receiveOffset;
                BOOL result = WSARecv(*this, &bufferDescriptor, 1, NULL, 0,
                    &_acceptOverlapped, 0);
                if (result != 0)
                    if (GetLastError() != WSA_IO_PENDING)
                        handleError();
                // Success case and delayed failure case will be handled by
                // the completion routine.
            }
        }

        void sendNextBox()
        {
            Box* box = _outgoingBoxes.getNext();
            if (box == 0)
                return;
            WSABUF bufferDescriptor;
            bufferDescriptor.len = box->bytes();
            bufferDescriptor.buf = box->buffer();
            int result = WSASend(*this, &bufferDescriptor, 1, NULL, 0,
                &_sendOverlapped, 0);
            if (result != 0)
                if (GetLastError() != WSA_IO_PENDING)
                    handleError();
            _sending = true;
        }

        AddressInformation* _addressInformation;
        Box* _incomingBox;
        LinkedList<Box> _outgoingBoxes;
        Server* _server;
        AcceptCompletion _acceptCompletion;
        BoxCompletion _boxCompletion;
        std::vector<Byte> _buffer;
        OVERLAPPED _acceptOverlapped;
        OVERLAPPED _sendOverlapped;
        DWORD _bytesReceived;
        int _receiveOffset;
        int _boxesToReceive;
        bool _sending;
        bool _receiving;
        bool _accepted;
        bool _closing;
    };

    class CalculationThread : public Thread
    {
    public:
        CalculationThread(Server* server)
          : _server(server),
            _ending(false),
            _failed(false),
            _running(false),
            _box(0)
        {
            _ready.reset();
            _finished.reset();
        }

        // Signals the thread to come to an end.
        void end() { _ending = true; _ready.set(); }

        bool running() const { return _running; }

        // Start the thread.
        void go()
        {
            _finished.reset();
            _ready.set();
        }

    private:
        int doubleIterate()
        {
            Complex<double> z;
            z.x = doubleFromFixed(_zx, 0, _precision);
            z.y = doubleFromFixed(_zy, 0, _precision);
            Complex<double> zS;
            zS.x = doubleFromFixed(_zSx, 0, _precision);
            zS.y = doubleFromFixed(_zSy, 0, _precision);
            Complex<double> c;
            c.x = doubleFromFixed(_t[4], 0, _precision);
            c.y = doubleFromFixed(_t[5], 0, _precision);
            int maximumIterations = _maximumIterations;
            double delta = ldexp(1.0, _logDelta);
            double bailoutRadius2 = ldexp(static_cast<double>(_bailoutRadius2),
                intBits - bitsPerDigit);
            for (int i = 0; i < maximumIterations; i += 2) {
                double zr2 = z.x*z.x;
                double zi2 = z.y*z.y;
                z = Complex<double>(zr2 - zi2 + c.x, 2*z.x*z.y + c.y);
                if (zr2 + zi2 > bailoutRadius2)
                    return i + 1;

                zr2 = z.x*z.x;
                zi2 = z.y*z.y;
                z = Complex<double>(zr2 - zi2 + c.x, 2*z.x*z.y + c.y);
                if (zr2 + zi2 > bailoutRadius2)
                    return i + 2;

                zr2 = zS.x*zS.x;
                zi2 = zS.y*zS.y;
                zS = Complex<double>(zr2 - zi2 + c.x, 2*zS.x*zS.y + c.y);
                Complex<double> d = z - zS;
                if (abs(d.x) < delta && abs(d.y) < delta)
                    return -(i + 2);
            }
            fixedFromDouble(_zx, z.x, 0, _precision);
            fixedFromDouble(_zy, z.y, 0, _precision);
            fixedFromDouble(_zSx, zS.x, 0, _precision);
            fixedFromDouble(_zSy, zS.y, 0, _precision);
            return -1;
        }

        int longFixedIterate()
        {
            int p = _precision;
            fixedFromDouble(_t[3], 1.0, _logDelta, p);
            int maximumIterations = _maximumIterations;
            for (int i = 0; i < maximumIterations; i += 2) {
                multiply(_t[0], _zx, _zx, _t[6], p);
                multiply(_t[1], _zy, _zy, _t[6], p);
                add(_t[2], _t[0], _t[1], p);
                if (static_cast<SignedDigit>(_t[2][p - 1]) > _bailoutRadius2)
                    return i + 1;
                multiply(_t[2], _zx, _zy, _t[6], p, intBits + 1);
                add(_zx, _t[0], _t[4], p);
                sub(_zx, _zx, _t[1], p);
                add(_zy, _t[2], _t[5], p);

                multiply(_t[0], _zx, _zx, _t[6], p);
                multiply(_t[1], _zy, _zy, _t[6], p);
                add(_t[2], _t[0], _t[1], p);
                if (static_cast<SignedDigit>(_t[2][p - 1]) > _bailoutRadius2)
                    return i + 2;
                multiply(_t[2], _zx, _zy, _t[6], p, intBits + 1);
                add(_zx, _t[0], _t[4], p);
                sub(_zx, _zx, _t[1], p);
                add(_zy, _t[2], _t[5], p);

                multiply(_t[0], _zSx, _zSx, _t[6], p);
                multiply(_t[1], _zSy, _zSy, _t[6], p);
                multiply(_t[2], _zSx, _zSy, _t[6], p, intBits + 1);
                add(_zSx, _t[0], _t[4], p);
                sub(_zSx, _zSx, _t[1], p);
                add(_zSy, _t[2], _t[5], p);
                sub(_t[0], _zSx, _zx, p);
                abs(_t[0], _t[0], p);
                if (lessThan(_t[0], _t[3], p)) {
                    sub(_t[0], _zSy, _zy, p);
                    abs(_t[0], _t[0], p);
                    if (lessThan(_t[0], _t[3], p))
                        return -(i + 2);
                }
            }
            return -1;
        }

        bool processOneBox()
        {
            {
                Lock lock(_server);
                if (_box != 0)
                    _box->_socket->moveToOutgoing(_box);
                _box = _server->getUniteratedBox();
                if (_box == 0)
                    return false;
            }
            _precision = _box->_precision;
            // Server may also use its own preferred value for
            // _maximumIterations, as long as it returns in the box the same
            // value that it uses.
            _maximumIterations = _box->_maximumIterations;
            _logDelta = _box->_logDelta;
            _bailoutRadius2 = _box->_bailoutRadius2;
            // Setup up temporary buffers
            _buffer.ensureLength(10*_precision);
            Digit* t = _buffer;
            for (int i = 0; i < 7; ++i) {
                _t[i] = t;
                t += _precision;
            }

            for (int i = 0; i < _box->_points; ++i) {
                BoxEntry* entry = _box->entry(i);
                fixedFromDouble(_t[4], entry->_point.x,
                    _box->_logUnitsPerPoint, _precision);
                add(_t[4], _t[4], _box->cx(), _precision);
                fixedFromDouble(_t[5], entry->_point.y,
                    _box->_logUnitsPerPoint, _precision);
                add(_t[5], _t[5], _box->cy(), _precision);
                _zx = entry->zx();
                _zy = entry->zy(_precision);
                _zSx = entry->zSx(_precision);
                _zSy = entry->zSy(_precision);
                if (_precision <= 2)
                    entry->_iterations = doubleIterate();
                else
                    entry->_iterations = longFixedIterate();
            }
            return true;
        }

        void threadProc()
        {
            BEGIN_CHECKED {
                do {
                    _ready.wait();
                    _ready.reset();
                    while (processOneBox())
                        ;
                    _finished.set();
                } while (!_ending);
            } END_CHECKED(Exception& e) {
                _exception = e;
                _failed = true;
                _finished.set();
            }
        }

        // Wait for a thread to finish if it hasn't already and rethrow the
        // exception on the main thread if it failed.
        void check()
        {
            _finished.wait();
            _finished.reset();
            if (_failed)
                throw _exception;
        }

        Exception _exception;
        Box* _box;
        int _precision;
        int _maximumIterations;
        int _logDelta;
        SignedDigit _bailoutRadius2;
        volatile bool _failed;
        volatile bool _ending;
        volatile bool _running;
        Event _ready;
        Event _finished;
        Server* _server;
        DigitBuffer _buffer;
        Digit* _t[7];
        Digit* _zx;
        Digit* _zy;
        Digit* _zSx;
        Digit* _zSy;
    };

    void openNewConnection()
    {
        AcceptSocket* socket = getSocket();
        socket->accept();
    }

public:
    Server(const char* port)
      : _addressInformation(port),
        _listenSocket(&_addressInformation),
        _uniteratedCount(0),
        _threadCount(0)
    {
        _listenSocket.bind();
        _listenSocket.listen();
        LPFN_ACCEPTEX acceptEx = 0;
        DWORD bytes;
        GUID guidAcceptEx = WSAID_ACCEPTEX;
        IF_NONZERO_THROW(WSAIoctl(_listenSocket,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guidAcceptEx,
            sizeof(guidAcceptEx),
            &_acceptEx,
            sizeof(_acceptEx),
            &bytes,
            NULL,
            NULL));

        openNewConnection();

        // Count available threads
        DWORD_PTR pam, sam;
        IF_ZERO_THROW(GetProcessAffinityMask(GetCurrentProcess(), &pam, &sam));
        for (DWORD_PTR p = 1; p != 0; p <<= 1)
            if ((pam&p) != 0)
                ++_threadCount;
        _threads.resize(_threadCount, 0);
        for (int i = 0; i < _threadCount; ++i) {
            CalculationThread* thread = new CalculationThread(this);
            thread->setPriority(THREAD_PRIORITY_BELOW_NORMAL);
            _threads[i] = thread;
            thread->start();
        }
    }

    ~Server()
    {
        // Tell the threads to stop what they're doing.
        for (int i = 0; i < _threadCount; ++i) {
            CalculationThread* thread = _threads[i];
            if (thread != 0)
                thread->end();
        }
        // Wait for them all to actually stop and delete them. Don't rethrow
        // any exceptions here.
        for (int i = 0; i < _threadCount; ++i) {
            CalculationThread* thread = _threads[i];
            if (thread != 0) {
                thread->join();
                delete thread;
            }
        }

        // Delete all the boxes.
        // Can't use a range-based for loop here because we're
        // removing items and continuing.
        auto box = _emptyBoxes.next();
        while (box != &_emptyBoxes) {
            auto next = box->next();
            box->remove();
            free(box);
            box = next;
        }
    }

    void loop()
    {
        while (true)
            _ioCompletionPort.process();
    }

    AcceptSocket* getSocket()
    {
        AcceptSocket* socket = _reserveSockets.next();
        if (socket == 0)
            socket = new AcceptSocket(this);
        else
            socket->remove();
        _activeSockets.add(socket);
        return socket;
    }

    void moveToReserve(AcceptSocket* socket)
    {
        socket->remove();
        _reserveSockets.add(socket);
    }

    Box* getEmptyBox()
    {
        Box* box = _emptyBoxes.getNext();
        if (box == 0)
            box = static_cast<Box*>(malloc(Box::bufferSize()));
        else
            box->remove();
        return box;
    }

    Box* getUniteratedBox()
    {
        Box* box = _uniteratedBoxes.getNext();
        if (box != 0)
            box->remove();
        --_uniteratedCount;
        // Use a higher multiple of _threadCount below if we end up with idle
        // threads.
        if (_uniteratedCount < _threadCount) {
            // Give each client a chance to give us another box.
            for (auto& socket : _activeSockets)
                socket.initiateReceive();
        }
        return box;
    }

    void moveToEmpty(Box* box) { _emptyBoxes.add(box); }

    void startThreads()
    {
        for (int i = 0; i < _threadCount; ++i) {
            CalculationThread* thread = _threads[i];
            if (!thread->running())
                thread->go();
        }
    }

    void moveToUniterated(Box* box)
    {
        _uniteratedBoxes.add(box);
        ++_uniteratedCount;
        // Now we have some work to do - make sure we have some threads to do
        // it.
        startThreads();
    }
private:
    WindowsSockets _windowsSockets;
    IOCompletionPort _ioCompletionPort;
    AddressInformation _addressInformation;
    Socket _listenSocket;
    int _threadCount;
    OwningLinkedList<AcceptSocket> _reserveSockets;
    LinkedList<AcceptSocket> _activeSockets;
    LinkedList<Box> _emptyBoxes;
    LinkedList<Box> _uniteratedBoxes;
    int _uniteratedCount;
    LPFN_ACCEPTEX _acceptEx;
    std::vector<CalculationThread*> _threads;
};

INT APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, INT)
{
    const char* port = "24448";
    if (lpCmdLine[0] != 0)
        port = lpCmdLine;
    do {
        BEGIN_CHECKED {
            Server server(port);
            server.loop();
        } END_CHECKED(Exception&) {
            // Wait a couple of seconds before restarting so that a persistent
            // failure doesn't cause us to peg the CPU.
            Sleep(2000);
            // Ignore exception and restart server.
        }
    } while (true);
}
