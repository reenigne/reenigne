#include "alfe/main.h"

#ifndef INCLUDED_AVI_H
#define INCLUDED_AVI_H

#include "alfe/com.h"
#include "alfe/bitmap.h"
#include "alfe/rational.h"
#include <Vfw.h>

String aviErrorMessage(HRESULT hr)
{
    switch (hr) {
        case AVIERR_UNSUPPORTED:    return "Unsupported";
        case AVIERR_BADFORMAT:      return "Bad format";
        case AVIERR_MEMORY:         return "Out of memory";
        case AVIERR_INTERNAL:       return "Internal error";
        case AVIERR_BADFLAGS:       return "Bad flags";
        case AVIERR_BADPARAM:       return "Bad parameters";
        case AVIERR_BADSIZE:        return "Bad size";
        case AVIERR_BADHANDLE:      return "Bad AVIFile handle";
        case AVIERR_FILEREAD:       return "File read error";
        case AVIERR_FILEWRITE:      return "File write error";
        case AVIERR_FILEOPEN:       return "File open error";
        case AVIERR_COMPRESSOR:     return "Compressor error";
        case AVIERR_NOCOMPRESSOR:   return "Compressor not available";
        case AVIERR_READONLY:       return "File marked read-only";
        case AVIERR_NODATA:         return "No data";
        case AVIERR_BUFFERTOOSMALL: return "Buffer too small";
        case AVIERR_CANTCOMPRESS:   return "Can't compress";
        case AVIERR_USERABORT:      return "Aborted by user";
    }
    return "Error code: " + hex(hr, 8);
}

#define IF_AVI_ERROR_THROW(expr) CODE_MACRO( \
    HRESULT hrMacro = (expr); \
    IF_TRUE_THROW(FAILED(hrMacro), Exception(aviErrorMessage(hrMacro))); \
)

class AVIFileInitializer : Uncopyable
{
public:
    AVIFileInitializer() { AVIFileInit(); }
    ~AVIFileInitializer() { AVIFileExit(); }
};

class AVIFile : Uncopyable
{
public:
    AVIFile(File file)
    {
        NullTerminatedWideString inputPathWide(file.path());
        IF_AVI_ERROR_THROW(
            AVIFileOpen(&_aviFile, inputPathWide, OF_READ, NULL));
    }
    ~AVIFile() { AVIFileRelease(_aviFile); }
    void getStream(IAVIStream** aviStream, DWORD fccType, LONG lParam)
    {
        IF_AVI_ERROR_THROW(_aviFile->GetStream(aviStream, fccType, lParam));
    }
    //AVIFILEINFO aviFileInfo;
    //ZeroMemory(&aviFileInfo, sizeof(AVIFILEINFO));
    //IF_AVI_ERROR_THROW(aviFile->Info(&aviFileInfo, sizeof(AVIFILEINFO)));
        //int width = aviFileInfo.dwWidth;
        //int height = aviFileInfo.dwHeight;
private:
    IAVIFile* _aviFile;
};

class AVIStream
{
public:
    AVIStream(AVIFile* aviFile, DWORD fccType = streamtypeVIDEO,
        LONG lParam = 0)
    {
        aviFile->getStream(&_aviStream, fccType, lParam);
        ZeroMemory(&_aviStreamInfo, sizeof(AVISTREAMINFO));
        IF_AVI_ERROR_THROW(_aviStream->Info(&_aviStreamInfo,
            sizeof(AVISTREAMINFO)));

        if (_aviStreamInfo.fccType != streamtypeVIDEO)
            throw Exception("Not a video stream");

        LONG cbFormat = 0;
        _aviStream->ReadFormat(_frame, 0, &cbFormat);
        _formatBuffer.ensure(cbFormat);
        IF_AVI_ERROR_THROW(_aviStream->ReadFormat(_frame, &_formatBuffer[0],
            &cbFormat));
        _bitmapInfoHeader =
            *reinterpret_cast<BITMAPINFOHEADER*>(&_formatBuffer[0]);
    }
    PGETFRAME getFrameOpen()
    {
        BITMAPINFOHEADER header = _bitmapInfoHeader;
        header.biSize = sizeof(BITMAPINFOHEADER);
        header.biCompression = BI_RGB;
        header.biBitCount = 24;
        return AVIStreamGetFrameOpen(_aviStream, &header);
    }
    BITMAPINFOHEADER bitmapInfoHeader() { return _bitmapInfoHeader; }
    int frames() { return _aviStreamInfo.dwLength; }
private:
    COMPointer<IAVIStream> _aviStream;
    AVISTREAMINFO _aviStreamInfo;
    BITMAPINFOHEADER _bitmapInfoHeader;
    PCMWAVEFORMAT _pcmWaveFormat;
    LONG _frame;
    //Array<Byte> _buffer;
    Array<Byte> _formatBuffer;
};

class GetFrame : Uncopyable
{
public:
    GetFrame(AVIStream* aviStream) : _getFrame(aviStream->getFrameOpen())
    {
        _bitmapInfoHeader = aviStream->bitmapInfoHeader();
        _frames = aviStream->frames();
        _frame = 0;
    }
    Bitmap<SRGB> getFrame(Bitmap<SRGB> bitmap)
    {
        int height = _bitmapInfoHeader.biHeight;
        if (height < 0)
            height = -height;
        //if (_bitmapInfoHeader.biCompression != BI_RGB)
        //    throw Exception("Don't know how to decode this pixel type yet");
        Vector s(_bitmapInfoHeader.biWidth, height);
        //_buffer.ensure(s.x * s.y * _bitmapInfoHeader.biBitCount / 8);
        bitmap.ensure(s);
        //LONG lBytes;
        //LONG lSamples;
        //IF_AVI_ERROR_THROW(_aviStream->Read(_frame, 1, &_buffer[0],
        //    s.x * s.y * 4, &lBytes, &lSamples));
        //if (lBytes > s.x * s.y * 4)
        //    throw Exception("Frame too large");
        //if (lSamples != 1)
        //    throw Exception("Didn't get 1 frame");

        Byte* sl = static_cast<Byte*>(_getFrame->GetFrame(_frame)) +
            sizeof(BITMAPINFOHEADER);

        if (_bitmapInfoHeader.biBitCount == 24) {
            //auto sl = &_buffer[0];
            int ss = _bitmapInfoHeader.biWidth * 3;
            if (_bitmapInfoHeader.biHeight > 0) {
                sl += ss * (s.y - 1);
                ss = -ss;
            }
            auto dl = bitmap.data();
            for (int y = 0; y < s.y; ++y) {
                auto p = reinterpret_cast<SRGB*>(sl);
                auto d = reinterpret_cast<SRGB*>(dl);
                for (int x = 0; x < s.x; ++x) {
                    *d = *p;
                    ++p;
                    ++d;
                }
                sl += ss;
                dl += bitmap.stride();
            }
        }
        else {
            if (_bitmapInfoHeader.biBitCount != 32) {
                throw Exception(
                    "Don't know how to decode this pixel size yet");
            }
            //auto sl = &_buffer[0];
            int ss = _bitmapInfoHeader.biWidth * 4;
            if (_bitmapInfoHeader.biHeight > 0) {
                sl += ss * (s.y - 1);
                ss = -ss;
            }
            auto dl = bitmap.data();
            for (int y = 0; y < s.y; ++y) {
                auto p = reinterpret_cast<DWORD*>(sl);
                auto d = reinterpret_cast<SRGB*>(dl);
                for (int x = 0; x < s.x; ++x) {
                    DWORD v = *p;
                    *d = SRGB((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff);
                    ++p;
                    ++d;
                }
                sl += ss;
                dl += bitmap.stride();
            }
        }
        ++_frame;
        return bitmap;
    }
    bool atEnd() { return _frame == _frames; }
    ~GetFrame() { AVIStreamGetFrameClose(_getFrame); }
private:
    PGETFRAME _getFrame;
    BITMAPINFOHEADER _bitmapInfoHeader;
    int _frames;
    int _frame;
};

class AVIOptions
{
public:
    AVIOptions(HWND hParent, IAVIStream** videoStream)
      : _hParent(hParent), _videoStream(videoStream)
    {
        ZeroMemory(&_opts, sizeof(_opts));
        _opts.fccHandler = mmioFOURCC('u', 'l', 'r', 'g');
        _opts.dwFlags = AVICOMPRESSF_VALID;
        _opts.cbParms = 0; // 4;
    }
    void showOptions()
    {
        _aopts[0] = &_opts;
        IF_FALSE_THROW(
            AVISaveOptions(_hParent, 0, 1, _videoStream, _aopts) == TRUE);
    }
    AVICOMPRESSOPTIONS* options() { return _aopts[0]; }
    ~AVIOptions() { AVISaveOptionsFree(1, _aopts); }
private:
    AVICOMPRESSOPTIONS _opts;
    AVICOMPRESSOPTIONS* _aopts[1];
    HWND _hParent;
    IAVIStream** _videoStream;
};

class AVIWriter
{
public:
    AVIWriter(File file, Vector size, Rational frameRate,
        /*const WAVEFORMATEX* waveFormatEx,*/
        HWND hParent)
    {
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        BITMAPINFOHEADER& bih = bi.bmiHeader;
        bih.biSize = sizeof(bih);
        bih.biWidth = size.x;
        bih.biHeight = size.y;
        bih.biPlanes = 1;
        bih.biBitCount = 24;
        bih.biCompression = BI_RGB;
        _bytesPerLine = (bih.biWidth * bih.biBitCount / 8 + 3) & -4;
        bih.biSizeImage = _bytesPerLine * size.y;
        bih.biXPelsPerMeter = 10000;
        bih.biYPelsPerMeter = 10000;
        bih.biClrUsed = 0;
        bih.biClrImportant = 0;

        HDC hdcScreen = GetDC(0);
        _hdc = CreateCompatibleDC(hdcScreen);
        ReleaseDC(0, hdcScreen);

        _hBitmap = CreateDIBSection(_hdc, (BITMAPINFO*)&bih, DIB_RGB_COLORS,
            &_bits, NULL, NULL);
        IF_ZERO_THROW(_hBitmap);
        IF_ZERO_THROW(GetObject(_hBitmap, sizeof(_dibSection), &_dibSection));

        NullTerminatedWideString w(file.path());
        IF_AVI_ERROR_THROW(
            AVIFileOpen(&_pfile, w, OF_WRITE | OF_CREATE, NULL));
        //if (waveFormatEx == NULL)
        //    ZeroMemory(&_waveFormatEx, sizeof(WAVEFORMATEX));
        //else
        //    CopyMemory(&_waveFormatEx, waveFormatEx, sizeof(WAVEFORMATEX));
        //_audioStream = 0;
        _frames = 0;
        //_samples = 0;

        AVISTREAMINFO strhdr;
        ZeroMemory(&strhdr, sizeof(strhdr));
        strhdr.fccType = streamtypeVIDEO;
        strhdr.fccHandler = 0;
        strhdr.dwRate = frameRate.numerator;
        strhdr.dwScale = frameRate.denominator;
        strhdr.dwSuggestedBufferSize = _dibSection.dsBmih.biSizeImage;
        SetRect(&strhdr.rcFrame, 0, 0, _dibSection.dsBmih.biWidth,
            _dibSection.dsBmih.biHeight);
        IF_AVI_ERROR_THROW(
            AVIFileCreateStream(_pfile, &_videoStream, &strhdr));

        {
            AVIOptions options(hParent, &_videoStream);
            options.showOptions();
            IF_AVI_ERROR_THROW(AVIMakeCompressedStream(&_compressedStream,
                _videoStream, options.options(), NULL));
        }
        IF_AVI_ERROR_THROW(AVIStreamSetFormat(_compressedStream, 0,
            &_dibSection.dsBmih, _dibSection.dsBmih.biSize +
            _dibSection.dsBmih.biClrUsed * sizeof(RGBQUAD)));
    }
    ~AVIWriter()
    {
        //if (_audioStream != 0)
        //    AVIStreamRelease(_audioStream);
        if (_compressedStream != 0)
            AVIStreamRelease(_compressedStream);
        if (_videoStream != 0)
            AVIStreamRelease(_videoStream);
        if (_pfile != 0)
            AVIFileRelease(_pfile);

        DeleteDC(_hdc);
        DeleteObject(_hBitmap);
    }

    void AddAviFrame()
    {
        IF_AVI_ERROR_THROW(AVIStreamWrite(_compressedStream, _frames, 1,
            _dibSection.dsBm.bmBits, _dibSection.dsBmih.biSizeImage,
            AVIIF_KEYFRAME, NULL, NULL));
        ++_frames;
    }
    Byte* bits() { return static_cast<Byte*>(_bits); }
    int stride() { return _bytesPerLine; }

private:
    IAVIFile* _pfile;
    //WAVEFORMATEX _waveFormatEx;
    //IAVIStream* _audioStream;
    IAVIStream* _videoStream;
    IAVIStream* _compressedStream;
    int _frames;
    //int _samples;
    HDC _hdc;
    HBITMAP _hBitmap;
    void* _bits;
    DIBSECTION _dibSection;
    int _bytesPerLine;
};

#endif // INCLUDED_AVI_H
