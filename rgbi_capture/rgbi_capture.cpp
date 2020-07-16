#include "alfe/main.h"
#include "alfe/avi.h"
#include "alfe/config_file.h"
#include "alfe/colour_space.h"
#include "alfe/thread.h"
#include "../../external/CyAPI/CyAPI.h"

static const SRGB palette[0x100] = {
	// bit 6 == 0 => unused
	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// speaker low, no hsync, no vsync
	SRGB(0x1c, 0x1c, 0x1c), SRGB(0x1c, 0x1c, 0xc6),
	SRGB(0x1c, 0xc5, 0x1c), SRGB(0x1c, 0xc6, 0xc6),
	SRGB(0xc6, 0x1c, 0x1c), SRGB(0xc6, 0x1c, 0xc6),
	SRGB(0xc6, 0x71, 0x1c), SRGB(0xc6, 0xc6, 0xc6),
	SRGB(0x39, 0x39, 0x39), SRGB(0x39, 0x39, 0xe3),
	SRGB(0x39, 0xe3, 0x39), SRGB(0x39, 0xe3, 0xe3),
	SRGB(0xe3, 0x39, 0x39), SRGB(0xe3, 0x39, 0xe3),
	SRGB(0xe3, 0xe3, 0x39), SRGB(0xe3, 0xe3, 0xe3),

	// speaker low, hsync, no vsync
	SRGB(0x1c, 0x71, 0x1c), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// speaker low, no hsync, vsync
	SRGB(0x71, 0x1c, 0x1c), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// speaker low, hsync, vsync
	SRGB(0x71, 0x71, 0x1c), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// bit 6 == 0 => unused
	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// speaker high, no hsync, no vsync
	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// speaker high, hsync, no vsync
	SRGB(0x00, 0x55, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// speaker high, no hsync, vsync
	SRGB(0x55, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff),

	// speaker high, hsync, vsync
	SRGB(0x55, 0x55, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff)
};

template<class T> class CompressThreadT : public ThreadTask
{
public:
	CompressThreadT() : _program(0) { }
	void setProgram(Program* program) { _program = program; /*restart();*/ }
private:
	void run()
	{
		do {
			bool gotData = _program->compressBlock();
			if (!gotData)
				return;
		} while (!cancelling());
	}

	Program* _program;
};

typedef CompressThreadT<void> CompressThread;

class Program : public ProgramBase
{
public:
	void run()
	{
		ConfigFile configFile;
		configFile.addDefaultOption("bufferSize", 262144);
		configFile.addDefaultOption("bufferCount", 16);

		String configName = "default.config";
		if (_arguments.count() >= 2)
			configName = _arguments[1];
		File configPath(configName, true);
		configFile.load(configPath);

		_compressThread.setProgram(this);

		CCyUSBDevice cyUSBDevice;
		int c = cyUSBDevice.DeviceCount();
		int vID, pID;
		int d = 0;
		for (d = 0; d < c; ++d) {
			cyUSBDevice.Open(d); // Open automatically calls Close() if necessary
			vID = cyUSBDevice.VendorID;
			pID = cyUSBDevice.ProductID;
			printf("VID: %04x PID: %04x\n", vID, pID);
			if (vID == 0x04b4 && pID == 0x8613)
				break;
		}
		if (d == c)
			throw Exception("FX2LP device not cconnected");

		int endPointCount = cyUSBDevice.EndPointCount();
		for (int endPoint = 0; endPoint < endPointCount; ++endPoint) {
			CCyUSBEndPoint* e = cyUSBDevice.EndPoints[endPoint];
			printf("Attributes: %i bIn: %i \n", e->Attributes, e->bIn);
		}
		CCyBulkEndPoint* cyBulkEndPoint = cyUSBDevice.BulkInEndPt;
		//CCyInterruptEndPoint* cyBulkEndPoint = cyUSBDevice.InterruptInEndPt;
		//CCyIsocEndPoint* cyBulkEndPoint = cyUSBDevice.IsocInEndPt;

		//cyBulkEndPoint->SetXferSize(65536);

		USHORT maxPacketSize = cyBulkEndPoint->MaxPktSize;
		//printf("maxPacketSize = %i\n", maxPacketSize);
		int bufferSize = configFile.get<int>("bufferSize"); // /*8 **/ 256 * 1024;
		int bufferCount = configFile.get<int>("bufferCount"); // 4;
		_writeOffset = 0;
		_readOffset = 0;
		_rawBytes = bufferSize * bufferCount;
		_rawBytesAvailable = 0;
		_rawData.allocate(_rawBytes);
		_maxPacketSize = cyBulkEndPoint->MaxPktSize;

		//int pkts;
		// Allocate the IsoPktInfo objects, and find-out how many were allocated
		//_isoPktInfos = cyBulkEndPoint->CreatePktInfos(_rawBytes, pkts);
		//printf("pkts = %i\n", pkts);



		AVIFileInitializer aviFileInitializer;
		AVIWriter aviWriter(File("rgbi.avi", true),
			Vector2Cast<int>(912, 262), Rational(1640625, 27379), NULL);
		_avi = &aviWriter;

		_x = 0;
		_y = 0;
		_bits = aviWriter.bits();
		_bytesPerLine = aviWriter.stride();
		_line = static_cast<Byte*>(_bits) + 261 * _bytesPerLine;
		_p = _line;

		AllocConsole();
		HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
		IF_ZERO_THROW(SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS));
		IF_ZERO_THROW(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL));
		//IF_ZERO_THROW(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL));

		bool done = false;

		int totalLength = 0;

		Array<OVERLAPPED> ovLapStatus;
		Array<PUCHAR> context;
		ovLapStatus.allocate(bufferCount);
		context.allocate(bufferCount);
		for (int i = 0; i < bufferCount; ++i) {
			memset(&ovLapStatus[i], 0, sizeof(OVERLAPPED));
			ovLapStatus[i].hEvent = CreateEvent(NULL, false, false, NULL);
			context[i] = cyBulkEndPoint->BeginDataXfer(&_rawData[i * bufferSize], bufferSize, &ovLapStatus[i]);
		}
		int bufNum = 0;
		int ioN = 0;

		do {
			//printf("IO %i: ", ioN); ++ioN;
			//printf("WaitForIO(0x%08x)\n", &ovLapStatus[bufNum]);
			bool   wResult = cyBulkEndPoint->WaitForIO(&ovLapStatus[bufNum]);
			//bool wResult = cyBulkEndPoint->WaitForXfer(&ovLapStatus[bufNum], 1000);
			LONG length = bufferSize;
			//printf("FinishDataXfer(0x%08x, %i, 0x%08x, 0x%08x, 0x%08x)\n", &_rawData[_writeOffset], length, &ovLapStatus[bufNum], context[bufNum], _isoPktInfos + _writeOffset/_maxPacketSize);
			bool   fResult = cyBulkEndPoint->FinishDataXfer(&_rawData[_writeOffset], length, &ovLapStatus[bufNum], context[bufNum], _isoPktInfos + _writeOffset / _maxPacketSize);

			if (!wResult || !fResult) {
				//printf("%i %i", wResult, fResult);


			//printf("_writeOffset = %i\n", _writeOffset);
			//if (!cyBulkEndPoint->XferData(&_rawData[_writeOffset], length, _isoPktInfos + _writeOffset/_maxPacketSize)) {
				printf("Error reading from FX2LP: 0x%08x\n", /*cyBulkEndPoint->NtStatus*/ GetLastError());

				return;
			}
			//printf("Written\n");
			int realLength = length; // _maxPacketSize* pkts / bufferCount;
			if (length != bufferSize)
				printf("Length was %i, expected %i\n", length, bufferSize);

			//for (int p = 0; p < pkts; ++p)
			//	realLength += _isoPktInfos[p + _writeOffset/_maxPacketSize].Length;
			//printf("Length: %i\n", length);
			//for (int i = 0; i < pkts; ++i)
			//	printf("pkt[%i] status = %i  length = %i\n", i, _isoPktInfos[i].Status, _isoPktInfos[i].Length);
			{
				Lock lock(&_mutex);
				_writeOffset = (_writeOffset + realLength) & (_rawBytes - 1);
				_rawBytesAvailable += realLength;
				_compressThread.restart();
			}

			do {
				DWORD nEvents;
				IF_ZERO_THROW(GetNumberOfConsoleInputEvents(hInput, &nEvents));
				if (nEvents == 0)
					break;
				INPUT_RECORD record;
				DWORD nEventsRead;
				IF_ZERO_THROW(ReadConsoleInput(hInput, &record, 1, &nEventsRead));
				if (nEventsRead > 0 && record.EventType == KEY_EVENT && record.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && record.Event.KeyEvent.bKeyDown)
					done = true;
			} while (true);

			//printf("BeginDataXfer(0x%08x, %i, 0x%08x)\n", &_rawData[(_writeOffset - realLength) & (_rawBytes - 1)], bufferSize, &ovLapStatus[bufNum]);
			context[bufNum] = cyBulkEndPoint->BeginDataXfer(&_rawData[(_writeOffset - realLength) & (_rawBytes - 1)], bufferSize, &ovLapStatus[bufNum]);
			bufNum = (bufNum + 1) % bufferCount;

		} while (!done);

		cyBulkEndPoint->Abort();

		for (int i = 0; i < bufferCount; ++i) {

			if (!cyBulkEndPoint->WaitForXfer(&ovLapStatus[i], 1000)) {
				cyBulkEndPoint->Abort();
				WaitForSingleObject(ovLapStatus[i].hEvent, INFINITE);
			}
			LONG rLen = bufferSize;
			cyBulkEndPoint->FinishDataXfer(&_rawData[0], rLen, &ovLapStatus[i], context[i]);

			CloseHandle(ovLapStatus[i].hEvent);
		}

		//{
		//	auto s = File("rgbi.dat").openWrite();
		//	int o = (_writeOffset - _maxPacketSize*pkts/bufferCount) & (_rawBytes - 1);
		//	for (int i = 0; i < pkts/bufferCount; ++i)
		//		s.write(&_rawData[o + i*_maxPacketSize], _isoPktInfos[o/_maxPacketSize + i].Length);
		//}

		IF_ZERO_THROW(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL));

		_compressThread.cancel();
		_compressThread.join();


		//auto s = File("output.dat").openWrite();
		//int n = 0;
		//do {
		//	LONG length = bytesPerTransfer;
		//	if (!cyBulkEndPoint->XferData(&buffer[0], length)) {
		//		printf("Status: 0x%08x\n", cyBulkEndPoint->NtStatus);
		//	}
		//	else {
		//		s.write(static_cast<const void*>(&buffer[0]), length);
		//		n += length;
		//		while (n > 14000000) {
		//			n -= 14000000;
		//			printf(".");
		//		}
		//	}
		//} while (true);

	}
	bool compressBlock()
	{
		Byte* buffer;
		CCyIsoPktInfo* pktInfos;
		int bytesAvailable;
		//printf("_readOffset = %i\n", _readOffset);
		{
			Lock lock(&_mutex);
			buffer = &_rawData[_readOffset];
			//pktInfos = &_isoPktInfos[_readOffset/_maxPacketSize];
			bytesAvailable = min(_rawBytes - _readOffset, _rawBytesAvailable);
			if (bytesAvailable == 0)
				return false;
			_rawBytesAvailable -= bytesAvailable;
			_readOffset = (_readOffset + bytesAvailable) & (_rawBytes - 1);
		}

		while (bytesAvailable > 0) {
			int bytes = _maxPacketSize; // pktInfos->Length;
			for (int i = 0; i < bytes; ++i) {
				Byte b = buffer[i];
				_p[0] = palette[b].z;
				_p[1] = palette[b].y;
				_p[2] = palette[b].x;
				_p += 4; // 3;
				++_x;
				if (_x == 912) {
					_x = 0;
					++_y;
					_line -= _bytesPerLine;
					if (_y == 262) {
						_y = 0;
						_line = static_cast<Byte*>(_bits) + 261 * _bytesPerLine;
						_avi->AddAviFrame();
						//printf("Compressed\n");
					}
					_p = _line;
				}
			}
			bytesAvailable -= _maxPacketSize;
			buffer += _maxPacketSize;
			//++pktInfos;
		}

		//Byte* line = static_cast<Byte*>(_bits);
		//for (int y = 0; y < 262; ++y) {
		//	Byte* p = line;
		//	for (int x = 0; x < 912; ++x) {
		//		p[0] = buffer[y*912 + x];
		//		p[1] = buffer[y*912 + x];
		//		p[2] = buffer[y*912 + x];
		//		p += 3;
		//	}
		//	line += _bytesPerLine;
		//}
		//AddAviFrame(_avi, _hbm);
		return true;
	}
private:
	Array<Byte> _rawData;
	int _writeOffset;
	int _readOffset;
	int _rawBytes;
	int _rawBytesAvailable;
	Mutex _mutex;
	void* _bits;
	int _bytesPerLine;
	AVIWriter* _avi;
	Byte* _line;
	Byte* _p;
	int _x;
	int _y;
	CCyIsoPktInfo* _isoPktInfos;
	int _maxPacketSize;

	CompressThread _compressThread;
};

