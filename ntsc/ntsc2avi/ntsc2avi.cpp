#include "alfe/main.h"
#include "alfe/avi.h"
#include "alfe/ntsc_decode.h"
#include "alfe/scanlines.h"
#include "alfe/config_file.h"
#include "alfe/wrap.h"

int sampleFromIRE(float IRE) { return static_cast<int>(IRE * 1.4 + 60.49999); }

class Program : public ProgramBase
{
public:
    void run()
    {
        ConfigFile configFile;
        configFile.addOption("inputNTSC", StringType());
        configFile.addDefaultOption("contrast", 100.0);
        configFile.addDefaultOption("brightness", 0.0);
        configFile.addDefaultOption("saturation", 100.0);
        configFile.addDefaultOption("hue", 0.0);
        configFile.addDefaultOption("showClipping", false);
        configFile.addDefaultOption("chromaBandwidth", 1.0);
        configFile.addDefaultOption("lumaBandwidth", 1.0);
        configFile.addDefaultOption("rollOff", 0.0);
        configFile.addDefaultOption("lobes", 4.0);

        configFile.addDefaultOption("aspectRatio", 5.0 / 6.0);
        configFile.addDefaultOption("scanlineWidth", 0.5);
        configFile.addDefaultOption("scanlineProfile", 0);
        configFile.addDefaultOption("horizontalProfile", 0);

        configFile.addDefaultOption("scanlineBleeding", 2);
        configFile.addDefaultOption("horizontalBleeding", 2);
        configFile.addDefaultOption("zoom", 2.0);
        configFile.addDefaultOption("horizontalRollOff", 0.0);
        configFile.addDefaultOption("verticalRollOff", 0.0);
        configFile.addDefaultOption("horizontalLobes", 4.0);
        configFile.addDefaultOption("verticalLobes", 4.0);
        configFile.addDefaultOption("subPixelSeparation", 1.0);
        configFile.addDefaultOption("phosphor", 0);
        configFile.addDefaultOption("mask", 0);
        configFile.addDefaultOption("maskSize", 0.0);
        configFile.addDefaultOption("overscan", 0.1);
        configFile.addDefaultOption("combFilter", 0);
        configFile.addDefaultOption("fftWisdom", String("wisdom"));

        List<Value> arguments;

        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name>(.ntsc|.config)\n");
            return;
        }
        String configPath = _arguments[1];
        int n = configPath.length();
        if (configPath.endsInIgnoreCase(".ntsc")) {
            configPath = "default.config";
            arguments.add(_arguments[0] + " " + configPath);
            for (int i = 1; i < _arguments.count(); ++i)
                arguments.add(_arguments[i]);
        }
        else {
            arguments.add(_arguments[0] + " " + configPath);
            for (int i = 1; i < _arguments.count() - 1; ++i)
                arguments.add(_arguments[i + 1]);
        }

        configFile.addDefaultOption("arguments",
            ArrayType(StringType(), IntegerType()), arguments);

        File config(configPath, true);
        configFile.load(config);

        FFTWWisdom<float> wisdom(
            File(configFile.get<String>("fftWisdom"), config.parent()));

        NTSCDecoder decoder;
        Linearizer linearizer;
        ScanlineRenderer scalers[2];
        int scalerNumber = 0;
        AVIFileInitializer aviFileInitializer;


        linearizer.setShowClipping(configFile.get<bool>("showClipping"));

        double overscan = configFile.get<double>("overscan");
        int combFilter = configFile.get<int>("combFilter");

        Vector2<float> zoomVector = Vector2<float>(static_cast<float>(
            configFile.get<double>("aspectRatio")) / 2.0f, 1.0f) *
            static_cast<float>(configFile.get<double>("zoom"));

        int black = sampleFromIRE(7.5);
        int white = sampleFromIRE(100);
        decoder.setHue(configFile.get<double>("hue") +
            (combFilter == 2 ? 180 : 0));

        double saturation = configFile.get<double>("saturation") / 100;
        double contrast = configFile.get<double>("contrast") / 100;
        double brightness = configFile.get<double>("brightness") / 100;

        static const int combDivisors[3] = { 1, 2, 4 };
        int scaling = combDivisors[combFilter];

        double c = 256.0 / (white - black);

        decoder.setContrast(contrast * c);
        decoder.setSaturation(saturation * c);
        decoder.setBrightness((-black * c + brightness * 5) / 256);

        decoder.setChromaBandwidth(configFile.get<double>("chromaBandwidth"));
        decoder.setLumaBandwidth(configFile.get<double>("lumaBandwidth"));
        decoder.setRollOff(configFile.get<double>("rollOff"));
        decoder.setLobes(configFile.get<double>("lobes"));

        double overscanScale = overscan * 2 + 1;
        Vector outputSize = Vector2Cast<int>(Vector2<float>(640, 200) *
            zoomVector * static_cast<float>(overscanScale));


        // We parse the filename relative to the current directory here instead
        // of relative to the config file path because the filename usually
        // comes from the command line.
        String inputName = configFile.get<String>("inputNTSC");
        FileStream fs = File(inputName, true).openRead();

        AVIWriter aviWriter(File(inputName + ".avi", true),
            Vector2Cast<int>(outputSize), Rational(60000, 1001), NULL);

        int pllWidth = 910;
        int pllHeight = (910 * 525) / 2;
        static const int driftHorizontal = 8;
        static const int driftVertical = 14 * pllWidth;

        int maxWidth = pllWidth + driftHorizontal;

        static const int syncThreshold = 30;
        static const int decoderPadding = 32;
        decoder.setPadding(decoderPadding);
        static const int fftLength = 512;
        static const int stride = fftLength - 2 * decoderPadding;
        const int blocksPerScanline = static_cast<int>(
            (640 * overscanScale + (stride - 1)) / stride);
        const int decodedSamplesPerScanline = blocksPerScanline * stride;
        const int scanlines = static_cast<int>(200 * overscanScale);
        Array<Byte> srgb(decodedSamplesPerScanline * 3);

        int bufferSize = (pllHeight / pllWidth) * maxWidth + driftVertical +
            2 * decoderPadding + combFilter * pllWidth;
        Array<Byte> buffer(bufferSize);
        SInt64 samplesRemaining = fs.size();

        Vector2<float> inputTL = Vector2<float>(64.0f, 20.0f)
            - static_cast<float>(overscan) * Vector2<float>(640, 200);
        Vector2<float> scalerOffset(-decoderPadding - 0.5f, 0);
        if (combFilter == 2)
            scalerOffset += Vector2<float>(2, -1);
        scalerOffset += inputTL + Vector2<float>(0.5f, 0.5f) / zoomVector;
        for (int i = 0; i < 2; ++i) {
            ScanlineRenderer* s = &scalers[i];
            s->setProfile(configFile.get<int>("scanlineProfile"));
            s->setHorizontalProfile(configFile.get<int>("horizontalProfile"));
            s->setWidth(static_cast<float>(
                configFile.get<double>("scanlineWidth")));
            s->setBleeding(configFile.get<int>("scanlineBleeding"));
            s->setHorizontalBleeding(
                configFile.get<int>("horizontalBleeding"));
            s->setHorizontalRollOff(static_cast<float>(
                configFile.get<double>("horizontalRollOff")));
            s->setVerticalRollOff(
                static_cast<float>(configFile.get<double>("verticalRollOff")));
            s->setHorizontalLobes(
                static_cast<float>(configFile.get<double>("horizontalLobes")));
            s->setVerticalLobes(
                static_cast<float>(configFile.get<double>("verticalLobes")));
            s->setSubPixelSeparation(static_cast<float>(
                configFile.get<double>("subPixelSeparation")));
            s->setPhosphor(configFile.get<int>("phosphor"));
            s->setMask(configFile.get<int>("mask"));
            s->setMaskSize(
                static_cast<float>(configFile.get<double>("maskSize")));
            s->setZoom(zoomVector);
            s->setOffset(scalerOffset);
            s->setOutputSize(outputSize);
            s->init();
        }

        AlignedBuffer unscaled = scalers[0].input();
        AlignedBuffer scaled = scalers[0].output();
        Vector tl = scalers[0].inputTL();
        scalerNumber = 0;

        static const Byte initialBurst[4] = { 75, 37, 45, 83 };
        decoder.init();
        decoder.calculateBurst(initialBurst);


        // Pad the buffer at the start. To avoid artifacts just repeat the
        // first scanline.
        int padding = (max(1, (1 - tl.y)) * pllWidth) & ~3;
        if (samplesRemaining < pllWidth)
            throw Exception("Input file too small.");
        int bufferRemaining = padding + pllWidth;
        int bufferOffset = bufferSize - bufferRemaining;
        fs.read(&buffer[bufferOffset], pllWidth);
        for (int i = 0; i < padding; ++i)
            buffer[i + pllWidth + bufferOffset] = buffer[i + bufferOffset];
        samplesRemaining -= pllWidth;
        int xPadding = max(0, -tl.x) & ~3;


        int scanlineOffset = padding + tl.y*pllWidth - xPadding;
        int fieldOffset = -682;

        while (samplesRemaining > 0) {
            // Move any remaining bytes to the start of the buffer and ensure
            // the buffer is full.
            int spaceToFill = bufferSize - bufferRemaining;
            memcpy(&buffer[0], &buffer[spaceToFill], bufferRemaining);
            if (samplesRemaining > spaceToFill) {
                fs.read(&buffer[bufferRemaining], spaceToFill);
            }
            else {
                int r = static_cast<int>(samplesRemaining);
                fs.read(&buffer[bufferRemaining], r);
                // TODO: fill with static instead?
                memset(&buffer[bufferRemaining + r], 0, spaceToFill - r);
            }
            samplesRemaining -= spaceToFill;
            bufferRemaining = bufferSize;


            // Adjust vertical offset for interlacing

            ScanlineRenderer* scaler = &scalers[scalerNumber];
            // Optimise the common case of interlacing
            scalerNumber = 1 - scalerNumber;
            scaler->setOffset(scalerOffset + Vector2<float>(0,
                static_cast<float>(682 + fieldOffset) / pllWidth));
            scaler->init();
            AlignedBuffer unscaled = scaler->input();
            AlignedBuffer scaled = scaler->output();
            Vector tl = scaler->inputTL();
            Vector br = scaler->inputBR();
            Vector unscaledSize = br - tl;


            // Find vertical sync pulse for next field
            {
                int offset = fieldOffset + pllHeight - driftVertical;
                Byte* p = &buffer[offset - 12*pllWidth - 186];
                int n = driftVertical * 2;
                int j;
                int s = 0;
                for (j = 0; j < n; ++j) {  // j += 57
                    if (p[j] < syncThreshold) {
                        ++s;
                        if (s == 3*57)
                            break;
                    }
                    else
                        s = max(0, s - 1);
                }
                fieldOffset = j + offset;
            }


            // Per scanline processing

            Byte* unscaledRow = unscaled.data();
            for (int y = 0;; ++y) {
                // Apply comb filter and decode to sRGB

                Byte* srgbP = &srgb[0];
                Byte* ntscBlock = &buffer[scanlineOffset];
                switch (combFilter) {
                    case 0:
                        // No comb filter
                        for (int j = 0; j < blocksPerScanline; ++j) {
                            decoder.decodeNTSC(ntscBlock,
                                reinterpret_cast<SRGB*>(srgbP));
                            srgbP += stride * 3;
                            ntscBlock += stride;
                        }
                        break;
                    case 1:
                        // 1 line.
                        for (int j = 0; j < blocksPerScanline; ++j) {
                            Byte* n0 = ntscBlock;
                            Byte* n1 = n0 + pllWidth;
                            float* y = decoder.yData();
                            float* i = decoder.iData();
                            float* q = decoder.qData();
                            for (int x = 0; x < fftLength; x += 4) {
                                y[0] = static_cast<float>(2 * n0[0]);
                                y[1] = static_cast<float>(2 * n0[1]);
                                y[2] = static_cast<float>(2 * n0[2]);
                                y[3] = static_cast<float>(2 * n0[3]);
                                i[0] = -static_cast<float>(n0[1] - n1[1]);
                                i[1] = static_cast<float>(n0[3] - n1[3]);
                                q[0] = static_cast<float>(n0[0] - n1[0]);
                                q[1] = -static_cast<float>(n0[2] - n1[2]);
                                n0 += 4;
                                n1 += 4;
                                y += 4;
                                i += 2;
                                q += 2;
                            }
                            decoder.decodeBlock(
                                reinterpret_cast<SRGB*>(srgbP));
                            srgbP += stride * 3;
                            ntscBlock += stride;
                        }
                        break;
                    case 2:
                        // 2 line.
                        for (int j = 0; j < blocksPerScanline; ++j) {
                            Byte* n0 = ntscBlock;
                            Byte* n1 = n0 + pllWidth;
                            Byte* n2 = n1 + pllWidth;
                            float* y = decoder.yData();
                            float* i = decoder.iData();
                            float* q = decoder.qData();
                            for (int x = 0; x < fftLength; x += 4) {
                                y[0] = static_cast<float>(4 * n1[0]);
                                y[1] = static_cast<float>(4 * n1[1]);
                                y[2] = static_cast<float>(4 * n1[2]);
                                y[3] = static_cast<float>(4 * n1[3]);
                                i[0] = static_cast<float>(
                                    n0[1] + n2[1] - 2 * n1[1]);
                                i[1] = static_cast<float>(
                                    2 * n1[3] - n0[3] - n2[3]);
                                q[0] = static_cast<float>(
                                    2 * n1[0] - n0[0] - n2[0]);
                                q[1] = static_cast<float>(
                                    n0[2] + n2[2] - 2 * n1[2]);
                                n0 += 4;
                                n1 += 4;
                                n2 += 4;
                                y += 4;
                                i += 2;
                                q += 2;
                            }
                            decoder.decodeBlock(
                                reinterpret_cast<SRGB*>(srgbP));
                            srgbP += stride * 3;
                            ntscBlock += stride;
                        }
                        break;
                }


                // Shift, clip, show clipping and linearization

                if (y < scanlines) {
                    float* p = reinterpret_cast<float*>(unscaledRow);
                    const Byte* s = &srgb[0];
                    int scanlineChannels = unscaledSize.x * 3;
                    for (int x = 0; x < scanlineChannels; ++x)
                        p[x] = linearizer.linear(s[x]);
                    unscaledRow += unscaled.stride();


                    //Byte* output = aviWriter.bits() + aviWriter.stride()*y;
                    //Byte* q = &buffer[scanlineOffset];
                    //for (int x = 0; x < outputSize.x; ++x) {
                    //    output[0] = q[x*3 + 0];
                    //    output[1] = q[x*3 + 1];
                    //    output[2] = q[x*3 + 2];
                    //    output += 3;
                    //}
                }


                // Find horizontal sync pulse for next scanline

                int offset = scanlineOffset + pllWidth - driftHorizontal;
                Byte* p = &buffer[offset + xPadding - 126];
                int n = driftHorizontal * 2 + 3;
                int i;
                int s = 0;
                for (i = 0; i < n; ++i) {
                    if (p[i] < syncThreshold) {
                        ++s;
                        if (s == 3)
                            break;
                    }
                    else
                        s = max(0, s - 1);
                }
                int samplesConsumed = i + pllWidth - driftHorizontal;
                scanlineOffset += samplesConsumed;


                // Calculate color burst for next scanline

                float burstF[4];
                for (int j = 0; j < 4; ++j)
                    burstF[j] = 0;
                p += i + 77;  // 864 - (pllWidth - 123) 
                for (int j = 0; j < 28; ++j)
                    burstF[j & 3] = p[j];
                Byte burst[4];
                for (int j = 0; j < 4; ++j)
                    burst[j] = static_cast<int>(burstF[j] / 7);
                decoder.calculateBurst(burst);

                bufferRemaining -= samplesConsumed;
                fieldOffset -= samplesConsumed;
                if (fieldOffset < 0)
                    break;
            }
            scanlineOffset -= bufferSize - bufferRemaining;

            // Scale to desired size and apply scanline filter
            scaler->render();


            // Delinearization and float-to-byte conversion
            const Byte* scaledRow = scaled.data();
            Byte* outputRow =
                aviWriter.bits() + (outputSize.y - 1)*aviWriter.stride();
            for (int y = 0; y < outputSize.y; ++y) {
                auto p = reinterpret_cast<const float*>(scaledRow);
                Byte* output = reinterpret_cast<Byte*>(outputRow);
                for (int x = 0; x < outputSize.x; ++x) {
                    SRGB srgb = linearizer.srgb(Colour(p[0], p[1], p[2]));
                    //output[0] = x * 255 / outputSize.x; // srgb.x;
                    //output[1] = y * 255 / outputSize.y; // srgb.y;
                    //static int frame = 0;
                    //if (x == 0 && y == 0)
                    //    ++frame;
                    //output[2] = frame; // srgb.z;

                    output[0] = srgb.x;
                    output[1] = srgb.y;
                    output[2] = srgb.z;

                    output += 3;
                    p += 3;
                }
                scaledRow += scaled.stride();
                outputRow -= aviWriter.stride();
            }

            aviWriter.AddAviFrame();
        }
    }
};
