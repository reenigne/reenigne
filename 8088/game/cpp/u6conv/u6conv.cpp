#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/colour_space.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        String chunks =
            File("q:\\projects\\u6\\reem\\chunks", true).contents();
        String map = File("q:\\projects\\u6\\reem\\map", true).contents();
        Bitmap<SRGB> tiles = PNGFileFormat<SRGB>().load(
            File("Q:\\Work\\Website\\Andrew\\computer\\u6maps\\u6tiles.png",
                true));
        Array<Byte> tileMap(0x10000);
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                int cx = (x>>3) & 31;
                int cy = (y>>3) & 31;
                int sx = x&7;
                int sy = y&7;
                int a = (((cy<<5) + cx)>>1)*3 + 0x7800;
                int c;
                if ((cx & 1) != 0)
                    c = (map[a + 2] << 4) + ((map[a + 1] >> 4) & 15);
                else
                    c = ((map[a + 1] & 15) << 8) + map[a];
                tileMap[y*0x100 + x] = chunks[(c<<6) + (sy<<3) + sx];
            }
        // tile active area           = 64 x 32      = 16 x 16
        // tile area with surrounding = 96 x 48      = 24 x 24
        // conversion area            = 768 x 192
        Bitmap<SRGB> output(Vector(768, 192));
        Linearizer l;
        for (int panel = 0; panel < 8; ++panel) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 8; ++x) {
                    int tile = panel*32 + y*8 + x;
                    Vector tileP(tile & 7, tile >> 3);
                    tileP <<= 4;
                    for (int yy = 0; yy < 16; ++yy)
                        for (int xx = 0; xx < 16; ++xx) {
                            SRGB c = tiles[tileP + Vector(xx, yy)];
                            for (int yyy = 0; yyy < 2; ++yyy)
                                for (int xxx = 0; xxx < 4; ++xxx) {
                                    output[Vector(xxx + xx*4 + x*96 + 16,
                                        yyy + yy*2 + y*48 + 8)] = c;
                                }
                        }
                    for (int ry = -1; ry <= 1; ++ry) {
                        for (int rx = -1; rx <= 1; ++rx) {
                            if (ry == 0 && rx == 0)
                                continue;
                            for (int yy = 0; yy < 16; ++yy) {
                                for (int xx = 0; xx < 16; ++xx) {
                                    if (rx*16 + xx < -4 || rx*16 + xx >= 20 ||
                                        ry*16 + yy < -4 || ry*16 + yy >= 20)
                                        continue;
                                    Colour total(0, 0, 0);
                                    int count = 0;
                                    for (int i = 0; i < 0x10000; ++i) {
                                        if (tileMap[i] != tile)
                                            continue;
                                        Vector p(i & 0xff, i >> 8);
                                        p += Vector(rx, ry);
                                        p &= 0xff;
                                        int t = tileMap[(p.y << 8) + p.x];
                                        Vector tt(t & 7, t >> 3);
                                        tt <<= 4;
                                        SRGB c = tiles[tt + Vector(xx, yy)];
                                        total += l.linear(c);
                                        ++count;
                                    }
                                    SRGB c(128, 128, 128);
                                    if (count != 0)
                                        c = l.srgb(total / count);
                                    for (int yyy = 0; yyy < 2; ++yyy)
                                        for (int xxx = 0; xxx < 4; ++xxx) {
                                            output[Vector(xxx + xx*4 + rx*64 + x*96 + 16,
                                                yyy + yy*2 + ry*32 + y*48 + 8)] = c;
                                        }
                                }
                            }
                        }
                    }

                        
                }
            }
            PNGFileFormat<SRGB>().save(output,
                File(String("panel") + decimal(panel) + ".png", true));
        }

        FileStream fs = File("world.dat").openWrite();
        fs.write(tileMap);
        Array<Byte> tileGraphics(0x10000);
        int p = 0;
        for (int panel = 0; panel < 8; ++panel) {
            String dat = File(String("panel") + decimal(panel) + "_out.dat").
                contents();
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 8; ++x) {
                    for (int yy = 0; yy < 16; ++yy) {
                        for (int xx = 0; xx < 16; ++xx) {
                            tileGraphics[p] = dat[(y*24 + yy + 4)*192 + x*24 + xx + 4];
                            ++p;
                        }
                    }
                }
            }
        }

        String sphere = File(String("q:\\pictures\\reenigne\\cga2ntsc\\sphere1s_out.dat"), true).contents();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; x += 2) {
                int p = (y + 3)*24 + x + 4;
                Word ch = sphere[p] + (sphere[p + 1] << 8);
                float yy = (static_cast<float>(y) - 7.5f)*3.0f;
                float xx = (static_cast<float>(x >> 1) - 3.5f)*5.0f;
                if (xx*xx + yy*yy >= 20.0f*20.0f /*22.5f*22.5f*/)
                    ch = 0xffff;
                tileGraphics[y*16+x] = ch & 0xff;
                tileGraphics[y*16+x + 1] = ch >> 8;

                tileGraphics[y*16+x + 0xff00] = 0xff;
                tileGraphics[y*16+x + 0xff01] = 0xff;
            }

        String coin = File(String("q:\\pictures\\reenigne\\cga2ntsc\\coin2_out.dat"), true).contents();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; x += 2) {
                int p = (y + 4)*24 + x + 4;
                Word ch = coin[p] + (coin[p + 1] << 8);
                float yy = (static_cast<float>(y) - 7.5f)*3.0f;
                float xx = (static_cast<float>(x >> 1) - 3.5f)*5.0f;
                if (xx*xx + yy*yy >= 20.0f*20.0f /*22.5f*22.5f*/)
                    ch = 0xffff;
                tileGraphics[y*16+x + 0x0600] = ch & 0xff;
                tileGraphics[y*16+x + 0x0601] = ch >> 8;
            }

        fs.write(tileGraphics);

        srand(0);
        Array<Byte> foreground(0x10000);
        for (int y = 0; y < 0x100; ++y) {
            for (int x = 0; x < 0x100; ++x) {
                int m = 0xff;
                if (rand() % 1000 == 0)
                    m = 6;
                foreground[y*0x100 + x] = m;
            }
        }
        fs.write(foreground);

    }
};