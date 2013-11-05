#include <stdlib.h>
#include <stdio.h>
#include <dos.h>

typedef unsigned long int  UInt32;
typedef signed long int    SInt32;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef int                Bool;

static const Bool true = 1;
static const Bool false = 0;

extern "C" {

UInt16 playWithTimer(UInt16 beginPara, UInt16 endPara, UInt16 timerCount,
    void (*timerVector)(), UInt16 catchUpCount);
void interrupt8test();
void interrupt8play();
UInt16 maxNops();
UInt16 playCalibrated(UInt16 beginPara, UInt16 endPara, UInt16 nopCount,
    UInt16 catchUpCount);
UInt16 muld(UInt16 a, UInt16 b, UInt16 c);
UInt16 mulh(UInt16 a, UInt16 b);
UInt16 openHandle(const char* filename);
void closeHandle(UInt16 handle);
UInt16 readHandle(UInt16 handle, UInt16 count, UInt16 offset, UInt16 segment);
UInt16 osError();
UInt16 dosAlloc(UInt16 paras);
UInt16 dosFree(UInt16 para);
UInt16 totalMemory();

}

struct WaveHeader
{
    UInt32 _rTag;
    UInt32 _rLen;
    UInt32 _wTag;
    UInt32 _fTag;
    UInt32 _fLen;
    UInt16 _wFormatTag;
    UInt16 _nChannels;
    UInt32 _nSamplesPerSec;
    UInt32 _nAvgBytesPerSec;
    UInt16 _nBlockAlign;
    UInt16 _wBitsPerSample;
    UInt32 _dTag;
    UInt32 _dLen;

    Bool verify()
    {
        if (!verifyTag("RIFF", _rTag))
            return false;
        if (!verifyTag("WAVE", _wTag))
            return false;
        if (!verifyTag("fmt ", _fTag))
            return false;
        if (_fLen != 16)
            return false;
        if (_wFormatTag != 1)
            return false;
        if (_nChannels < 1 || _nChannels > 2)
            return false;
        if (_nAvgBytesPerSec != _nSamplesPerSec * _nBlockAlign)
            return false;
        if (!verifyTag("data", _dTag))
            return false;
        if (_rLen != _dLen + 36)
            return false;
        if (_nBlockAlign * 8 != _wBitsPerSample * _nChannels)
            return false;
        if (_wBitsPerSample != 8 && _wBitsPerSample != 16)
            return false;
        if (_dLen % _nBlockAlign != 0)
            return false;
        return true;
    }

private:
    static Bool verifyTag(const char* expected, UInt32 tag)
    {
        return (*(UInt32*)(expected) != tag);
    }
};

class File
{
public:
    File() : _filename(0), _opened(false) { }
    ~File()
    {
        if (_opened)
            closeHandle(_handle);
    }
    void init(const char* filename) { _filename = filename; }
    void open()
    {
        _handle = openHandle(_filename);
        UInt16 e = osError();
        if (e != 0)
            error("opening", e);
        _opened = true;
    }
    void read(UInt16 size, UInt16 offset, UInt16 segment)
    {
        UInt16 bytesRead = readHandle(_handle, size, offset, segment);
        if (bytesRead != size)
            printf("End of file reading %s\n", _filename);
        UInt16 e = osError();
        if (e != 0)
            error("reading", e);
    }
private:
    void error(const char* action, UInt16 errorCode)
    {
        printf("Error %i %s file %s\n", errorCode, action, _filename);
        exit(1);
    }

    const char* _filename;
    UInt16 _handle;
    Bool _opened;
};

UInt8 conversionTable[256];

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Syntax: %s <name of .wav file to play>\n", argv[0]);
        return 0;
    }

    UInt16 paras = totalMemory() << 6;
    // 32KB is large enough not to fix into any gaps but small enough that
    // it's unlikely to fail.
    void __far *p = malloc(32767);
    UInt16 firstPara = _FP_SEG(p) + ((_FP_OFF(p) + 15) >> 4);
    paras -= firstPara;

    UInt16 nextPara = firstPara;
//    printf("Memory available: 0x%04x paragraphs starting at 0x%04x\n", paras,
//        firstPara);
    printf("%iKB available\n", paras >> 6);

    File file;
    const char* filename = argv[1];
    file.init(filename);
    file.open();
    WaveHeader w;
    file.read(sizeof(WaveHeader), _FP_OFF(w), _FP_SEG(w));
    if (!w.verify()) {
        printf("Unrecognized format reading %s\n", filename);
        exit(1);
    }
    UInt32 nSamples = w._dLen / w._nBlockAlign;

    printf("File has sample rate %liHz. Checking speed...\n",
        w._nSamplesPerSec);

    UInt16 timerCount = (UInt16)(1193182L/w._nSamplesPerSec);
    // For calibration, pick a number of samples which is divisible by 18
    UInt16 canary = playWithTimer(nextPara, nextPara + 9, timerCount,
        interrupt8test, mulh(9*16, timerCount));
    Bool useIRQ0 = true;
    UInt16 nops;
    if (canary != 9*16) {
        useIRQ0 = false;

        int nopsLow = -1;
        int nopsHigh = maxNops() + 1;
        UInt32 totalAdjust = 0;
        UInt16 timerIdeal = timerCount*9*16;
        UInt16 timerLow = 0;
        UInt16 timerHigh = 0;
        while (nopsHigh - nopsLow > 1) {
            nops = (nopsHigh + nopsLow) / 2;
            UInt16 timer9 = playCalibrated(firstPara, firstPara + 9, nops, 0);
            UInt16 timer18 =
                playCalibrated(firstPara, firstPara + 18, nops, 0);
            totalAdjust += timer9 + timer18;
            UInt16 timer = timer18 - timer9;
            if (timer >= timerIdeal) {
                nopsHigh = nops;
                timerHigh = timer;
            }
            if (timer <= timerIdeal) {
                nopsLow = nops;
                timerLow = timer;
            }
        }
        if (nopsLow == -1) {
            nopsLow = nopsHigh;
            timerLow = timerHigh;
        }
        nops = nopsLow;

        // Adjust BIOS time-of-day
        playCalibrated(firstPara, firstPara, 0, (UInt8)(totalAdjust >> 16));

        printf("Using cycle counting. Actual playback frequency = %liHz\n",
            (UInt16)(171818182L/timerLow));
    }
    else
        printf("Using timer. Actual playback frequency = %liHz\n",
            (UInt16)(1193182L/timerCount));

    UInt16 t = 0;
    if (w._wBitsPerSample == 8)
        for (int i = 0; i < 256; ++i) {
            conversionTable[i] = (UInt8)((t >> 8) + 1);
            t += timerCount;
        }
    else
        for (int i = -128; i < 128; ++i) {
            conversionTable[i & 0xff] = (UInt8)((t >> 8) + 1);
            t += timerCount;
        }

    int shift =
        (w._wBitsPerSample == 16 ? 1 : 0) + (w._nChannels == 2 ? 1 : 0);

    do {
        UInt16 bytesToConvert = 512;
        UInt32 remainingOnDisk = nSamples << shift;
        UInt32 remainingInMemory = (UInt32)paras << 4;
        if (bytesToConvert > remainingOnDisk)
            bytesToConvert = (UInt16)remainingOnDisk;
        if (bytesToConvert > remainingInMemory)
            bytesToConvert = (UInt16)remainingInMemory;
        if (bytesToConvert == 0)
            break;

        file.read(bytesToConvert, 0, nextPara);

        int samplesConverted;
        UInt8 __far* from = (UInt8 __far*)_MK_FP(nextPara, 0);
        UInt8 __far* to = from;
        if (w._nChannels == 1)
            if (w._wBitsPerSample == 8) {
                for (UInt16 i = 0; i < bytesToConvert; ++i) {
                    *to = conversionTable[*from];
                    ++from;
                    ++to;
                }
                samplesConverted = bytesToConvert;
            }
            else {
                for (UInt16 i = 0; i < bytesToConvert >> 1; ++i) {
                    ++from;
                    *to = conversionTable[*from];
                    ++from;
                    ++to;
                }
                samplesConverted = bytesToConvert >> 1;
            }
        else
            if (w._wBitsPerSample == 8) {
                for (UInt16 i = 0; i < bytesToConvert >> 1; ++i) {
                    UInt16 t = *from;
                    ++from;
                    t += *from;
                    ++from;
                    *to = conversionTable[t >> 1];
                    ++to;
                }
                samplesConverted = bytesToConvert >> 1;
            }
            else {
                for (UInt16 i = 0; i < bytesToConvert >> 2; ++i) {
                    ++from;
                    SInt16 t = (SInt8)*from;
                    from += 2;
                    t += (SInt8)*from;
                    ++from;
                    *to = conversionTable[t >> 1];
                    ++to;
                }
                samplesConverted = bytesToConvert >> 2;
            }
        while ((samplesConverted & 0xf) != 0) {
            *to = 0;
            ++to;
            ++samplesConverted;
        }

        int p = samplesConverted >> 4;
        nextPara += p;
        paras -= p;
        nSamples -= samplesConverted;
    } while (true);

    UInt16 catchUps = mulh(nextPara - firstPara, timerCount*16);
    if (useIRQ0)
        playWithTimer(firstPara, nextPara, timerCount, interrupt8play,
            catchUps);
    else
        playCalibrated(firstPara, nextPara, nops, catchUps);

    return 0;
}
