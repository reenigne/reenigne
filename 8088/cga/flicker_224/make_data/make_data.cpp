#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        _frame = 0;
        addImage("bass.cgu");
        addImage("lena.cgu");
        addImage("lips.cgu");
        addImage("lsl6.cgu");
        addImage("poptitle.cgu");
        addImage("wolf3d.cgu");
        appendWord(_frame);  // Frame number (30Hz)
        appendWord(0x1ffc);  // Start position in video memory for copy
        _output.append(2);  // Width of block in characters
        _output.append(1); // Height of block in characters
        for (int i = 0; i < 2*2*2; ++i)
            _output.append(0);
        appendWord(0);       // CRTC start address

        File("video.dat").openWrite().write(_output);
    }
private:
    AppendableArray<Byte> _output;
    int _frame;

    void addImage(String name)
    {
        String input = File("../../../../../v/" + name).contents();
        appendWord(_frame);  // Frame number (30Hz)
        _frame += 30;
        appendWord(0);       // Start position in video memory for copy
        _output.append(28);  // Width of block in characters
        _output.append(140); // Height of block in characters
        for (int i = 0; i < 28*140*2*2; ++i)
            _output.append(input[i]);
        appendWord(0);       // CRTC start address
    }
    void appendWord(int w)
    {
        _output.append(w & 0xff);
        _output.append(w >> 8);
    }
};