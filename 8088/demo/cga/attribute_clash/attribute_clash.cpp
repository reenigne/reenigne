#include "unity/main.h"
#include "unity/file.h"
#include "unity/perceptual.h"
#include <stdio.h>

typedef Vector3<double> Colour;

class PerceptualModel
{
public:
    static PerceptualModel luv() { return PerceptualModel(true); }
    static PerceptualModel lab() { return PerceptualModel(false); }
    Colour perceptualFromSrgb(const Vector3<UInt8>& srgb)
    {
        if (_luv)
            return luvFromSrgb(srgb);
        else
            return labFromSrgb(srgb);
    }
private:
    PerceptualModel(bool luv) : _luv(luv) { }
    bool _luv;
};

class Program : public ProgramBase
{
public:
    void run()
    {
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
#if 0
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
        PerceptualModel model = PerceptualModel::luv();

#if 0
        File pictureFile(String("/t/castle.raw"));
        String picture = pictureFile.contents();
        int height = 100;
        int width = 640;
#else
        File pictureFile(String("/t/rose.raw"));
        String picture = pictureFile.contents();
        int height = 200;
        int width = 320;
#endif

        Array<Colour> image;
        image.allocate(width*height);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                int p = (y*width + x)*3;
                image[y*width + x] = Vector3<UInt8>(picture[p], picture[p + 1],
                    picture[p + 2]);
            }

        int fileSize = width*height*3;
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
            Colour error(0, 0, 0);
            for (int x = 0; x < width; x += 8) {
                int bestCh;
                int bestAt;
                double score;
                double bestScore = 1e99;
                for (int ch = 0; ch < 0x100; ++ch) {
                    //if (ch != 0 && ch != 84 && ch != 106 && ch != 0xdd &&
                    //    ch != 65 && ch != 85 && ch != 67 && ch != 0x0d)
                    //    continue;
                    int bits = data[(3*256 + ch)*8];
                    for (int at = 0; at < 0x100; ++at) {
                        score = 0;
                        Colour error2 = error;
                        for (int xx = 0; xx < 8; ++xx) {
                            int col = ((bits & (128 >> xx)) != 0) ?
                                (at & 0x0f) : (at >> 4);
                            Colour target = image[y*width + x + xx] + error2;
                            error2 = target - colours[col];
                            score += error2.modulus2();
                            error2 /= 2;
                        }
                        if (score < bestScore) {
                            bestScore = score;
                            bestCh = ch;
                            bestAt = at;
                        }
                    }
                }
                int bits = data[(3*256 + bestCh)*8];
                for (int xx = 0; xx < 8; ++xx) {
                    int col = ((bits & (128 >> xx)) != 0) ?
                        (bestAt & 0x0f) : (bestAt >> 4);
                    int p = (y*width + x + xx)*3;
                    Vector3<UInt8> rgb = palette[col];
                    buffer[p] = rgb.x;
                    buffer[p + 1] = rgb.y;
                    buffer[p + 2] = rgb.z;
                    Colour target = image[y*width + x + xx] + error;
                    error = target - colours[col];
                    error /= 2;
                    if (y < height - 1)
                        image[(y + 1)*width + x + xx] += error;
                }
                int p = y*width/4 + x/4;
                screen[p] = bestCh;
                screen[p + 1] = bestAt;
                printf(".");

            }
        }

        File outputFile(String("attribute_clash.raw"));
        outputFile.save(String(buffer, 0, fileSize));

        File screenFile(String("picture.dat"));
        screenFile.save(String(screen, 0, width*height/4));
    }
private:
    int hexDigit(int n) { return n < 10 ? n + '0' : n + 'a' - 10; }
};
