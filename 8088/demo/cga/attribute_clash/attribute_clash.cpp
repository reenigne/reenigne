#include "alfe/main.h"
#include "alfe/file.h"
#include "alfe/colour_space.h"
#include <stdio.h>
#include "alfe/user.h"
#include "alfe/thread.h"
#include "alfe/bitmap.h"
#include "alfe/config_file.h"
#include "alfe/bitmap_png.h"

typedef Vector3<int> YIQ;

class AttributeClashImage : public Image
{
public:
    AttributeClashImage(ConfigFile* config)
    {
        String cgaRomFileName = config->get<String>("cgaRomFile");
        _iterations = config->get<int>("iterations");
        _colourSpace = config->get<ColourSpace>("colourSpace");
        Array<Any> inputSizeArray = config->get<List<Any> >("inputSize");
        _inputSize = Vector(inputSizeArray[0].value<int>(),
            inputSizeArray[1].value<int>());
        Array<Any> outputSizeArray = config->get<List<Any> >("outputSize");
        _outputSize = Vector(outputSizeArray[0].value<int>(),
            outputSizeArray[1].value<int>());
        Array<Any> offsetArray = config->get<List<Any> >("offset");
        _offset = Vector(offsetArray[0].value<int>(),
            offsetArray[1].value<int>());
        static const int overscanColour = config->get<int>("overscanColour");
        String inputPictureFileName = config->get<String>("inputPicture");
        _hres = config->get<bool>("hres");
        _composite = config->get<bool>("compositeTarget");

        _outputNTSCFile = config->get<String>("outputNTSC");
        _outputCompositeFile = config->get<String>("outputComposite");
        _outputDigitalFile = config->get<String>("outputDigital");
        _outputDigitalRawFile = config->get<String>("outputDigitalRaw");
        _outputDataFile = config->get<String>("outputData");

        // Determine the set of unique patterns that appear in the top lines
        // of CGA text characters.
        {
            File file(cgaRomFileName);
            String cgaROM = file.contents();
            _patterns.allocate(0x100);
            _characters.allocate(0x100);
            _patternCount = 0;
            for (int i = 0; i < 0x100; ++i) {
                UInt8 bits = cgaROM[(3*256 + i)*8];
                _patterns[i] = bits;
                int j;
                for (j = 0; j < _patternCount; ++j)
                    if (_patterns[_characters[j]] == bits ||
                        _patterns[_characters[j]] == ~bits)
                        break;
                if (j == _patternCount) {
                    _characters[_patternCount] = i;
                    ++_patternCount;
                }
            }
        }
        _gamma.allocate(256);
        _gamma.allocate(256);
         for (int i = 0; i < 256; ++i)
            _gamma[i] = static_cast<int>(
                pow(static_cast<float>(i)/255.0f, 1.9f)*255.0f);

        _iteration = 0;

        class ConvertSRGBToLinear
        {
        public:
            ConvertSRGBToLinear() : _c(ColourSpace::rgb()) { }
            Vector3<float> convert(SRGB c)
            {
                return Vector3Cast<float>(_c.fromSrgb(c));
            }
        private:
            ColourSpace _c;
        };
        class ConvertLinearToSRGB
        {
        public:
            ConvertLinearToSRGB() : _c(ColourSpace::rgb()) { }
            SRGB convert(Vector3<float> c)
            {
                return _c.toSrgb24(c);
            }
        private:
            ColourSpace _c;
        };
        Bitmap<SRGB> srgbInputOriginal =
            PNGFileFormat().load(inputPictureFileName);
        Bitmap<Vector3<float> > linearInput(srgbInputOriginal.size());
        srgbInputOriginal.convert(linearInput, ConvertSRGBToLinear());
        Bitmap<Vector3<float> > linearScaled(_inputSize);
        linearInput.resample(linearScaled);
        Bitmap<SRGB> srgbInput(_inputSize);
        linearScaled.convert(srgbInput, ConvertLinearToSRGB());

        _srgbPalette[0x00] = SRGB(0x00, 0x00, 0x00);
        _srgbPalette[0x01] = SRGB(0x00, 0x00, 0xaa);
        _srgbPalette[0x02] = SRGB(0x00, 0xaa, 0x00);
        _srgbPalette[0x03] = SRGB(0x00, 0xaa, 0xaa);
        _srgbPalette[0x04] = SRGB(0xaa, 0x00, 0x00);
        _srgbPalette[0x05] = SRGB(0xaa, 0x00, 0xaa);
        _srgbPalette[0x06] = SRGB(0xaa, 0x55, 0x00);
        _srgbPalette[0x07] = SRGB(0xaa, 0xaa, 0xaa);
        _srgbPalette[0x08] = SRGB(0x55, 0x55, 0x55);
        _srgbPalette[0x09] = SRGB(0x55, 0x55, 0xff);
        _srgbPalette[0x0a] = SRGB(0x55, 0xff, 0x55);
        _srgbPalette[0x0b] = SRGB(0x55, 0xff, 0xff);
        _srgbPalette[0x0c] = SRGB(0xff, 0x55, 0x55);
        _srgbPalette[0x0d] = SRGB(0xff, 0x55, 0xff);
        _srgbPalette[0x0e] = SRGB(0xff, 0xff, 0x55);
        _srgbPalette[0x0f] = SRGB(0xff, 0xff, 0xff);

        for (int i = 0; i < 0x10; ++i)
            _linearPalette[i] = ColourSpace::rgb().fromSrgb(_srgbPalette[i]);

        //for (int i = 0; i < 0x10; ++i)
        //    _perceptualPalette[i] = _colourSpace.fromSrgb(_srgbPalette[i]);

        _position = Vector(0, 0);
        _changed = false;

        _dataOutput = Bitmap<UInt16>(
            Vector(_inputSize.x/(_hres ? 8 : 16), _inputSize.y));
        _compositeData = Bitmap<YIQ>(_outputSize + Vector(8, 0));
        _digitalOutput = Bitmap<SRGB>(_outputSize);
        _digitalOutputRaw = Bitmap<UInt8>(_inputSize);
        _compositeOutput = Bitmap<SRGB>(_outputSize + Vector(22, 0));
        _perceptualInput = Bitmap<Colour>(_outputSize);
        _linearInput = Bitmap<Colour>(_outputSize);
        _linearOutput = Bitmap<Colour>(_outputSize);
        _linearError = Bitmap<Colour>(_outputSize);
        _perceptualInput.fill(Colour(0, 0, 0));
        _linearInput.fill(Colour(0, 0, 0));
        _linearOutput.fill(Colour(0, 0, 0));
        _linearError.fill(Colour(0, 0, 0));

        static const float brightness = 0.06f;
        static const float contrast = 3.0f;
        static const float saturation = 0.7f;
        static const float tint = 270.0f + 33.0f;

        _yContrast = static_cast<int>(contrast*1463.0f);
        static const float radians = static_cast<float>(tau)/360;
        float tintI = -cos(tint*radians);
        float tintQ = sin(tint*radians);

        // Determine the color burst.
        float colorBurst[4];
        // First set _iqMultipliers to 0. The result of the first update will
        // have valid Y data but not valid I and Q data yet. Fortunately we
        // only need the Y data to find the color burst.
        for (int i = 0; i < 4; ++i)
            _iqMultipliers[i] = 0;
        UInt16 overscanData = (overscanColour << 8) | 0xdb;
        update(-_offset, overscanData);
        for (int i = 0; i < 4; ++i)
            colorBurst[i] =
                static_cast<float>(_compositeData[Vector(i + 8, 0)].x);
        float burstI = colorBurst[2] - colorBurst[0];
        float burstQ = colorBurst[3] - colorBurst[1];
        float colorBurstGain = 32.0f/sqrt((burstI*burstI + burstQ*burstQ)/2);
        float s = saturation*contrast*colorBurstGain*0.352f;
        _iqMultipliers[0] = static_cast<int>((burstI*tintI - burstQ*tintQ)*s);
        _iqMultipliers[1] = static_cast<int>((burstQ*tintI + burstI*tintQ)*s);
        _iqMultipliers[2] = -_iqMultipliers[0];
        _iqMultipliers[3] = -_iqMultipliers[1];

        _brightness =
            static_cast<int>(brightness*100.0 - 7.5f*256.0f*contrast)<<8;

        // Now that _iqMultipliers has been initialized correctly, we can do a
        // full initialization.
        int hdots = _hres ? 8 : 16;
        for (int y = 0; y < _outputSize.y; ++y) {
            update(Vector(-8, y) - _offset, overscanData);
            for (int x = 0; x < _outputSize.x; x += hdots)
                update(Vector(x, y) - _offset, overscanData);
        }

        // Set up the linear input, output and error and perceptual input
        Colour border = _linearOutput[Vector(6, 0)];
        for (int y = 0; y < _outputSize.y; ++y)
            for (int x = 0; x < _outputSize.x; ++x) {
                Vector p(x, y);
                Colour rgb;
                Vector q = p - _offset;
                if (q.inside(_inputSize))
                    rgb = ColourSpace::rgb().fromSrgb(srgbInput[q]);
                else
                    rgb = border;
                _linearInput[p] = rgb;
                _perceptualInput[p] = _colourSpace.fromRgb(rgb);
                _linearOutput[p] = border;
                _linearError[p] = border - rgb;
            }

        //for (int y = 0; y < _outputSize.y; ++y)
        //    for (int x = 0; x < _outputSize.x; ++x) {
        //        int col = (y/8)*40 + (x/16);
        //        int ch;
        //        int at;
        //        if (col < 16) {
        //            ch = 0;
        //            at = col*0x11;
        //        }
        //        else {
        //            col -= 16;
        //            int pattern = col / 240;
        //            static int chars[4] = {0xb0, 0xb1, 0x13, 0x48};
        //            ch = chars[pattern];
        //            at = col % 240;
        //            int fg = at%15;
        //            int bg = at/15;
        //            if (fg >= bg)
        //                ++fg;
        //            at = fg + (bg << 4);
        //        }
        //        int o = x/(_hres ? 8 : 16);
        //        Vector p(o, y);
        //        _dataOutput.pixel(p) = ch | (at << 8);
        //        _position = Vector(o*(_hres ? 8 : 16), y);
        //        errorFor(_patterns[ch], at & 15, at >> 4);
        //    }
        _position = Vector(0, 0);

        _thread.initialize(this);
        _thread.start();
    }

    Vector size() const { return Vector(_outputSize.x, _outputSize.y*3); }

    void paint(const PaintHandle& paint)
    {
        class ConvertSRGBToDWord
        {
        public:
            DWord convert(SRGB srgb)
            {
                return (srgb.x<<16) + (srgb.y<<8) + srgb.z;
            }
        };

        class ConvertYToDWord
        {
        public:
            DWord convert(YIQ yiq) { return (60 + yiq.x)*0x10101; }
        };

        Vector s = size();
        int xMax = min(s.x, _outputSize.x);
        int ySize = s.y;
        Vector pos(0, 0);
        Vector cs(s.x, min(ySize, _outputSize.y));
        _digitalOutput.convert(subBitmap(pos, cs), ConvertSRGBToDWord());

        pos.y += cs.y;
        ySize -= _outputSize.y;
        if (ySize < 0)
            return;
        cs.y = min(ySize, _outputSize.y);
        _compositeOutput.subBitmap(Vector(0, 0), cs).
            convert(subBitmap(pos, cs), ConvertSRGBToDWord());

        pos.y += cs.y;
        ySize -= _outputSize.y;
        if (ySize < 0)
            return;
        cs.y = min(ySize, _outputSize.y);
        _compositeData.subBitmap(Vector(8, 0), cs).
            convert(subBitmap(pos, cs), ConvertYToDWord());

        Image::paint(paint);
    }

    void destroy()
    {
        _thread.end();

        PNGFileFormat png;
        _digitalOutput.save(png, _outputDigitalFile);
        _compositeOutput.save(png, _outputCompositeFile);
        Vector s = _dataOutput.size();
        _outputDataFile.save(_dataOutput.data(), s.x*s.y*sizeof(UInt16));
        Array<Byte> compositeData(_outputSize.x*_outputSize.y);
        for (int y = 0; y < _outputSize.y; ++y)
            for (int x = 0; x < _outputSize.x; ++x)
                compositeData[y*_outputSize.x + x] =
                    _compositeData.row(y)[x + 8].x;
        _outputNTSCFile.save(compositeData); // TODO: Add sync/burst/blank

        Array<Byte> rgbiData(_inputSize.x*_inputSize.y);
        for (int y = 0; y < _inputSize.y; ++y)
            for (int x = 0; x < _inputSize.x; ++x)
                rgbiData[y*_inputSize.x + x] = _digitalOutputRaw[Vector(x, y)];
        _outputDigitalRawFile.save(rgbiData);
    }

    double updatePixel(Vector p, SRGB srgb)
    {
        Colour rgb = ColourSpace::rgb().fromSrgb(srgb);
        _linearOutput[p] = rgb;
        Colour target = _linearInput[p];
        if (p.y > 0)
            target += _linearError[Vector(p.x, p.y - 1)]/2;
        if (p.x > 0)
            target += _linearError[Vector(p.x - 1, p.y)]/2;
        _linearError[p] = target - rgb;
        return (_colourSpace.fromRgb(rgb) - _colourSpace.fromRgb(target)).
            modulus2();
    }

    // Updates all the outputs and returns the total error for this data.
    double update(Vector inputPosition, UInt16 data)
    {
        int hdots = _hres ? 8 : 16;
        Vector dataPosition(inputPosition.x/hdots, inputPosition.y);
        Vector outputPosition = inputPosition + _offset;

        // Update data
        if (inputPosition.inside(_inputSize))
            _dataOutput[dataPosition] = data;

        // Update digital output and composite data
        UInt8 character = data & 0xff;
        UInt8 attribute = data >> 8;
        UInt8 bits = _patterns[character];
        int fg = attribute & 0xf;
        int bg = attribute >> 4;
        double error = 0;
        for (int x = 0; x < hdots; ++x) {
            Vector p = outputPosition + Vector(x, 0);

            int i = x;
            if (!_hres)
                i >>= 1;
            int colour = ((bits & (128 >> i)) != 0 ? fg : bg);
            SRGB srgb = _srgbPalette[colour];
            if (p.x >= 0) {
                if (inputPosition.inside(_inputSize))
                    _digitalOutputRaw[inputPosition + Vector(x, 0)] = colour;
                _digitalOutput[p] = srgb;
                if (!_composite) {
                    //Colour rgb = _linearPalette[colour];
                    //_linearOutput[p] = rgb;
                    //Colour target = _linearInput[p];
                    //if (p.y > 0)
                    //    target += _linearError[Vector(p.x, p.y - 1)]/2;
                    //if (p.x > 0)
                    //    target += _linearError[Vector(p.x - 1, p.y)]/2;
                    //_linearError[p] = target - rgb;
                    //error += (srgb - ColourSpace::rgb().toSrgb(target)).
                    //    modulus2();
                    error += updatePixel(p, srgb);
                }
            }

            // These give the colour burst patterns for the 8 colours ignoring
            // the intensity bit.
            static const int colorBurst[8][4] = {
                {0, 0, 0, 0}, /* Black */
                {2, 3, 4, 5}, /* Blue */
                {1, 0, 0, 1}, /* Green */
                {5, 2, 3, 4}, /* Cyan */
                {3, 4, 5, 2}, /* Red */
                {0, 1, 1, 0}, /* Magenta */
                {4, 5, 2, 3}, /* Yellow-burst */
                {1, 1, 1, 1}};/* White */

            // The values in the colorBurst array index into phaseLevels which
            // gives us the amount of time that the +CHROMA bit spends high
            // during that pixel.

            // TODO: use phase to compute phaseLevels entries 2 to 5
            static const int phase = 128;
            static const int phaseLevels[6] = {0, 256, -53, 128, 309, 128};

            // The following levels are computed as follows:
            // Using Falstad's circuit simulator applet
            // (http://www.falstad.com/circuit/) with the CGA composite output
            // stage and a 75 ohm load gives the following voltages:
            //   +CHROMA = 0,  +I = 0  0.416V  (colour 0)
            //   +CHROMA = 0,  +I = 1  0.709V  (colour 8)
            //   +CHROMA = 1,  +I = 0  1.160V  (colour 7)
            //   +CHROMA = 1,  +I = 1  1.460V  (colour 15)
            // Scaling these and adding an offset (equivalent to adjusting the
            // contrast and brightness respectively) such that colour 0 is at
            // the standard black level of IRE 7.5 and that colour 15 is at the
            // standard white level of IRE 100 gives:
            //   +CHROMA = 0,  +I = 0  IRE   7.5
            //   +CHROMA = 0,  +I = 1  IRE  33.5
            //   +CHROMA = 1,  +I = 0  IRE  73.4
            //   +CHROMA = 1,  +I = 1  IRE 100.0
            // Then we convert to sample levels using the standard formula:
            //   sample = 1.4*IRE + 60
            static const int sampleLevels[4] = {71, 107, 163, 200};

            // The sample grid should be aligned such that 00330033 is
            // green/magenta[/orange/aqua], not blue/cyan/red/yellow-burst. The
            // former aligns the samples with the pixels with the composite
            // samples
            // 0  0000  black
            // 1  0001  dark cyan
            // 2  0010  dark blue
            // 3  0011  aqua
            // 4  0100  dark red
            // 5  0101  grey
            // 6  0110  magenta
            // 7  0111  light blue
            // 8  1000  dark yellow-burst
            // 9  1001  green
            // A  1010  grey
            // B  1011  light cyan
            // C  1100  orange
            // D  1101  light yellow-burst
            // E  1110  light red
            // F  1111  white

            // So the order of bits is yellow-burst/red/blue/cyan

            int chroma = phaseLevels[colorBurst[colour & 7][x & 3]];
            chroma = 128 + static_cast<int>(
                static_cast<float>(chroma - 128) * 4 / (M_PI * sqrt(2.0f)));
            int intensity = (colour & 8) >> 3;
            int sampleLow = sampleLevels[intensity];
            int sampleHigh = sampleLevels[intensity + 2];
            int sample = (((sampleHigh - sampleLow)*chroma) >> 8) + sampleLow -
                60;
            _compositeData[p + Vector(8, 0)] = YIQ(sample,
                sample*_iqMultipliers[p.x & 3],
                sample*_iqMultipliers[(p.x + 3)&3]);
        }

        // Update composite output
        static int weightsHigh[14] =
            {1, 5, 12, 20, 27, 31, 32, 32, 31, 27, 20, 12, 5, 1};
        static int weightsLow[22] =
            {1, 5, 12, 20, 27, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 31,
            27, 20, 12, 5, 1};
        int* weights = _hres ? weightsHigh : weightsLow;
        YIQ* d = &_compositeData[outputPosition];
        if (outputPosition.x < 0)
            return error;
        for (int x = 0; x < hdots + 6; ++x) {
            Vector p = outputPosition + Vector(x, 0);

            // We use a low-pass Finite Impulse Response filter to
            // remove high frequencies (including the color carrier
            // frequency) from the signal. We could just keep a
            // 4-sample running average but that leads to sharp edges
            // in the resulting image.
            // The kernel of this FIR is [1, 4, 7, 8, 7, 4, 1]
            YIQ yiq =
                    d[x + 2] + d[x + 8]
                + ((d[x + 3] + d[x + 7])<<2)
                +  (d[x + 4] + d[x + 6])*7
                +  (d[x + 5]<<3);

            // Contrast for I and Q is handled by _iqMultipliers, along with
            // saturation. Brightness only affects Y.
            int y = yiq.x*_yContrast + _brightness;
            int i = yiq.y;
            int q = yiq.z;

            //static const double divisor = 16777216.0;
            //Colour rgb((y + 243*i + 160*q)/divisor,
            //    (y -  71*i - 164*q)/divisor, (y - 283*i + 443*q)/divisor);

            Colour srgb(
                _gamma[clamp(0, (y + 243*i + 160*q)>>16, 255)],
                _gamma[clamp(0, (y -  71*i - 164*q)>>16, 255)],
                _gamma[clamp(0, (y - 283*i + 443*q)>>16, 255)]);
            //SRGB srgb = ColourSpace::rgb().toSrgb(rgb);
            _compositeOutput[p] = srgb;

            if (_composite && p.x < _outputSize.x) {
                //_linearOutput[p] = rgb;
                //Colour target = _linearInput[p];
                //if (p.y > 0)
                //    target += _linearError[Vector(p.x, p.y - 1)]/2;
                //if (p.x > 0)
                //    target += _linearError[Vector(p.x - 1, p.y)]/2;
                //_linearError[p] = target - rgb;
                //error += (srgb - ColourSpace::rgb().toSrgb(target)).modulus2()*
                //    weights[x];
                error += updatePixel(p, srgb)*weights[x];
            }
        }

        return error;
    }

    void calculate()
    {
        Vector d(_position.x/(_hres ? 8 : 16), _position.y);
        UInt16 oldData = _dataOutput[d];

        UInt16 bestData = 0;
        double bestScore = 1e99;
        for (int pattern = 0; pattern < _patternCount; ++pattern) {
            UInt8 character = _characters[pattern];
            int bits = _patterns[character];
            for (int at = 0; at < 0x100; ++at) {
                int fg = at & 0x0f;
                int bg = at >> 4;
                if ((bits == 0 || bits == 0xff) != (fg == bg))
                    continue;

                UInt16 data = (at << 8) | character;
                double score = update(_position, data);
                if (score < bestScore) {
                    bestScore = score;
                    bestData = data;
                }
            }
        }
        if (bestData != oldData) {
            _changed = true;
            _dataOutput[d] = bestData;
        }
        update(_position, bestData);

        _position.x += (_hres ? 8 : 16);
        if (_position.x >= _inputSize.x) {
            ++_position.y;
            _position.x = 0;
            if (_position.y == _inputSize.y) {
                ++_iteration;
                if (!_changed || _iteration == _iterations)
                    _thread.finished();
                _position.y = 0;
            }
        }
    }
private:
    class CalcThread : public Thread
    {
    public:
        CalcThread() : _ending(false) { }

        void initialize(AttributeClashImage* image)
        {
            _image = image;
            doRestart();
        }

        void end()
        {
            _ending = true;
            join();
        }

        void finished()
        {
            _ending = true;
        }

    private:
        void doRestart()
        {
            _restartRequested = false;
        }

        void threadProc()
        {
            do {
                if (_restartRequested) {
                    doRestart();
                    _event.signal();
                }
                _image->calculate();
            } while (!_ending);
        }

        bool _restartRequested;
        bool _ending;
        Event _event;
        AttributeClashImage* _image;
    };

    int _iterations;
    bool _hres;
    bool _composite;
    Vector _inputSize;
    Vector _outputSize;
    Vector _offset;
    ColourSpace _colourSpace;

    Bitmap<Colour> _perceptualInput;
    Bitmap<Colour> _linearInput;
    Bitmap<Colour> _linearOutput;
    Bitmap<Colour> _linearError;
    Bitmap<SRGB> _digitalOutput;
    Bitmap<UInt8> _digitalOutputRaw;
    Bitmap<SRGB> _compositeOutput;
    Bitmap<UInt16> _dataOutput;
    Bitmap<YIQ> _compositeData;

    CalcThread _thread;
    SRGB _srgbPalette[0x10];
    Colour _linearPalette[0x10];
//    Colour _perceptualPalette[0x10];
    Vector _position;
    bool _changed;

    Array<UInt8> _patterns;
    Array<UInt8> _characters;
    int _patternCount;

    Array<int> _gamma;

    int _iqMultipliers[4];

    int _yContrast;
    int _brightness;

    int _iteration;

    File _outputNTSCFile;
    File _outputCompositeFile;
    File _outputDigitalFile;
    File _outputDataFile;
    File _outputDigitalRawFile;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            NullTerminatedWideString s
                ("Usage: attribute_clash <config file path>\n");
            MessageBox(NULL, s, L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        ConfigFile config;

        List<StructuredType::Member> vectorMembers;
        vectorMembers.add(StructuredType::Member("x", Type::integer));
        vectorMembers.add(StructuredType::Member("y", Type::integer));
        StructuredType vectorType("Vector", vectorMembers);
        config.addType(vectorType);

        List<EnumerationType::Value> colourSpaceMembers;
        colourSpaceMembers.add(
            EnumerationType::Value("srgb", ColourSpace::srgb()));
        colourSpaceMembers.add(
            EnumerationType::Value("rgb", ColourSpace::rgb()));
        colourSpaceMembers.add(
            EnumerationType::Value("xyz", ColourSpace::xyz()));
        colourSpaceMembers.add(
            EnumerationType::Value("luv", ColourSpace::luv()));
        colourSpaceMembers.add(
            EnumerationType::Value("lab", ColourSpace::lab()));
        EnumerationType colourSpaceType("colourSpace", colourSpaceMembers);
        config.addType(colourSpaceType);

        config.addOption("cgaRomFile", Type::string);
        config.addOption("inputPicture", Type::string);
        config.addOption("outputNTSC", Type::string);
        config.addOption("outputComposite", Type::string);
        config.addOption("outputDigital", Type::string);
        config.addOption<String>("outputDigitalRaw", Type::string, "");
        config.addOption("outputData", Type::string);
        config.addOption("compositeTarget", Type::boolean);
        config.addOption("hres", Type::boolean);
        config.addOption("inputSize", vectorType);
        config.addOption("outputSize", vectorType);
        config.addOption("offset", vectorType);
        config.addOption("overscanColour", Type::integer);
        config.addOption("iterations", Type::integer);
        config.addOption("colourSpace", colourSpaceType);
        config.load(_arguments[1]);

        AttributeClashImage image(&config);

        Window::Params wp(&_windows, L"Composite output");
        typedef RootWindow<Window> RootWindow;
        RootWindow::Params rwp(wp);
        typedef ImageWindow<RootWindow, AttributeClashImage> ImageWindow;
        ImageWindow::Params iwp(rwp, &image);
        typedef AnimatedWindow<ImageWindow> AnimatedWindow;
        AnimatedWindow::Params awp(iwp);
        AnimatedWindow window(awp);

        window.show(_nCmdShow);
        pumpMessages();
    }
};
