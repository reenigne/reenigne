#include "alfe/main.h"
#include "alfe/complex.h"
#include "alfe/colour_space.h"
#include "fftw3.h"

float preamp[] = {
  50.0, //  131.717972,
  50.0, //  99.066330,
  50.0, //  57.235191,
  50.0, //  31.985214,
  50.0, //  42.455242,
  50.0, //  54.362297,
  50.0, //  53.158817,
  50.0, //  44.854256,
  50.0, //  38.591202,
  50.0, //  41.439701,
  50.0, //  47.131847,
  50.0, //    50.507721,
  50.0, //    47.894325,
  50.0, //    43.842155,
  50.0, //    44.105804,
  51.250809,
  55.293766,
  52.829926,
  48.648155,
  53.144211,
  60.727947,
  62.886639,
  59.236660,
  59.417824,
  65.317169,
  70.915886,
  71.066559,
  70.171486,
  71.635559,
  76.507912,
  79.595840,
  80.578156,
  80.147758,
  81.534538,
  84.577538,
  88.057564,
  88.007866,
  87.471085,
  88.784683,
  93.012978,
  93.933510,
  92.771317,
  92.870209,
  96.410736,
  98.649315,
  97.632446,
  95.400223,
  98.003036,
  100.857582,
  100.781311,
  98.689583,
  100.130798,
  102.200394,
  102.702477,
  101.589912,
  103.132942,
  105.029419,
  105.195679,
  104.136658,
  105.785393,
  108.758194,
  109.016121,
  107.399094,
  108.403770,
  110.286720,
  111.278099,
  110.423889,
  111.651085,
  113.737419,
  113.947441,
  113.132278,
  114.636902,
  117.386353,
  118.569450,
  116.435684,
  116.505341,
  118.826492,
  120.834862,
  119.372063,
  117.781837,
  118.840759,
  120.393631,
  119.886414,
  119.425652,
  118.937302,
  119.470383,
  119.114441,
  119.132561,
  118.913589,
  119.520508,
  119.259209,
  119.819557,
  119.517570,
  118.784134,
  118.154175,
  119.327705,
  118.075691,
  116.252640,
  114.706184,
  115.571106,
  117.492737,
  117.208977,
  116.983711,
  125.138832,
  128.440567,
  131.280502,
  130.454224,
  130.403000,
  130.925812,
  132.234375,
  132.378281,
  133.168411,
  133.402359,
  133.105240,
  132.756943,
  133.854874,
  134.704895,
  134.662704,
  134.000244,
  135.714157,
  137.518478,
  137.914352,
  137.566513,
  138.444214,
  140.500549,
  140.843521,
  138.504379,
  138.448563,
  139.768539,
  141.214066,
  139.467224,
  136.000351,
  136.020477,
  138.070114,
  138.947449,
  139.354599,
  138.483765,
  138.798721,
  139.347412,
  140.662689,
  139.692627,
  138.783386,
  138.318176,
  139.321045,
  140.523178,
  138.861862,
  137.958008,
  138.954071,
  140.904022,
  140.341705,
  138.652130,
  138.883118,
  139.999985,
  141.033356,
  139.607620,
  137.664337,
  138.053360,
  138.895218,
  139.113297,
  137.314926,
  134.596329,
  134.654266,
  135.920090,
  135.505508,
  133.371048,
  130.916534,
  130.823639,
  130.787933,
  129.587631,
  126.507828,
  125.233131,
  124.430168,
  124.094849,
  121.953766,
  120.755920,
  119.105507,
  118.158447,
  115.818001,
  115.033089,
  112.617516,
  111.608864,
  110.166626,
  109.140785,
  106.590744,
  105.259476,
  103.301109,
  101.927650,
  99.601349,
  96.281143,
  92.747803,
  91.090240,
  89.640907,
  86.257065,
  83.085129,
  81.199257,
  79.358566,
  76.602379,
  72.350037,
  68.928642,
  66.846069,
  64.539261,
  60.905903,
  57.590942,
  54.329788,
  50.727127,
  47.210518};

class VCRDecoder
{
public:
    VCRDecoder()
    {
        FileStream inputStream = File("U:\\c2.raw", true).openRead();
        //FileStream inputStream = File("U:\\captured.bin", true).openRead();
        int nn = inputStream.size();

        FileStream h = File("U:\\vcr_decoded.bin", true).openWrite();

        int samplesPerFrame = 1824*253;
        Array<Byte> input(samplesPerFrame);
        _fftData.allocate(2048);
        _chromaData.allocate(2048);
        _burstData.allocate(91);
        _forward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&_fftData[0]), reinterpret_cast<fftwf_complex*>(&_fftData[0]), -1, FFTW_MEASURE);
        _backward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&_fftData[0]), reinterpret_cast<fftwf_complex*>(&_fftData[0]), 1, FFTW_MEASURE);
        _chromaForward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&_chromaData[0]), reinterpret_cast<fftwf_complex*>(&_chromaData[0]), -1, FFTW_MEASURE);
        _chromaBackward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&_chromaData[0]), reinterpret_cast<fftwf_complex*>(&_chromaData[0]), 1, FFTW_MEASURE);
        Complex<float> localOscillatorFrequency = unit(4.4f*11/315);
        Complex<float> chromaOscillatorFrequency = unit(2.0f/91);  // 0.629 == 315/11 / 2 / 4 * 16 / 91      0.629f*11/315 == 16/8/91

        //static const float cutoff = 3.0f*2048*11/315;  // 3.3f
        //static const int cutoff = 208;
        static const int cutoff = static_cast<int>(2.2f*2048*11/315);
        static const int chromaCutoff = static_cast<int>(2048.0*2.0/91)*0.75;  // *0.5;

        _outputb0.allocate(2048);
        _outputb1.allocate(2048);

        Array<Complex<float>> amplitudes(253);
        for (int y = 0; y < 253; ++y)
            amplitudes[y] = 0;

        Array<Vector3<float>> frameCache(3*2048*253);
        for (int i = 0; i < 3*2048*253; ++i)
            frameCache[i] = Vector3<float>(0, 0, 0);

        int frame = 0;
        Complex<float> burstAcc = 0;
        Complex<float> framePhase = unit((90 - 33)/360.0f);
    }

    void decodeField()
    {
        while (nn > 0) {
            inputStream.read(&input[0], min(nn, samplesPerFrame));
            nn -= samplesPerFrame;

            Complex<float> linePhase;
            if ((frame & 1) == 0)
                linePhase = framePhase * Complex<float>(0, 1);
            else
                linePhase = framePhase;

            int p = 1824*2 + 1820;
            int x;
            int lows = 0;
            for (x = 0; x < 1820; ++x) {
                if (input[p] < 22)
                    ++lows;
                if (lows > 20)
                    break;
                ++p;
                if (p >= samplesPerFrame - 1)
                    break;
            }
            p += 133 - 20;
            p -= 1820 - 10;
            if (x < 1000)
                p += 1820;

            for (int y = 3; y < 253; ++y) {
                float pFrac;
                p += 1820 - 10;
                for (x = 0; x < 20; ++x) {
                    if (input[p] >= 22)
                        break;
                    ++p;
                    if (p >= samplesPerFrame - 1)
                        break;
                }
                int d = input[p] - input[p - 1];
                if (d == 0)
                    pFrac = 0;
                else
                    pFrac = static_cast<float>(input[p] - 22)/d;
                // 22 position is input[p - pFrac]

                for (x = 0; x < 1820; ++x) {
                    if (p + x - 64 >= samplesPerFrame)
                        break;
                    _fftData[x] = input[p + x - 64];
                }
                for (; x < 2048; ++x)
                    _fftData[x] = 0;

                // TODO: shift by pFrac

                Complex<float> burst = 0;
                int nBurst = (y & 1 ? 46 : 45);
                for (int x = 0; x < nBurst; ++x)
                    burst += _fftData[x + 76]*unit(x*2.0f/91);
                burst /= nBurst;
                burstAcc = burstAcc*0.9f + burst*0.1f;

                Complex<float> localOscillator = 1;
                Complex<float> chromaOscillator = 1;

                float total = 0;
                for (int i = 0; i < 1666; ++i) {
                    float v = _fftData[i + 82].x;
                    _chromaData[i + 82] = v * chromaOscillator;
                    total += v;
                }
                float mean = total/1666;
                for (int i = 0; i < 160; ++i)
                    _chromaData[i] = mean;
                for (int i = 1666+82; i < 2048; ++i)
                    _chromaData[i] = mean;


                for (int i = 0; i < 2048; ++i) {
                    _chromaData[i] *= chromaOscillator;
                    _fftData[i] *= localOscillator;
                    localOscillator *= localOscillatorFrequency;
                    chromaOscillator *= chromaOscillatorFrequency;
                }

                fftwf_execute(_forward);
                fftwf_execute(_chromaForward);
                for (int x = cutoff; x < 2048 - cutoff; ++x)
                    _fftData[x] = 0;
                for (int x = chromaCutoff; x < 2048 - chromaCutoff; ++x)
                    _chromaData[x] = 0;
                fftwf_execute(_chromaBackward);

                fftwf_execute(_backward);
                for (int x = 0; x < 2047; ++x) {
                    float deltaPhase = (_fftData[x + 1]/fftData[x]).argument();
                    // 0  MHz                     <=> 0 deltaPhase
                    // 3.4MHz <=> -40   IRE                             1 MHz
                    //              7.5 IRE <=>   0
                    // 4.4MHz <=> 100   IRE <=> 255                     0 MHz
                    // 315/11MHz                  <=> tau deltaPhase
                    float mhz = deltaPhase*315/(tau*11);
                    float ire = (1.0f - mhz)*140 - 40;
                    fftData[x] = (ire - 7.5f)*255.0f/(100 - 7.5);
                }

                //fftwf_execute(_forward);
                //for (int x = 0; x < 206; ++x) {
                //    fftData[x + 2] *= 50.0f/(preamp[x]*2048.0f);
                //    fftData[2048 - (x + 2)] *= 50.0f/(preamp[x]*2048.0f);
                //}
                //fftwf_execute(_backward);

                Complex<float> chromaPhase = burstAcc.conjugate()*linePhase/burstAcc.modulus();

                for (int x = 0; x < 2048; ++x) {
                    //outputb[x] = byteClamp(fftData[x].x / 2048.0f);
                    float yy = _fftData[x].x;
                    Complex<float> c = _chromaData[x]*chromaPhase / 20.48f; //  / 100.0f;
                    float ii = c.x;
                    float qq = c.y;
                    frameCache[(((frame + 2)%3)*253 + y)*2048 + x] =
                        Vector3<float>(
                            yy + 0.9563*c.x + 0.6210*c.y,
                            yy - 0.2721*c.x - 0.6474*c.y,
                            yy - 1.1069*c.x + 1.7046*c.y);
                }
                if ((frame & 1) != 0)
                    linePhase *= Complex<float>(0, -1);
                else
                    linePhase *= Complex<float>(0, 1);

                //if (frame >= 1041 && frame <= 1638) {
                //    fftwf_execute(forward);
                //    //for (int x = 0; x < 2048; ++x)
                //    //    amplitudes[y*2048 + x] = fftData[x].modulus() / 2048.0f;
                //    amplitudes[y] += fftData[y + 2];
                //}

                //h.write(outputb);
            }
            printf(".");

            for (int y = 3; y < 253; ++y) {
                int p = (((frame + 1)%3)*253 + y)*2048;
                int pu = (((frame + 1)%3)*253 + y - 1)*2048;
                if (y == 0)
                    pu = p;
                int pd = (((frame + 1)%3)*253 + y + 1)*2048;
                if (y == 252)
                    pd = p;
                int pp = ((frame%3)*253 + y)*2048;
                int pn = (((frame + 2)%3)*253 + y)*2048;

                for (int x = 0; x < 2048; ++x) {
                    Vector3<float> c = frameCache[p + x];
                    _outputb0[x] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
                    c = (frameCache[pu + x] + frameCache[pd + x] + frameCache[pp + x] + frameCache[pn + x])/4.0f;
                    _outputb1[x] = SRGB(byteClamp(c.x), byteClamp(c.y), byteClamp(c.z));
                }

                if (frame%2 != 0) {
                    h.write(_outputb0);
                    h.write(_outputb1);
                }
                else {
                    h.write(_outputb1);
                    h.write(_outputb0);
                }
            }

            //if (frame >= 1041 && frame <= 1638) {
            //    int bestOffset;
            //    float bestAmplitude = 0;
            //    for (int offset = -10; offset < 10; ++offset) {
            //        float amplitude = 0;
            //        for (int y = 0; y < 253; ++y)
            //            if (y+offset >= 0)
            //                amplitude += amplitudes[y*2048 + y + offset];
            //        if (amplitude > bestAmplitude) {
            //            bestAmplitude = amplitude;
            //            bestOffset = offset;
            //        }
            //    }
            //    printf("frame %i strongest offset %i amplitude %f\n", frame, bestOffset);
            //}

            ++frame;

            if ((frame & 1) == 0)
                framePhase *= Complex<float>(0, 1);

        }
        //for (int y = 0; y < 253; ++y)
        //    printf("  %f, %f,\n", amplitudes[y].x / (1639 - 1041), amplitudes[y].y / (1639 - 1041));
    }

    void decodeFrame()
    {
    }

    void setBrightness(double brightness)
    {
        _brightness = brightness;
    }
    void setSaturation(double saturation)
    {
        _saturation = saturation;
    }
    void setContrast(double contrast) { _contrast = contrast; }
    void setHue(double hue) { _hue = hue; }
    void setLumaCutoff(double lumaCutoff) { _lumaCutoff = lumaCutoff; }
    void setLumaHeterodyneFrequency(double lumaHeterodyneFrequency)
    {
        _lumaHeterodyneFrequency = lumaHeterodyneFrequency;
    }
    void setChromaCutoff(double chromaCutoff) { _chromaCutoff = chromaCutoff; }
    void setDeinterlacing(double deinterlacing) { _deinterlacing = deinterlacing; }
    void setFieldNumber(double fieldNumber) { _fieldNumber = fieldNumber; }

private:
    fftwf_plan _forward;
    fftwf_plan _backward;
    fftwf_plan _chromaForward;
    fftwf_plan _chromaBackward;
    Array<Complex<float>> _fftData;
    Array<Complex<float>> _chromaData;
    Array<Complex<float>> _burstData;
    Array<SRGB> _outputb0;
    Array<SRGB> _outputb1;

    double _brightness;
    double _saturation;
    double _contrast;
    double _hue;
    double _lumaCutoff;
    double _lumaHeterodyneFrequency;
    double _chromaCutoff;
    double _deinterlacing;
    double _fieldNumber;
};

class VCRDecodeWindow;

template<class T> class DecodedBitmapWindowT : public BitmapWindow
{
public:
    void setCaptureWindow(VCRDecodeWindow* vcrDecodeWindow)
    {
        _vcrDecodeWindow = vcrDecodeWindow;
    }
    void create()
    {
        setSize(Vector(960, 720));

        _vbiCapPipe = File("\\\\.\\pipe\\vbicap", true).openPipe();
        _vbiCapPipe.write<int>(1);

        BitmapWindow::create();
        _thread.setWindow(this);
        _thread.start();
    }

    void update()
    {
        _vbiCapPipe.read(_b, 1024*450);
        _decoder.setOutputBuffer(_bitmap);
        _decoder.decode();
        Bitmap<DWORD> nextBitmap = setNextBitmap(_bitmap);
        invalidate();
        _bitmap = nextBitmap;
    }

    void draw()
    {
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(Vector(960, 720));
        _thread.go();
    }

    void setBrightness(double brightness) { _decoder.setBrightness(brightness); }
    void setSaturation(double saturation) { _decoder.setSaturation(saturation); }
    void setContrast(double contrast) { _decoder.setContrast(contrast); }
    void setHue(double hue) { _decoder.setHue(hue; }
    void setLumaCutoff(double lumaCutoff) { _decoder.setLumaCutoff(lumaCutoff); }
    void setLumaHeterodyneFrequency(double lumaHeterodyneFrequency) { _decoder.setLumaHeterodyneFrequency(lumaHeterodyneFrequency); }
    void setChromaCutoff(double chromaCutoff) { _decoder.setChromaCutoff(chromaCutoff; }
    void setDeinterlacing(double deinterlacing) { _decoder.setDeinterlacing(deinterlacing); }
    void setFieldNumber(double fieldNumber) { _decoder.setFieldNumber(fieldNumber); }

private:
    Bitmap<DWORD> _bitmap;
    VCRDecoder _decoder;

    VCRDecodeWindow* _vcrDecodeWindow;
};

template<class T> class BrightnessSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setBrightness(value); }
    void create()
    {
        setRange(-255, 255);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef BrightnessSliderWindowT<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setSaturation(value); }
    void create()
    {
        setRange(0, 2);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef SaturationSliderWindowT<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setContrast(value); }
    void create()
    {
        setRange(0, 2);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef ContrastSliderWindowT<void> ContrastSliderWindow;

template<class T> class HueSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setHue(value); }
    void create()
    {
        setRange(-180, 180);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef HueSliderWindowT<void> HueSliderWindow;

template<class T> class LumaCutoffSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setLumaCutoff(value); }
    void create()
    {
        setRange(0, 5);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef LumaCutoffSliderWindowT<void> LumaCutoffSliderWindow;

template<class T> class LumaHeterodyneFrequencySliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setLumaHeterodyneFrequency(value); }
    void create()
    {
        setRange(0, 5);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef LumaHeterodyneFrequencySliderWindowT<void> LumaHeterodyneFrequencySliderWindow;

template<class T> class ChromaCutoffSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setChromaCutoff(value); }
    void create()
    {
        setRange(0, 5);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef ChromaCutoffSliderWindowT<void> ChromaCutoffSliderWindow;

template<class T> class DeinterlacingSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setDeinterlacing(value); }
    void create()
    {
        setRange(-1, 1);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef DeinterlacingSliderWindowT<void> DeinterlacingSliderWindow;

template<class T> class FieldNumberSliderWindowT : public Slider
{
public:
    void setHost(CaptureWindow* host) { _host = host; }
    void valueSet(double value) { _host->setFieldNumber(value); }
    void create()
    {
        setRange(0, 1);
        Slider::create();
    }
private:
    CaptureWindow* _host;
};
typedef FieldNumberSliderWindowT<void> FieldNumberSliderWindow;

class VCRDecodeWindow : public RootWindow
{
public:
    VCRDecodeWindow()
    {
        _output.setCaptureWindow(this);

        add(&_output);

        add(&_brightnessCaption);
        add(&_brightness);
        add(&_brightnessText);
        add(&_saturationCaption);
        add(&_saturation);
        add(&_saturationText);
        add(&_contrastCaption);
        add(&_contrast);
        add(&_contrastText);
        add(&_hueCaption);
        add(&_hue);
        add(&_hueText);
        add(&_lumaCutoffCaption);
        add(&_lumaCutoff);
        add(&_lumaCutoffText);
        add(&_lumaHeterodyneFrequencyCaption);
        add(&_lumaHeterodyneFrequency);
        add(&_lumaHeterodyneFrequencyText);
        add(&_chromaCutoffCaption);
        add(&_chromaCutoff);
        add(&_chromaCutoffText);
        add(&_deinterlacingCaption);
        add(&_deinterlacing);
        add(&_deinterlacingText);
        add(&_fieldNumberCaption);
        add(&_fieldNumber);
        add(&_fieldNumberText);
    }
    void create()
    {
        _brightnessCaption.setText("Brightness: ");
        _saturationCaption.setText("Saturation: ");
        _contrastCaption.setText("Contrast: ");
        _hueCaption.setText("Hue: ");
        _lumaCutoffCaption.setText("Luma cutoff: ");
        _lumaHeterodyneFrequencyCaption.setText("Luma heterodyne frequency: ");
        _chromaCutoffCaption.setText("Chroma cutoff: ");
        _deinterlacingCaption.setText("Deinterlacing: ");
        _fieldNumberCaption.setText("Field number: ");

        _brightness.setHost(this);
        _saturation.setHost(this);
        _contrast.setHost(this);
        _hue.setHost(this);
        _lumaCutoff.setHost(this);
        _lumaHeterodyneFrequency.setHost(this);
        _chromaCutoff.setHost(this);
        _deinterlacing.setHost(this);
        _fieldNumber.setHost(this);

        setText("VCR decode");
        setSize(Vector(1321, 760 + 23));
        RootWindow::create();

        _brightness.setValue(-26);
        _contrast.setValue(1.65);
        _saturation.setValue(0.30);
        _hue.setValue(0);
        _lumaCutoff.setValue(2.9);
        _lumaHeterodyneFrequency.setValue(4.4);
        _chromaCutoff.setValue(0.6);
        _deinterlacing.setValue(0);
        _fieldNumber.setValue(0);
    }
    void sizeSet(Vector size)
    {
        _output.setPosition(Vector(20, 20));
        int w = _output.right() + 20;

        Vector vSpace(0, 15);

        _brightness.setSize(Vector(301, 24));
        _brightness.setPosition(Vector(w, 20));
        _brightnessCaption.setPosition(_brightness.bottomLeft() + vSpace);
        _brightnessText.setPosition(_brightnessCaption.topRight());

        _saturation.setSize(Vector(301, 24));
        _saturation.setPosition(_brightnessCaption.bottomLeft() + 2*vSpace);
        _saturationCaption.setPosition(_saturation.bottomLeft() + vSpace);
        _saturationText.setPosition(_saturationCaption.topRight());

        _contrast.setSize(Vector(301, 24));
        _contrast.setPosition(_saturationCaption.bottomLeft() + 2*vSpace);
        _contrastCaption.setPosition(_contrast.bottomLeft() + vSpace);
        _contrastText.setPosition(_contrastCaption.topRight());

        _hue.setSize(Vector(301, 24));
        _hue.setPosition(_contrastCaption.bottomLeft() + 2*vSpace);
        _hueCaption.setPosition(_hue.bottomLeft() + vSpace);
        _hueText.setPosition(_hueCaption.topRight());

        _lumaCutoff.setSize(Vector(301, 24));
        _lumaCutoff.setPosition(_hueCaption.bottomLeft() + 2*vSpace);
        _lumaCutoffCaption.setPosition(_lumaCutoff.bottomLeft() + vSpace);
        _lumaCutoffText.setPosition(_lumaCutoffCaption.topRight());

        _lumaHeterodyneFrequency.setSize(Vector(301, 24));
        _lumaHeterodyneFrequency.setPosition(_lumaCutoffCaption.bottomLeft() + 2*vSpace);
        _lumaHeterodyneFrequencyCaption.setPosition(_lumaHeterodyneFrequency.bottomLeft() + vSpace);
        _lumaHeterodyneFrequencyText.setPosition(_lumaHeterodyneFrequencyCaption.topRight());

        _chromaCutoff.setSize(Vector(301, 24));
        _chromaCutoff.setPosition(_lumaHeterodyneFrequencyCaption.bottomLeft() + 2*vSpace);
        _chromaCutoffCaption.setPosition(_chromaCutoff.bottomLeft() + vSpace);
        _chromaCutoffText.setPosition(_chromaCutoffCaption.topRight());

        _deinterlacing.setSize(Vector(301, 24));
        _deinterlacing.setPosition(_chromaCutoffCaption.bottomLeft() + 2*vSpace);
        _deinterlacingCaption.setPosition(_deinterlacing.bottomLeft() + vSpace);
        _deinterlacingText.setPosition(_deinterlacingCaption.topRight());

        _fieldNumber.setSize(Vector(301, 24));
        _fieldNumber.setPosition(_deinterlacingCaption.bottomLeft() + 2*vSpace);
        _fieldNumberCaption.setPosition(_fieldNumber.bottomLeft() + vSpace);
        _fieldNumberText.setPosition(_fieldNumberCaption.topRight());

        RootWindow::sizeSet(size);
    }
    void setBrightness(double brightness)
    {
        _output.setBrightness(brightness);
        _brightnessText.setText(format("%f", brightness));
        _brightnessText.autoSize();
    }
    void setSaturation(double saturation)
    {
        _output.setSaturation(saturation);
        _saturationText.setText(format("%f", saturation));
        _saturationText.autoSize();
    }
    void setContrast(double contrast)
    {
        _output.setContrast(contrast);
        _contrastText.setText(format("%f", contrast));
        _contrastText.autoSize();
    }
    void setHue(double hue)
    {
        _output.setHue(hue);
        _hueText.setText(format("%f", hue));
        _hueText.autoSize();
    }
    void setLumaCutoff(double lumaCutoff)
    {
        _output.setSharpness(lumaCutoff);
        _lumaCutoffText.setText(format("%f", lumaCutoff));
        _lumaCutoffText.autoSize();
    }
    void setLumaHeterodyneFrequency(double lumaHeterodyneFrequency)
    {
        _output.setHeterodyneFrequency(lumaHeterodyneFrequency);
        _lumaHeterodyneFrequencyText.setText(format("%f", lumaHeterodyneFrequency));
        _lumaHeterodyneFrequencyText.autoSize();
    }
    void setChromaCutoff(double chromaCutoff)
    {
        _output.setChromaCutoff(chromaCutoff);
        _chromaCutoffText.setText(format("%f", chromaCutoff));
        _chromaCutoffText.autoSize();
    }
    void setDeinterlacing(double deinterlacing)
    {
        _output.setDeinterlacing(deinterlacing);
        _deinterlacingText.setText(format("%f", deinterlacing));
        _deinterlacingText.autoSize();
    }
    void setFieldNumber(double fieldNumber)
    {
        _output.setFieldNumber(fieldNumber);
        _fieldNumberText.setText(format("%f", fieldNumber));
        _fieldNumberText.autoSize();
    }

    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
private:
    DecodedBitmapWindow _output;
    TextWindow _brightnessCaption;
    BrightnessSliderWindow _brightness;
    TextWindow _brightnessText;
    TextWindow _saturationCaption;
    SaturationSliderWindow _saturation;
    TextWindow _saturationText;
    TextWindow _contrastCaption;
    ContrastSliderWindow _contrast;
    TextWindow _contrastText;
    TextWindow _hueCaption;
    HueSliderWindow _hue;
    TextWindow _hueText;
    TextWindow _lumaCutoffCaption;
    LumaCutoffSliderWindow _lumaCutoff;
    TextWindow _lumaCutoffText;
    TextWindow _lumaHeterodyneFrequencyCaption;
    LumaHeterodyneFrequencySliderWindow _lumaHeterodyneFrequency;
    TextWindow _lumaHeterodyneFrequencyText;
    TextWindow _chromaCutoffCaption;
    ChromaCutoffSliderWindow _chromaCutoff;
    TextWindow _chromaCutoffText;
    TextWindow _deinterlacingCaption;
    DeinterlacingSliderWindow _deinterlacing;
    TextWindow _deinterlacingText;
    TextWindow _fieldNumberCaption;
    FieldNumberSliderWindow _fieldNumber;
    TextWindow _fieldNumberText;

};

class Program : public WindowProgram<VCRDecodeWindow> { };
