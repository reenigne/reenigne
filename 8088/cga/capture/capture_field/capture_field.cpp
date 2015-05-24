#include "alfe/main.h"
#include "alfe/complex.h"
#include "alfe/terminal6.h"
#include "alfe/bitmap_png.h"
#include "alfe/evaluate.h"
#include "alfe/ntsc_decode.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        String name = "captured.png";
        int frames = 1;
        if (_arguments.count() >= 2) {
            name = _arguments[1];
            if (_arguments.count() >= 3)
                frames = evaluate<int>(_arguments[2]);
        }
        //String inName = "q:\\input.raw";
        //String inName = "q:\\bottom.raw";
        //String inName = "q:\\6cycle.raw";
        //AutoHandle h = File(inName, true).openRead();
        AutoHandle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);

        int samples = 450*1024*frames;
        int sampleSpaceBefore = 256;
        int sampleSpaceAfter = 256;
        Array<Byte> buffer(sampleSpaceBefore + samples + sampleSpaceAfter);
        Byte* b = &buffer[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i)
            b[i - sampleSpaceBefore] = 0;
        for (int i = 0; i < sampleSpaceAfter; ++i)
            b[i + samples] = 0;
        for (int i = 0; i < 450*frames; ++i)
            h.read(&b[i*1024], 1024);

        Bitmap<SRGB> decoded(Vector(640, 240));

        NTSCCaptureDecoder<SRGB> decoder;
        decoder.setBuffers(b, decoded);

        decoder.decode();
        PNGFileFormat<SRGB> png;
        Bitmap<SRGB> output(Vector(640, 480));

        //float z0 = 0;
        //for (int o = 0; o < 480; ++o) {
        //    //float a = 0;
        //    float t = 0;
        //    float fk = o*240.0/480.0;
        //    int k = fk;
        //    fk -= k;
        //    int firstInput = fk - 3;
        //    int lastInput = fk + 3;
        //    Vector3<float> a[640];
        //    for (int x = 0; x < 640; ++x)
        //        a[x] = Vector3<float>(0, 0, 0);
        //    for (int j = firstInput; j <= lastInput; ++j) {
        //        float s = lanczos(j + z0);
        //        int y = j+k;
        //        if (y >= 0 && y < 240) {
        //            for (int x = 0; x < 640; ++x) {
        //                SRGB srgb = decoded[Vector(x, y)];
        //                a[x] += Vector3Cast<float>(srgb)*s;
        //            }
        //            t += s;
        //        }
        //    }
        //    for (int x = 0; x < 640; ++x) {
        //        Vector3<float> c = a[x]/t;
        //        output[Vector(x, o)] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
        //    }
        //    int k1 = (o + 1)*240.0/480.0;
        //    z0 += (k1 - k) - 240.0/480.0;
        //}
        for (int o = 0; o < 478; o += 2) {
            for (int x = 0; x < 640; ++x) {
                output[Vector(x, o)] = decoded[Vector(x, o/2)];
                Vector3<int> a = Vector3Cast<int>(decoded[Vector(x, o/2)]) + Vector3Cast<int>(decoded[Vector(x, o/2 + 1)]);
                output[Vector(x, o+1)] = Vector3Cast<Byte>(a/2);
                //output[Vector(x, o+1)] = decoded[Vector(x, o/2)];
            }
        }
        for (int x = 0; x < 640; ++x) {
            output[Vector(x, 478)] = decoded[Vector(x, 239)];
            output[Vector(x, 479)] = decoded[Vector(x, 239)];
        }

        png.save(output, File(name, true));

        AutoHandle out = File(name + ".raw", true).openWrite();
        out.write(b, 450*1024);
    }
};
