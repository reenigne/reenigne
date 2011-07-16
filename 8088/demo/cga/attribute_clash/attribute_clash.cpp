#include "unity/main.h"
#include "unity/file.h"
#include "unity/perceptual.h"

class Colour : public Vector3
{
public:
    Colour() { }
    Colour(int r, int g, int b)
    {

    }
};

class Program : public ProgramBase
{
public:
    void run()
    {
#if 0
        File file(String(
            "/t/projects/emulation/mamemess/mess_run/roms/pc/5788005.u33"));
        String data = file.contents();

#if 0
        // Dump entire ROM so we can see how it's laid out.
        int fileSize = 8*16 * 4*16*8;
        OwningBuffer buffer(fileSize);

        for (int set = 0; set < 4; ++set) {
            for (int ch = 0; ch < 256; ++ch) {
                for (int y = 0; y < 8; ++y) {
                    int bits = data[(set*256 + ch)*8 + y];
                    for (int x = 0; x < 8; ++x)
                        buffer[
                            ((set*16 + (ch >> 4))*8 + y)*8*16 + (ch & 15)*8 + x
                            ] = ((bits & (128 >> x)) != 0 ? 255 : 0);
                }
            }
        }
        // Set 0: MDA characters rows 0-7
        // Set 1: MDA characters rows 8-13
        // Set 2: CGA narrow characters
        // Set 3: CGA normal characters
#endif
        // Dump the top rows to a text file
        int fileSize = 13*256;
        OwningBuffer buffer(fileSize);

        for (int ch = 0; ch < 256; ++ch) {
            int bits = data[(3*256 + ch)*8];
            for (int x = 0; x < 8; ++x) {
                int p = ch*13;
                buffer[p + x] = ((bits & (128 >> x)) != 0 ? '*' : ' ');
                buffer[p + 8] = ' ';
                buffer[p + 9] = hexDigit(ch >> 4);
                buffer[p + 10] = hexDigit(ch & 15);
                buffer[p + 11] = 13;
                buffer[p + 12] = 10;
            }
        }
#endif
        File file(String("/t/rose.raw"));
        String data = file.contents();

        // TODO:
        // For each group of 8 pixels:
        //   Try all possible character/attribute combinations
        //   Use error diffusion within the group
        //   Minimize total error (squared?)
        //   Carry the error over into the start of the next group

        // To do the error diffusion: should we fold the error into the first pixel or distribute it across the whole group?
        //   First pixel I think

        int height = 200;
        int width = 320;

        int fileSize = 320*200*3;
        OwningBuffer buffer(fileSize);

        Vector3<UInt8> palette[0x10];
        palette[0x00] = Vector3<UInt8>(0x00, 0x00, 0x00);
        palette[0x01] = Vector3<UInt8>(0x00, 0x00, 0xaa);
        palette[0x02] = Vector3<UInt8>(0x00, 0xaa, 0x00);
        palette[0x03] = Vector3<UInt8>(0x00, 0xaa, 0xaa);
        palette[0x04] = Vector3<UInt8>(0xaa, 0x00, 0x00);
        palette[0x05] = Vector3<UInt8>(0xaa, 0x00, 0xaa);
        palette[0x06] = Vector3<UInt8>(0xaa, 0x55, 0x00);
        palette[0x07] = Vector3<UInt8>(0xaa, 0xaa, 0xaa);
        palette[0x08] = Vector3<UInt8>(0x55, 0x55, 0x55);
        palette[0x09] = Vector3<UInt8>(0x55, 0x55, 0xff);
        palette[0x0a] = Vector3<UInt8>(0x55, 0xff, 0x55);
        palette[0x0b] = Vector3<UInt8>(0x55, 0xff, 0xff);
        palette[0x0c] = Vector3<UInt8>(0xff, 0x55, 0x55);
        palette[0x0d] = Vector3<UInt8>(0xff, 0x55, 0xff);
        palette[0x0e] = Vector3<UInt8>(0xff, 0xff, 0x55);
        palette[0x0f] = Vector3<UInt8>(0xff, 0xff, 0xff);

        Colour colours[0x10];
        for (int i = 0; i < 0x10; ++i)
            colours[i] = Colour(palette[i]);


        OwningBuffer screen(width*height/4);

        for (int y = 0; y < height; ++y) {
            Colour error;
            for (int x = 0; x < width; x += 4) {
                Colour c[4];
                for (int xx = 0; xx < 4; ++xx) {
                    int p = (y*width + x + xx)*3;
                    c[xx] = Vector3<UInt8>(data[p], data[p + 1], data[p + 2]);
                }
                int bestW;
                int score;
                for (int w = 0; w < 0x10000; ++w) {
                }

            }
        }

        File outputFile(String("attribute_clash.raw"));
        outputFile.save(String(buffer, 0, fileSize));
    }
private:
    int hexDigit(int n) { return n < 10 ? n + '0' : n + 'a' - 10; }
};
