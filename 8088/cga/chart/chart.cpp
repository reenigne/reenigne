#include "alfe/main.h"
#include "alfe/bitmap.h"

#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

int pal16[16][3]={{0,0,0},{0,0,170},{0,170,0},{0,170,170},{170,0,0},{170,0,170},{170,85,0},{170,170,170},{85,85,85},{85,85,255},{85,255,85},{85,255,255},{255,85,85},{255,85,255},{255,255,85},{255,255,255}};

unsigned char font[128] =
{
    0x00, 0x3C, 0x46, 0x4A, 0x52, 0x62, 0x3C, 0x00,	/* 0 */
    0x00, 0x18, 0x28, 0x08, 0x08, 0x08, 0x3E, 0x00, /* 1 */
    0x00, 0x3C, 0x42, 0x02, 0x3C, 0x40, 0x7E, 0x00, /* 2 */
    0x00, 0x3C, 0x42, 0x0C, 0x02, 0x42, 0x3C, 0x00, /* 3 */
    0x00, 0x08, 0x18, 0x28, 0x48, 0x7E, 0x08, 0x00, /* 4 */
    0x00, 0x7E, 0x40, 0x7C, 0x02, 0x42, 0x3C, 0x00, /* 5 */
    0x00, 0x3C, 0x40, 0x7C, 0x42, 0x42, 0x3C, 0x00, /* 6 */
    0x00, 0x7E, 0x02, 0x04, 0x08, 0x10, 0x10, 0x00, /* 7 */
    0x00, 0x3C, 0x42, 0x3C, 0x42, 0x42, 0x3C, 0x00, /* 8 */
    0x00, 0x3C, 0x42, 0x42, 0x3E, 0x02, 0x3C, 0x00, /* 9 */
    0x00, 0x3C, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x00, /* A */
    0x00, 0x7C, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00, /* B */
    0x00, 0x3C, 0x42, 0x40, 0x40, 0x42, 0x3C, 0x00, /* C */
    0x00, 0x78, 0x44, 0x42, 0x42, 0x44, 0x78, 0x00, /* D */
    0x00, 0x7E, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0x00, /* E */
    0x00, 0x7E, 0x40, 0x7C, 0x40, 0x40, 0x40, 0x00, /* F */
};

class Program : public ProgramBase
{
protected:
    void run()
    {
        Vector screenSize(640+2*8, 400+2*8);
        Vector size = screenSize*Vector(5, 8);
        Bitmap<SRGB> output(size);
        for (int y = 0; y < size.y; ++y) {
            int yMaj = y/screenSize.y;
            int yMin = y%screenSize.y;
            static const int portValueTable[8] = {
                0x1a00, 0x1e00, 0x0a00, 0x0a10,
                0x0a20, 0x0a30, 0x0e20, 0x0e30};
            int overscan = ((yMin - 8)/12) & 0x0f;
            int portValues = portValueTable[yMaj] | overscan;
            bool bpp1 = ((portValues & 0x1000) != 0);
            bool bw = ((portValues & 0x0400) != 0);
            bool backGroundI = ((portValues & 0x10) != 0);
            bool colorSel = ((portValues & 0x20) != 0);
            int pal[4];
            pal[0] = overscan;
            pal[1] = 2;
            pal[2] = 4;
            pal[3] = 6;
            if (colorSel || bw) {
                pal[1] = 3;
                pal[3] = 7;
            }
            if (colorSel && !bw)
                pal[2] = 5;
            if (backGroundI) {
                pal[1] += 8;
                pal[2] += 8;
                pal[3] += 8;
            }


        }

        RawFileFormat raw(size);
        raw.save(output, File("chart.raw"));
    }

    void runOriginal()
    {
        double Ys[1280],Is[1280],Qs[1280];
        int Rs[1280],Gs[1280],Bs[1280];
        double hue_adjust = -(45+33)*M_PI/180.0;

        int gamma[256];
        for (int i = 0; i < 256; ++i)
            gamma[i] =
                clamp(0, static_cast<int>(pow((i/255.0), 2.2)*255.0), 255);

        FILE* out=fopen("test.raw","wb");
        for (int y=0;y<(16*4*8)*4;++y) {
            int rgbi = 0;
            for (int xx=0;xx<1280+64;++xx) {
                int x = xx-64;
                if (x<0)
                    x+=64;
                bool bw;
                enum { black, digital, raw_composite, ntsc_decode } display_type;

                int row = y>>4;
                int nybble = x/80; //row&15; //
                int colreg = row&15; //x/80; //
                int display = x%80;

                if (display<24)
                    display_type=digital;
                else if (display<48)
                    display_type=raw_composite;
                else
                    display_type=ntsc_decode;

                int mode = (row>>4);

                if (mode<2) {
                    // 640x200 modes
                    rgbi = ((nybble<<((x&6)>>1))&8) ? colreg : 0;
                }
                else {
                    // 320x200 modes
                    int pal4[6][4]={{0,2,4,6},{0,10,12,14},{0,3,5,7},{0,11,13,15},{0,3,4,7},{0,11,12,15}};
                    if (x&4)
                        rgbi=pal4[mode-2][nybble&3];
                    else
                        rgbi=pal4[mode-2][nybble>>2];
                    if (rgbi==0)
                        rgbi=colreg;
                }
                bw=(mode==1 || mode>=6);

                if (xx<64) {
                    int o3d8 = (mode<2 ? 0x1a : 0x0a) + (bw ? 4 : 0);
                    int o3d9 = colreg;
                    if (mode>=2 && mode<6)
                        o3d9 += (((mode-2)&3)<<4);
                    if (mode>=6)
                        o3d9 += (((mode-6)&3)<<4)+0x20;

                    int regs = (o3d8<<8)+o3d9;
                    int ch_x = (63-xx)>>4;
                    int cy = (y>>1)&7;
                    int cx = ((63-xx)>>1)&7;
                    int cc = (regs>>(ch_x<<2))&15;
                    rgbi = (font[(cc<<3)+cy]&(1<<cx)) ? 15 : 0;
                    display_type=digital;
                }
                double composite;
#if 0
                double luma = (rgbi&8) ? 0.28 : 0;
                int color_burst[8][8]={
                    {0,0,0,0,0,0,0,0}, /* D0 */
                    {0,0,0,1,1,1,1,0}, /* D1 */
                    {1,1,0,0,0,0,1,1}, /* D2 */
                    {1,0,0,0,0,1,1,1}, /* D3 */
                    {0,1,1,1,1,0,0,0}, /* D4 */
                    {0,0,1,1,1,1,0,0}, /* D5 */
                    {1,1,1,0,0,0,0,1}, /* D6 */
                    {1,1,1,1,1,1,1,1}};/* D7 */
                double chroma;
                if (bw)
                    chroma = ((rgbi & 7)!=0 ? 0.72 : 0);
                else
                    chroma = (color_burst[rgbi&7][x&7] ? 0.72 : 0);
                composite = luma + chroma;
#else
                double luma = (rgbi&8) ? 0.32 : 0;
                int color_burst[8][8]={
                    {0,0,0,0,0,0,0,0}, /* D0 */
                    {0,0,0,1,1,1,1,0}, /* D1 */
                    {1,1,0,0,0,0,1,1}, /* D2 */
                    {1,0,0,0,0,1,1,1}, /* D3 */
                    {0,1,1,1,1,0,0,0}, /* D4 */
                    {0,0,1,1,1,1,0,0}, /* D5 */
                    {1,1,1,0,0,0,0,1}, /* D6 */
                    {1,1,1,1,1,1,1,1}};/* D7 */
                double chroma;
                if (bw)
                    chroma = ((rgbi & 7)!=0 ? 0.29 : 0);
                else
                    chroma = (color_burst[rgbi&7][x&7] ? 0.29 : 0);
                composite = luma + chroma + ((rgbi & 1) != 0 ? 0.07 : 0) +
                    ((rgbi & 2) != 0 ? 0.22 : 0) + ((rgbi & 4) != 0 ? 0.1 : 0);
#endif

                double s = sin(hue_adjust + (x&7)*M_PI/4.0);
                double c = cos(hue_adjust + (x&7)*M_PI/4.0);
                Ys[x]=composite;
                if (bw)
                    Is[x]=Qs[x]=0;
                else {
                    Is[x]=composite*2*c;
                    Qs[x]=composite*2*s;
                }

                switch (display_type) {
                    case black:
                        Rs[x]=Gs[x]=Bs[x]=0;
                        break;
                    case digital:
                        Rs[x]=pal16[rgbi][0];
                        Gs[x]=pal16[rgbi][1];
                        Bs[x]=pal16[rgbi][2];
                        break;
                    case raw_composite:
                        Rs[x]=Gs[x]=Bs[x]=composite*255.0;
                        break;
                    case ntsc_decode:
                        double Y=0;
                        double Q=0;
                        double I=0;
                        for (int i=0;i<8;++i) {
                            Y += Ys[x-i];
                            I += Is[x-i];
                            Q += Qs[x-i];
                        }
                        Y/=8.0;
                        I/=8.0;
                        Q/=8.0;
                        if (Y>1.0) Y=1.0; if (Y<0) Y=0;
                        if (I>0.5957) I=0.5957; if (I<-0.5957) I=-0.5957;
                        if (Q>0.5226) Q=0.5226; if (Q<-0.5226) Q=-0.5226;
                        double R = Y + 0.9562948323208939905*I + 0.6210251254447287141*Q;
                        double G = Y - 0.2721214740839773195*I - 0.6473809535176157223*Q;
                        double B = Y - 1.1069899085671282160*I + 1.7046149754988293290*Q;
                        Rs[x]=gamma[clamp(0, static_cast<int>(R*255), 255)];
                        Gs[x]=gamma[clamp(0, static_cast<int>(G*255), 255)];
                        Bs[x]=gamma[clamp(0, static_cast<int>(B*255), 255)];
                        break;
                }

                fputc(Rs[x],out);
                fputc(Gs[x],out);
                fputc(Bs[x],out);
            }
        }
        fclose(out);
    }
};