#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        _frame = 0;
        _p = 0;
        addImage("bass.cgu");
        addImage("lena.cgu");
        addImage("lips.cgu");
        addImage("lsl6.cgu");
        addImage("poptitle.cgu");
        addImage("wolf3d.cgu");
        appendChunk(_frame, 0, 0x1ffc, 1, 1, 0, 0);
        File("images.dat").openWrite().write(_images);
        File("meta.dat").openWrite().write(_meta);
    }
private:
    AppendableArray<Byte> _images;
    AppendableArray<Byte> _meta;
    int _frame;
    int _p;

    void addImage(String name)
    {
        String input = File("../../../../../v/" + name).contents();
        for (int i = 0; i < 28*140*2*2; ++i)
            _images.append(input[i]);
        appendChunk(_frame, _p, 0, 28, 140, 0, 0);
        appendChunk(_frame, _p + 28*140*2, 0x2000, 28, 140, 0, 0);
        _p += 28*140*2*2;
        _frame += 30;
    }
    void appendChunk(int frame, int p, int dest, int width, int height, int skip, int crtc)
    {
        appendWord(frame);     // Frame number (30Hz)
        appendWord(p & 15);    // offset
        appendWord(p >> 4);    // segment
        appendWord(dest);      // Start position in video memory for copy
        _meta.append(width);   // Width of block in characters
        _meta.append(height);  // Height of block in characters
        appendWord(skip);      // Source bytes to skip each line
        appendWord(crtc);      // CRTC start address
    }
    void appendWord(int w)
    {
        _meta.append(w & 0xff);
        _meta.append(w >> 8);
    }
};