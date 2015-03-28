#include "alfe/main.h"
#include "alfe/vectors.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        static const int numFrames = 838;
        static const int numLines = 200;

        FileHandle output = File("tables.asm").openWrite();
        output.write("align 16\n\n");
        //output.write("sineTable:");
        //for (int y = 0; y < 838 + 116 - 1; ++y) {
        //    int x = static_cast<int>(78.5 + 78.5 * sin(7 * tau*y / 838));
        //    if (x >= 157)
        //        x = 156;
        //    if (x < 0)
        //        x = 0;
        //    if ((y & 7) == 0)
        //        output.write("\n  dw ");
        //    else
        //        output.write(", ");
        //    output.write(String(hex(x*2, 4)));
        //}
        //output.write("\npixelTable:");
        //for (int x = 0; x < 320; ++x) {
        //    int xx = x % 160;
        //    
        //}


        output.write("frameTable:\n");
        Int16 frameno_6[numFrames];
        Int16 frameno_8[numFrames];
        Int32 frameno_1017_acc[numFrames];
        Int32 frameno_547_acc[numFrames];
        Int32 frameno_78_acc[numFrames];
        UInt16 frameno_20020_acc[numFrames];
        UInt16 frameno_240247_acc[numFrames];
        UInt16 frameno_140144_acc[numFrames];
        for (int frame = 0; frame < numFrames; ++frame) {
            frameno_6[frame] = (frame*6) & 0xff;
            frameno_8[frame] = (frame*8) & 0xff;
            //frameno_1017[frame] = frame*1017;
            //frameno_547[frame] = frame*547;
            //frameno_78[frame] = frame*78;
            frameno_1017_acc[frame] = ((frame*1017)>>8) & 0xff;
            frameno_547_acc[frame] = ((frame*547)>>8) & 0xff;
            frameno_78_acc[frame] = ((frame*78)>>8) & 0xff;
            //frameno_20020[frame] = frame*20020;
            //frameno_240247[frame] = frame*240247;
            //frameno_140144[frame] = frame*140144;
            frameno_20020_acc[frame] = ((frame*20020)>>16) & 0xff;
            frameno_240247_acc[frame] = ((frame*240247)>>16) & 0xff;
            frameno_140144_acc[frame] = ((frame*140144)>>16) & 0xff;

            output.write(String("  db "));
            output.write(String(hex(frameno_8[frame], 2)) + ", ");
            output.write(String(hex(frameno_6[frame], 2)) + ", ");
            output.write(String(hex(frameno_1017_acc[frame], 2)) + ", ");
            output.write(String(hex(frameno_547_acc[frame], 2)) + ", ");
            output.write(String(hex(frameno_78_acc[frame], 2)) + ", ");
            output.write(String(hex(frameno_20020_acc[frame], 2)) + ", ");
            output.write(String(hex(frameno_240247_acc[frame], 2)) + ", ");
            output.write(String(hex(frameno_140144_acc[frame], 2)) + "\n");
            output.write(String("  dw ") + hex(frame*2, 4) + "\n");
        }
        output.write(String("\n"));

        Int16 y_900[numLines];
        Int16 y_1024[numLines];
        UInt16 y_521[numLines];
        UInt16 y_1043[numLines];
        UInt16 y_642[numLines];
        Int16 y_82[numLines];
        UInt16 y_469[numLines];
        UInt16 y_1064[numLines];
        UInt16 y_2107[numLines];
        int yTab[numLines];
        output.write(String("yTable:\n"));
        for (int y = 0; y < numLines; ++y) {
            y_900[y] = (Int16)((y*900) >> 8) & 0xff;
            y_1024[y] = (Int16)((y*1024) >> 8) & 0xff;
            y_521[y] = ((y*130) >> 6) & 0xff;
            y_1043[y] = ((y*261) >> 6) & 0xff;
            y_642[y] = ((y*161) >> 6) & 0xff;
            y_82[y] = y*82;
            y_469[y] = ((y*59) >> 5) & 0xff;
            y_1064[y] = ((y*133) >> 5) & 0xff;
            y_2107[y] = ((y*263) >> 5) & 0xff;
            yTab[y] = (y & 1)*1848;

            output.write("  dw " + String(hex(y_82[y], 4)) + "\n");
            output.write("  db " + String(hex(y_1024[y], 2)) + ", ");
            output.write(String(hex(y_900[y], 2)) + ", ");
            output.write(String(hex(y_469[y], 2)) + ", ");
            output.write(String(hex(y_1064[y], 2)) + ", ");
            output.write(String(hex(y_2107[y], 2)) + ", ");
            output.write(String(hex(y_521[y], 2)) + ", ");
            output.write(String(hex(y_1043[y], 2)) + ", ");
            output.write(String(hex(y_642[y], 2)) + "\n");
            output.write("  dw " + String(hex(yTab[y], 4)) + ", ");
            output.write(String(hex(y * ((numFrames*2 + 15) >> 4), 4)) + "\n");
        }
        output.write(String("\n"));

        Int16 sintab_12[512];
        Int16 sintab_13[512];
        Int16 sintab_16[512];
        Int16 sintab_25[512];
        Int16 sintab_42[512];
        Int16 sintab_48[512];
        Int16 sintab_76[512];
        for (int i = 0; i < 512; ++i) {
            sintab_12[i] = (Int16) (sin (i * 2.0 * M_PI / 256.0) * 128.0 * 12.0);
            //sintab_13[i] = (Int16) (sin (i * 2.0 * M_PI / 256.0) * 128.0 * 13.0);
            sintab_25[i] = (Int16) (sin (i * 2.0 * M_PI / 256.0) * 128.0 * 25.0);
            sintab_42[i] = (Int16) (sin (i * 2.0 * M_PI / 256.0) * 128.0 * 42.0) + 77*256;
            //sintab_48[i] = (Int16) (sin (i * 2.0 * M_PI / 256.0) * 128.0 * 12.0);
            sintab_16[i] = (Int16) (sin (i * 2.0 * M_PI / 256.0) * 128.0 * 16.0);
            sintab_76[i] = (Int16) (sin (i * 2.0 * M_PI / 256.0) * 128.0 * 76.0);
        
        }
        output.write(String("sintab_12:\n"));
        for (int i = 0; i < 256; ++i)
            output.write(String("  dw ") + hex(sintab_12[i], 4) + "\n");
        output.write(String("sintab_25:\n"));
        for (int i = 0; i < 256; ++i)
            output.write(String("  dw ") + hex(sintab_25[i], 4) + "\n");
        output.write(String("sintab_42:\n"));
        for (int i = 0; i < 256; ++i)
            output.write(String("  dw ") + hex(sintab_42[i], 4) + "\n");
        output.write(String("sintab_16:\n"));
        for (int i = 0; i < 256; ++i)
            output.write(String("  dw ") + hex(sintab_16[i], 4) + "\n");
        output.write(String("sintab_76:\n"));
        for (int i = 0; i < 256; ++i)
            output.write(String("  dw ") + hex(sintab_76[i], 4) + "\n");

        int xTab[154];
        output.write(String("xTab:\n"));
        for (int x = 0; x < 154; ++x) {
            xTab[x] = x*12;
            output.write(String("  dw ") + hex(xTab[x], 4) + "\n");
        }

        int colourTab[16];
        //output.write(String("colourTab:\n"));
        for (int colour = 0; colour < 16; ++colour)
            colourTab[colour] = colour*3696;

        output.write(String("lcTab:\n"));
        static char lc[] = { 0, 0, 1, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        for (int colour = 0; colour < 16; ++colour)
            output.write(String("  dw ") + hex(colourTab[lc[colour]], 4) + "\n");
        
        output.write(String("hcTab:\n"));
        static char hc[] = { 0, 1, 9, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        for (int colour = 0; colour < 16; ++colour)
            output.write(String("  dw ") + hex(colourTab[hc[colour]], 4) + "\n");

        output.write(String("highlightTab:\n"));
        static char highlight[] = { 0, 0, 0, 0, 4, 6, 14, 15, 
            15, 15, 15, 15, 15, 15, 15, 15,
        };
        for (int colour = 0; colour < 16; ++colour)
            output.write(String("  dw ") + hex(colourTab[highlight[colour]], 4) + "\n");

        UInt16 finalTable[numLines*(numFrames + 2)];

        for (int frame = 0; frame < numFrames; ++frame) {
            UInt16 y_accum = 0;
            int p = frame;
            for (int y = 0; y < numLines; ++y) {
                UInt16 ym = y_82[y]
                    + sintab_16[frameno_8[frame] - y_1024[y]]
                    + sintab_12[frameno_6[frame] - y_900[y]];
                                                                            
                y_accum += ym & 0x1fc0;
                                                                            
                int ym3 = ((ym + 0x2000) >> 13);
                
                int colour;

                if (y_accum > 0x2000) {
                    colour = hc[ym3];
                    y_accum -= 0x2000;
                }
                else                                                          
                    colour = lc[ym3];
                                                                            
                Int16 ovtmp = sintab_76[frameno_1017_acc[frame] + y_469[y]]
                        + sintab_76[frameno_547_acc[frame] + y_1064[y]]
                        + sintab_25[frameno_78_acc[frame] + y_2107[y]];

                if (ovtmp > 8191)                                             
                    colour = highlight[ovtmp >> 11];

                int x = (sintab_42[frameno_20020_acc[frame] + y_521[y]]
                    + sintab_25[frameno_240247_acc[frame] + y_1043[y]]
                    + sintab_76[frameno_140144_acc[frame] + y_642[y]]) >> 8;

                int v = xTab[x] + yTab[y] + colour;

                finalTable[p] = v;
                p += (numFrames + 2);
            }
        }
        output.write("\n\nunrolledCode:\n");
    }
};