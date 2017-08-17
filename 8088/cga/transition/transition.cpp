#include "alfe/main.h"
#include "alfe/cga.h"
#include "alfe/config_file.h"

class Collect
{
public:
    Collect(AppendableArray<File>* collection) : _collection(collection) { }
    void operator()(const FileSystemObject& file)
    {
        File f(file);
        if (f.valid())
            _collection->append(f);
    }
    void operator()(const Directory& directory) { }
private:
    AppendableArray<File>* _collection;
};

class TransitionWindow : public RootWindow
{
public:
    TransitionWindow()
        : _wisdom(File("wisdom")), _output(&_data, &_sequencer, &_bitmap)
    {
        ConfigFile configFile;
        configFile.addDefaultOption("fadeHalfSteps", 5);
        configFile.addDefaultOption("smoothness", 40000000);
        configFile.addDefaultOption("fadeNumber", 6);
        configFile.addDefaultOption("wipeFrames", 203);
        configFile.addDefaultOption("fadeFrames", 120);
        configFile.addDefaultOption("wipeNumber", 2);

        configFile.load(File("default.config", true));

        _fadeHalfSteps = configFile.get<int>("fadeHalfSteps");
        int smoothness = configFile.get<int>("smoothness");
        _fadeNumber = configFile.get<int>("fadeNumber");
        _wipeFrames = configFile.get<int>("wipeFrames");
        _fadeFrames = configFile.get<int>("fadeFrames");
        _wipeNumber = configFile.get<int>("wipeNumber");


        _sequencer.setROM(File("5788005.u33"));

        _output.setConnector(0);          // RGBI
        _output.setScanlineProfile(0);    // rectangle
        _output.setHorizontalProfile(0);  // rectangle
        _output.setScanlineWidth(1);
        _output.setScanlineBleeding(2);   // symmetrical
        _output.setHorizontalBleeding(2); // symmetrical
        _output.setZoom(2);
        _output.setHorizontalRollOff(0);
        _output.setHorizontalLobes(4);
        _output.setVerticalRollOff(0);
        _output.setVerticalLobes(4);
        _output.setSubPixelSeparation(1);
        _output.setPhosphor(0);           // colour
        _output.setMask(0);
        _output.setMaskSize(0);
        _output.setAspectRatio(5.0/6.0);
        _output.setOverscan(0);
        _output.setCombFilter(0);         // no filter
        _output.setHue(0);
        _output.setSaturation(100);
        _output.setContrast(100);
        _output.setBrightness(0);
        _output.setShowClipping(false);
        _output.setChromaBandwidth(1);
        _output.setLumaBandwidth(1);
        _output.setRollOff(0);
        _output.setLobes(1.5);
        _output.setPhase(1);

        _regs = -CGAData::registerLogCharactersPerBank;
        _cgaBytes.allocate(0x4000 + _regs);
        _vram = &_cgaBytes[_regs];
        _vram[CGAData::registerLogCharactersPerBank] = 12;
        _vram[CGAData::registerScanlinesRepeat] = 1;
        _vram[CGAData::registerHorizontalTotalHigh] = 0;
        _vram[CGAData::registerHorizontalDisplayedHigh] = 0;
        _vram[CGAData::registerHorizontalSyncPositionHigh] = 0;
        _vram[CGAData::registerVerticalTotalHigh] = 0;
        _vram[CGAData::registerVerticalDisplayedHigh] = 0;
        _vram[CGAData::registerVerticalSyncPositionHigh] = 0;
        _vram[CGAData::registerMode] = 9;
        _vram[CGAData::registerPalette] = 0;
        _vram[CGAData::registerHorizontalTotal] = 114 - 1;
        _vram[CGAData::registerHorizontalDisplayed] = 80;
        _vram[CGAData::registerHorizontalSyncPosition] = 90;
        _vram[CGAData::registerHorizontalSyncWidth] = 10; // 16;
        _vram[CGAData::registerVerticalTotal] = 128 - 1;
        _vram[CGAData::registerVerticalTotalAdjust] = 6;
        _vram[CGAData::registerVerticalDisplayed] = 100;
        _vram[CGAData::registerVerticalSyncPosition] = 112;
        _vram[CGAData::registerInterlaceMode] = 2;
        _vram[CGAData::registerMaximumScanline] = 1;
        _vram[CGAData::registerCursorStart] = 6;
        _vram[CGAData::registerCursorEnd] = 7;
        _vram[CGAData::registerStartAddressHigh] = 0;
        _vram[CGAData::registerStartAddressLow] = 0;
        _vram[CGAData::registerCursorAddressHigh] = 0;
        _vram[CGAData::registerCursorAddressLow] = 0;
        _data.setTotals(238944, 910, 238875);
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);

        _outputSize = _output.requiredSize();

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(60);

        AppendableArray<File> imageFiles;
        applyToWildcard(Collect(&imageFiles), "*.dat", true,
            CurrentDirectory());
        _images.allocate(imageFiles.count());
        for (int i = 0; i < imageFiles.count(); ++i)
            _images[i] = imageFiles[i].contents();

        for (int i = 0; i < 16000; ++i)
            _vram[i] = 0;

        srand(time(NULL));

        _wipes = 3;
        _gradientWipe.allocate(8000 * _wipes);
        _wipeSequence.allocate(8000 * _wipes);
        for (int i = 0; i < 8000; ++i) {
            _gradientWipe[i] = (i*256)/8000;
            _wipeSequence[i] = i;
        }
        for (int i = 0; i < 8000; ++i) {
            _gradientWipe[i + 8000] = _gradientWipe[i];
            _wipeSequence[i + 8000] = _wipeSequence[i];
        }
        for (int i = 0; i < 8000 - 3; ++i) {
            int j = (rand() % (8000 - i)) + i;
            Byte t = _gradientWipe[i + 8000];
            _gradientWipe[i + 8000] = _gradientWipe[j + 8000];
            _gradientWipe[j + 8000] = t;
            Word w = _wipeSequence[i + 8000];
            _wipeSequence[i + 8000] = _wipeSequence[j + 8000];
            _wipeSequence[j + 8000] = w;
        }
        for (int i = 0; i < 8000; ++i) {
            _gradientWipe[i + 16000] = _gradientWipe[i + 8000];
            _wipeSequence[i + 16000] = _wipeSequence[i + 8000];
        }
#ifndef _DEBUG
        for (int i = 0; i < smoothness; ++i) {
            int p1 = rand() % 8000;
            int p2 = rand() % 8000;
            int metricUnswapped = metric(p1, p2);
            swap(p1, p2);
            int metricSwapped = metric(p1, p2);
            if (metricSwapped > metricUnswapped)
                swap(p1, p2);
            else {
                Word t = _wipeSequence[p1 + 16000];
                _wipeSequence[p1 + 16000] = _wipeSequence[p2 + 16000];
                _wipeSequence[p2 + 16000] = t;
            }
        }
#endif
        for (int i = 0; i < _wipes; ++i)
            invertWipeSequence(i*8000);

        _fadeHalfSteps = 5;
        _fadeRGBI.allocate(16*_fadeHalfSteps*16);
        for (int midPoint = 0; midPoint < 16; ++midPoint) {
            SRGB srgbMid = rgbiPalette[midPoint];
            Colour mid = _linearizer.linear(srgbMid);
            for (int i = 0; i < 16; ++i) {
                SRGB srgb = rgbiPalette[i];
                Colour c = _linearizer.linear(srgb);
                for (int j = 0; j < _fadeHalfSteps; ++j) {
                    Colour c1 = c * static_cast<float>(j)/(_fadeHalfSteps - 1) + mid*static_cast<float>(_fadeHalfSteps - (j + 1))/(_fadeHalfSteps - 1);
                    _fadeRGBI[j*16 + i + midPoint*16*_fadeHalfSteps] = closestRGBI(_linearizer.srgb(c1));
                }
            }
        }
        _fadeRGBILuminance.allocate(16*_fadeHalfSteps*16);
        for (int midPoint = 0; midPoint < 16; ++midPoint) {
            SRGB srgbMid = rgbiPalette[midPoint];
            Colour mid = _linearizer.linear(srgbMid);
            for (int i = 0; i < 16; ++i) {
                SRGB srgb = rgbiPalette[i];
                Colour c = _linearizer.linear(srgb);
                for (int j = 0; j < _fadeHalfSteps; ++j) {
                    Colour c1 = c * static_cast<float>(j)/(_fadeHalfSteps - 1) + mid*static_cast<float>(_fadeHalfSteps - (j + 1))/(_fadeHalfSteps - 1);
                    SRGB srgbTarget = _linearizer.srgb(c1);
                    float lumTarget = 0.299f*srgbTarget.x + 0.587f*srgbTarget.y + 0.114f*srgbTarget.z;
                    int bestColour = 0;
                    float bestMetric = 1e30;
                    for (int k = 0; k < 16; ++k) {
                        SRGB srgbTrial = rgbiPalette[k];
                        float lumTrial = 0.299f*srgbTrial.x + 0.587f*srgbTrial.y + 0.114f*srgbTrial.z;
                        float metric = lumTrial - lumTarget;
                        metric *= metric;
                        if (metric < bestMetric) {
                            bestMetric = metric;
                            bestColour = k;
                        }
                    }
                    _fadeRGBILuminance[j*16 + i + midPoint*16*_fadeHalfSteps] = bestColour;
                }
            }
        }
        _fadeRGBINoRepeat.allocate(16*_fadeHalfSteps*16);
        for (int midPoint = 0; midPoint < 16; ++midPoint) {
            SRGB srgbMid = rgbiPalette[midPoint];
            Colour mid = _linearizer.linear(srgbMid);
            int lumMid = 299*srgbMid.x + 587*srgbMid.y + 114*srgbMid.z;
            for (int i = 0; i < 16; ++i) {
                SRGB srgb = rgbiPalette[i];
                Colour c = _linearizer.linear(srgb);
                int lumStart = 299*srgb.x + 587*srgb.y + 114*srgb.z;
                for (int j = _fadeHalfSteps - 1; j >= 0; --j) {
                    Colour c1 = c * static_cast<float>(j)/(_fadeHalfSteps - 1) + mid*static_cast<float>(_fadeHalfSteps - (j + 1))/(_fadeHalfSteps - 1);
                    SRGB srgbTarget = _linearizer.srgb(c1);
                    int lumTarget = 299*srgbTarget.x + 587*srgbTarget.y + 114*srgbTarget.z;
                    int bestColour = 0;
                    float bestMetric = 1e30;
                    for (int k = 0; k < 16; ++k) {
                        bool repeat = false;
                        if (k != 0) {
                            int l = j + 1;
                            for (; l < _fadeHalfSteps; ++l)
                                if (_fadeRGBINoRepeat[l*16 + i] == k)
                                    break;
                            if (l < _fadeHalfSteps)
                                repeat = true;
                        }

                        SRGB srgbTrial = rgbiPalette[k];
                        float metric = delta2(srgbTrial, srgbTarget);
                        int lumTrial = 299*srgbTrial.x + 587*srgbTrial.y + 114*srgbTrial.z;
                        if (lumMid < lumStart) {
                            if (lumTrial > lumTarget)
                                continue;
                        }
                        if (lumMid > lumStart) {
                            if (lumTrial < lumTarget)
                                continue;
                        }
                        if (midPoint == i && k != i)
                            continue;
                        if (repeat)
                            metric += 1e6;
                        if (metric < bestMetric) {
                            bestMetric = metric;
                            bestColour = k;
                        }
                    }
                    _fadeRGBINoRepeat[j*16 + i + midPoint*16*_fadeHalfSteps] = bestColour;
                }
            }
        }
        fillCube(&_rgbCube, &_linearizer);
        _noGamma.setGamma(1.0f);
        fillCube(&_srgbCube, &_noGamma);

        String asmOutput;
        asmOutput += "fadeRGBI:\n";
        //for (int i = 0; i < 16*_fade)

        _newImage = _images[0];
        for (int i = 0; i < 16000; ++i)
            _vram[i] = _newImage[i];

        initTransition();
    }
    ~TransitionWindow() { _output.join(); }
    void create()
    {
        setText("CGA transitions");
        setInnerSize(_outputSize);
        _bitmap.setTopLeft(Vector(0, 0));
        _bitmap.setInnerSize(_outputSize);
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
        _output.restart();
        _animated.restart();
        ++_transitionFrame;
        _spaceStartFrame += _frameIncrement;
        _spaceStartFracFrame += _frameFracIncrement;
        if (_spaceStartFracFrame >= _denominator) {
            _spaceStartFracFrame -= _denominator;
            ++_spaceStartFrame;
        }
        _spaceEndFrame += _frameIncrement;
        _spaceEndFracFrame += _frameFracIncrement;
        if (_spaceEndFracFrame >= _denominator) {
            _spaceEndFracFrame -= _denominator;
            ++_spaceEndFrame;
        }

        if (_fadeFrames > _fadeSteps) {
            // Gaps in the wipe sequence
            int spaceStart = _spaceStartFrame;
            int spaceEnd = _spaceEndFrame;
            int spaceStartFrac = _spaceStartFracFrame;
            int spaceEndFrac = _spaceEndFracFrame;
            for (int i = 1; i < _fadeSteps; ++i) {
                spaceStart += _spaceStepIncrement;
                spaceStartFrac += _spaceStepFracIncrement;
                if (spaceStartFrac < 0) {
                    spaceStartFrac += _denominator;
                    --spaceStart;
                }
                spaceEnd += _spaceStepIncrement;
                spaceEndFrac += _spaceStepFracIncrement;
                if (spaceEndFrac < 0) {
                    spaceEndFrac += _denominator;
                    --spaceEnd;
                }

                //int spaceStart2 = (((_transitionFrame - 1)*_fadeSteps - (i - 1)*_fadeFrames)*8000)/(_wipeFrames*_fadeSteps);
                //int spaceEnd2 = ((_transitionFrame*_fadeSteps - (i - 1)*_fadeFrames)*8000)/(_wipeFrames*_fadeSteps);
                //if (spaceStart != spaceStart2 || spaceEnd != spaceEnd2)
                //    printf("Error\n");

                int start = spaceStart;
                int end = spaceEnd;
                if (start < 0)
                    start = 0;
                if (end > 8000)
                    end = 8000;
                for (int s = start; s < end; ++s) {
                    int p = _wipeSequence[s + 8000*_wipeNumber];
                    Word o = _oldImage[p*2] + (_oldImage[p*2 + 1] << 8);
                    Word n = _newImage[p*2] + (_newImage[p*2 + 1] << 8);
                    Word r = fade(o, n, i, p);
                    _vram[p*2] = r & 0xff;
                    _vram[p*2 + 1] = r >> 8;
                }
            }
        }
        else {
            // Gaps in the fade sequence

            int spaceStart = _spaceStartFrame;
            int spaceEnd = _spaceEndFrame;

            //int spaceStart2 = (((_transitionFrame - 1)*_fadeSteps - (_fadeSteps - 2)*_fadeFrames)*8000)/(_wipeFrames*_fadeSteps);
            //int spaceEnd2 = (_transitionFrame*8000)/_wipeFrames;
            //if (_spaceStartFracFrame != 0 && _spaceStartFrame < 0)
            //    --spaceStart2;
            //if (_spaceEndFracFrame != 0 && _spaceEndFrame < 0)
            //    --spaceEnd2;
            //if (spaceStart != spaceStart2 || spaceEnd != spaceEnd2)
            //    printf("Error\n");

            if (spaceStart < 0)
                spaceStart = 0;
            if (spaceEnd > 8000)
                spaceEnd = 8000;

            int denominator = _fadeFrames*8000;
            int step = ((_transitionFrame*8000 - spaceStart*_wipeFrames)*_fadeSteps)/denominator;
            int stepFrac = ((_transitionFrame*8000 - spaceStart*_wipeFrames)*_fadeSteps)%denominator;
            if (stepFrac < 0) {
                printf("Correcting\n");
                --step;
                stepFrac += denominator;
            }
            int increment = -((_wipeFrames*_fadeSteps)/denominator);
            int incrementFrac = -((_wipeFrames*_fadeSteps)%denominator);

            for (int s = spaceStart; s < spaceEnd; ++s) {
                int p = _wipeSequence[s + 8000*_wipeNumber];
                Word o = _oldImage[p*2] + (_oldImage[p*2 + 1] << 8);
                Word n = _newImage[p*2] + (_newImage[p*2 + 1] << 8);

                int step2 = ((_transitionFrame*8000 - s*_wipeFrames)*_fadeSteps)/denominator;
                if (step2 != step)
                    printf("Error\n");

                int clippedStep = step;
                if (clippedStep <= 0)
                    clippedStep = 1;
                if (clippedStep >= _fadeSteps)
                    clippedStep = _fadeSteps - 1;

                Word r = fade(o, n, clippedStep, p);
                _vram[p*2] = r & 0xff;
                _vram[p*2 + 1] = r >> 8;

                step += increment;
                stepFrac += incrementFrac;
                if (stepFrac < 0) {
                    stepFrac += denominator;
                    --step;
                }
            }
        }

        if (_transitionFrame == _wipeFrames + _fadeFrames)
            initTransition();
    }
private:
    void initTransition()
    {
        _fadeSteps = _fadeHalfSteps*2;
        if (_fadeNumber >= 6)
            _fadeSteps = 10;

        _denominator = _wipeFrames*_fadeSteps;
        if (_fadeFrames > _fadeSteps) {
            _spaceStartInitial = ((_fadeFrames - _fadeSteps)*8000)/_denominator;
            _spaceStartFracInitial = ((_fadeFrames - _fadeSteps)*8000)%_denominator;
            _spaceEndInitial = (_fadeFrames*8000)/_denominator;
            _spaceEndFracInitial = (_fadeFrames*8000)%_denominator;
        }
        else {
            _spaceStartInitial = -(((_fadeSteps + (_fadeSteps - 2)*_fadeFrames)*8000)/_denominator);
            _spaceStartFracInitial = -(((_fadeSteps + (_fadeSteps - 2)*_fadeFrames)*8000)%_denominator);
            if (_spaceStartFracInitial < 0) {
                --_spaceStartInitial;
                _spaceStartFracInitial += _denominator;
            }
            _spaceEndInitial = 0;
            _spaceEndFracInitial = 0;
        }
        // To compute the following in asm, we can negate the dividend instead
        // of the quotient and remainder.
        _spaceStepIncrement = -((_fadeFrames*8000)/_denominator);
        _spaceStepFracIncrement = -((_fadeFrames*8000)%_denominator);
        _frameIncrement = (_fadeSteps*8000)/_denominator;
        _frameFracIncrement = (_fadeSteps*8000)%_denominator;


        int lastImage = _imageIndex;
        _oldImage = _newImage;
        _imageIndex = rand() % (_images.count() - 1);
        if (_imageIndex >= lastImage)
            ++_imageIndex;
        _newImage = _images[_imageIndex];
        _transitionFrame = 0;

        _spaceStartFrame = _spaceStartInitial;
        _spaceEndFrame = _spaceEndInitial;
        _spaceStartFracFrame = _spaceStartFracInitial;
        _spaceEndFracFrame = _spaceEndFracInitial;

        _midPoints.ensure(8000);
        for (int i = 0; i < 8000; ++i) {
            Word o = _oldImage[i*2] + (_oldImage[i*2 + 1] << 8);
            Word n = _newImage[i*2] + (_newImage[i*2 + 1] << 8);
            Colour c = (averageColour(o) + averageColour(n))/2;
            _midPoints[i] = closestRGBI(_linearizer.srgb(c));
        }
    }

    void swap(int i, int j)
    {
        Byte t = _gradientWipe[i + 16000];
        _gradientWipe[i + 16000] = _gradientWipe[j + 16000];
        _gradientWipe[j + 16000] = t;
    }
    int metric(int i, int j)
    {
        return metricForPoint(i) + metricForPoint(j);
    }
    int metricForPoint(int i)
    {
        int x = i % 80;
        int y = i / 80;
        int h = secondDerivative(x, y, (x + 1)%80, y, (x + 79)%80, y)*3;
        int v = secondDerivative(x, y, x, (y + 1)%100, x, (y + 99)%100)*5;
        return h*h + v*v;
    }
    int secondDerivative(int x1, int y1, int x0, int y0, int x2, int y2)
    {
        return getByte(x2, y2) + getByte(x0, y0) - 2*getByte(x1, y1);
    }
    int getByte(int x, int y) { return _gradientWipe[y*80 + x + 16000]; }

    Word fade(Word start, Word end, int transition, int p)
    {
        int fadeNumber = _fadeNumber;
        int midPoint = 0;
        if (_fadeNumber >= 3 && _fadeNumber < 6) {
            fadeNumber -= 3;
            midPoint = _midPoints[p];
        }
        Array<Byte>* fade;
        switch (fadeNumber) {
            case 0:
                fade = &_fadeRGBI;
                break;
            case 1:
                fade = &_fadeRGBILuminance;
                break;
            case 2:
                fade = &_fadeRGBINoRepeat;
                break;
        }
        if (fadeNumber < 6) {
            Byte ch;
            Byte at;
            int tn;
            if (transition > _fadeHalfSteps) {
                ch = end & 0xff;
                at = end >> 8;
                tn = transition - _fadeHalfSteps;
            }
            else {
                ch = start & 0xff;
                at = start >> 8;
                tn = _fadeHalfSteps - transition;
            }
            Byte fg = at & 0xf;
            Byte bg = at >> 4;
            fg = (*fade)[tn*16 + fg + midPoint*16*_fadeHalfSteps];
            bg = (*fade)[tn*16 + bg + midPoint*16*_fadeHalfSteps];
            at = fg + (bg << 4);
            return ch + (at << 8);
        }
        if (transition == 9)
            return end;
        Array<Word>* cube = &_rgbCube;
        Linearizer* linearizer = &_linearizer;
        if (fadeNumber == 7) {
            cube = &_srgbCube;
            linearizer = &_noGamma;
        }
        SRGB srgbOld = _linearizer.srgb(averageColour(start));
        SRGB srgbNew = _linearizer.srgb(averageColour(end));
        Colour co = linearizer->linear(srgbOld);
        Colour cn = linearizer->linear(srgbNew);
        Colour c = co * (static_cast<float>(8 - transition)/7.0f) +
            cn * static_cast<float>((transition - 1)/7.0f);
        c *= 7.0f;
        int r = clamp(0, static_cast<int>(c.x + 0.5), 7);
        int g = clamp(0, static_cast<int>(c.y + 0.5), 7);
        int b = clamp(0, static_cast<int>(c.z + 0.5), 7);
        return (*cube)[r + g*8 + b*64];
    }

    void invertWipeSequence(int offset)
    {
        Array<Word> temp(8000);
        for (int i = 0; i < 8000; ++i)
            temp[i] = _wipeSequence[i + offset];
        for (int i = 0; i < 8000; ++i) {
            int j;
            for (j = 0; j < 8000; ++j)
                if (temp[j] == i)
                    break;
            _wipeSequence[i + offset] = j;
        }
    }

    float delta2(SRGB c1, SRGB c2)
    {
        float dr = static_cast<float>(c1.x - c2.x);
        float dg = static_cast<float>(c1.y - c2.y);
        float db = static_cast<float>(c1.z - c2.z);
        // Fast colour distance metric from
        // http://www.compuphase.com/cmetric.htm .
        float mr = (c1.x + c2.x)/512.0f;
        return 4.0f*dg*dg + (2.0f + mr)*dr*dr + (3.0f - mr)*db*db;
    }
    int closestRGBI(SRGB srgbTarget)
    {
        int bestColour = 0;
        float bestMetric = 1e30;
        for (int k = 0; k < 16; ++k) {
            float metric = delta2(rgbiPalette[k], srgbTarget);
            if (metric < bestMetric) {
                bestMetric = metric;
                bestColour = k;
            }
        }
        return bestColour;
    }
    Colour averageColour(Word w)
    {
        Colour c(0, 0, 0);
        for (int y = 0; y < 2; ++y) {
            UInt64 pixels = _sequencer.process(w, 9, 0, y, false, 0);
            for (int x = 0; x < 8; ++x)
                c += _linearizer.linear(rgbiPalette[(pixels >> (x*4)) & 0x0f]);
        }
        return c/16;
    }
    void fillCube(Array<Word>* cube, Linearizer* linearizer)
    {
        cube->allocate(8*8*8);
        for (int slot = 0; slot < 8*8*8; ++slot) {
            Word bestWord = 0;
            float bestMetric = 1e30;
            int r = slot & 7;
            int g = (slot >> 3) & 7;
            int b = (slot >> 6) & 7;
            Colour target(r, g, b);
            target /= 7;
            SRGB srgbTarget = linearizer->srgb(target);
            for (int w = 0; w < 0x10000; ++w) {
                int ch = w & 0xff;
                if (ch != 0xb1 && ch != 0xb0 && ch != 0x1d && ch != 0x7e && ch != 0x7f)
                    continue;
                float metric = delta2(_linearizer.srgb(averageColour(w)), srgbTarget);
                if (metric < bestMetric) {
                    bestWord = w;
                    bestMetric = metric;
                }
            }
            (*cube)[slot] = bestWord;
        }
    }

    Linearizer _linearizer;
    Linearizer _noGamma;
    FFTWWisdom<float> _wisdom;
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput _output;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    Vector _outputSize;
    Array<Byte> _cgaBytes;
    Byte* _vram;
    int _regs;

    Array<String> _images;
    String _oldImage;
    String _newImage;
    int _imageIndex;

    int _fadeSteps;
    int _fadeFrames;
    int _wipeFrames;
    int _transitionFrame;
    int _wipeNumber;
    int _fadeNumber;

    int _denominator;
    int _spaceStartFrame;
    int _spaceEndFrame;
    int _spaceStartFracFrame;
    int _spaceEndFracFrame;
    int _spaceStepIncrement;
    int _spaceStepFracIncrement;
    int _frameIncrement;
    int _frameFracIncrement;
    int _spaceStartInitial;
    int _spaceEndInitial;
    int _spaceStartFracInitial;
    int _spaceEndFracInitial;

    int _wipes;
    Array<Byte> _gradientWipe;
    Array<Word> _wipeSequence;

    int _fadeHalfSteps;
    Array<Byte> _fadeRGBI;
    Array<Byte> _fadeRGBILuminance;
    Array<Byte> _fadeRGBINoRepeat;
    Array<Byte> _midPoints;
    Array<Word> _rgbCube;
    Array<Word> _srgbCube;
};

class Program : public WindowProgram<TransitionWindow>
{
};
