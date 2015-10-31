#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "alfe/main.h"
#include "alfe/complex.h"
#include "alfe/evaluate.h"
#include "alfe/bitmap.h"
#include "alfe/ntsc_decode.h"

#ifdef WIN32
#define ZLIB_WINAPI
#endif
#include "zlib.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        bool doDecode = true;

        static const int samples = 450*1024;
        static const int inputBufferSize = samples;
        static const int sampleSpaceBefore = 256;
        static const int sampleSpaceAfter = 256;

        FileStream in = File("captured.zdr", true).openRead();
        UInt64 inputFileSizeRemaining = in.size();
        Array<Byte> inputBuffer(inputBufferSize);
        int inputBufferRemaining = 0;
        Byte* inputPointer = 0;
        z_stream zs;
        memset(&zs, 0, sizeof(z_stream));
        if (inflateInit(&zs) != Z_OK)
            throw Exception("inflateInit failed");

        Array<Byte> buffer(sampleSpaceBefore + samples + sampleSpaceAfter);
        Byte* b = &buffer[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i)
            b[i - sampleSpaceBefore] = 0;
        for (int i = 0; i < sampleSpaceAfter; ++i)
            b[i + samples] = 0;
        int outputBytesRemaining = samples;

        Vector outputSize;
        NTSCCaptureDecoder<UInt32> decoder;

        if (doDecode)
            outputSize = Vector(960, 240);
        else
            outputSize = Vector(1824, 253);
        Bitmap<UInt32> decoded(outputSize);
        decoder.setOutputBuffer(decoded);
        decoded.fill(0);
        decoder.setInputBuffer(b);
        decoder.setOutputPixelsPerLine(1140);
        decoder.setYScale(1);
        decoder.setDoDecode(doDecode);
        //decoder.setBrightness(-71);
        decoder.setBrightness(-100);
        decoder.setSaturation(0.33);
        //decoder.setContrast(2.13);
        decoder.setContrast(2.47);
        decoder.setHue(0);
        decoder.setChromaSamples(16);

        FileStream outputStream = File("u:\\captured_decoded.bin", true).openWrite();

        do {
            if (inputBufferRemaining == 0) {
                int bytesToRead = inputBufferSize;
                if (bytesToRead > inputFileSizeRemaining)
                    bytesToRead = inputFileSizeRemaining;
                inputPointer = &inputBuffer[0];
                in.read(inputPointer, bytesToRead);
                inputBufferRemaining = bytesToRead;
                inputFileSizeRemaining -= bytesToRead;
            }
            zs.avail_in = inputBufferRemaining;
            zs.next_in = inputPointer;
            zs.avail_out = outputBytesRemaining;
            zs.next_out = b + samples - outputBytesRemaining;
            int r = inflate(&zs, Z_SYNC_FLUSH);
            if (r != Z_STREAM_END && r != Z_OK)
                throw Exception("inflate failed");
            outputBytesRemaining = zs.avail_out;
            inputPointer = zs.next_in;
            inputBufferRemaining = zs.avail_in;

            if (outputBytesRemaining == 0) {
                if (inflateReset(&zs) != Z_OK)
                    throw Exception("inflateReset failed");
                outputBytesRemaining = samples;
                console.write(".");

                if (doDecode) {
                    decoder.decode();
                    outputStream.write(decoded.data(), decoded.stride()*outputSize.y);
                }
                else
                    outputStream.write(b, 1824*253);
            }

        } while (inputFileSizeRemaining != 0);

        if (inflateEnd(&zs) != Z_OK)
            throw Exception("inflateEnd failed");
    }
};
