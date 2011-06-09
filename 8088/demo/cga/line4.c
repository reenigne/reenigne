void line(UInt16 x0, UInt16 y0, UInt16 x1, UInt16 y1, UInt8 c)
{
    ES = 0xb800;
    dy = abs(y1 - y0);
    dx = abs(x1 - x0);
    if (dx < dy) {
        dy2 = dy << 1;
        dx2 = dx << 1;
        UInt16 e = -dx;
        if (y0 > y1) {
            DX = x0;
            x0 = x1;
            x1 = DX;
            DX = y0;
            y0 = y1;
            y1 = DX;
        }
        initPosition(x0, y0);
        UInt16 count = dy + 1;
        if (x1 < x0) while (count-->0) { plot(); down(); e += dx2; if (e > 0) { left();  e -= dy2; } }
        else         while (count-->0) { plot(); down(); e += dx2; if (e > 0) { right(); e -= dy2; } }
    }
    else {
        dx2 = dx << 1;
        dy2 = dy << 1;
        UInt16 e = -dy;
        if (x0 > x1) {
            DX = x0;
            x0 = x1;
            x1 = DX;
            DX = y0;
            y0 = y1;
            y1 = DX;
        }
        initPosition(x0, y0);
        UInt16 count = dx + 1;
        if (y1 < y0) { while (count-->0) { plot(); right(); e += dy2; if (e > 0) { up();   e -= dx2; } }
        else           while (count-->0) { plot(); right(); e += dy2; if (e > 0) { down(); e -= dx2; } }
    }
}

void line(UInt16 x0, UInt16 y0, UInt16 x1, UInt16 y1, UInt8 c)
{
    ES = 0xb800;
    initPosition(x0, y0);
    UInt16 dx = x1 - x0;
    UInt16 dy = y1 - y0;
    if (dy >= 0) {
        if (dx >= 0) {
            if (dx < dy) {
                UInt16 e = -dy;
                UInt16 count = dy + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); down();  e += dx; if (e > 0) { right(); e -= dy; } }
            }
            else {
                UInt16 e = -dx;
                UInt16 count = dx + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); right(); e += dy; if (e > 0) { down();  e -= dx; } }
            }
        }
        else {
            dx = -dx;
            if (dx < dy) {
                UInt16 e = -dy;
                UInt16 count = dy + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); down();  e += dx; if (e > 0) { left();  e -= dy; } }
            }
            else {
                UInt16 e = -dx;
                UInt16 count = dx + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); left();  e += dy; if (e > 0) { down();  e -= dx; } }
            }
        }
    }
    else {
        dy = -dy;
        if (dx >= 0) {
            if (dx < dy) {
                UInt16 e = -dy;
                UInt16 count = dy + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); up();    e += dx; if (e > 0) { right(); e -= dy; } }
            }
            else {
                UInt16 e = -dx;
                UInt16 count = dx + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); right(); e += dy; if (e > 0) { up();    e -= dx; } }
            }
        }
        else {
            dx = -dx;
            if (dx < dy) {
                UInt16 e = -dy;
                UInt16 count = dy + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); up();    e += dx; if (e > 0) { left();  e -= dy; } }
            }
            else {
                UInt16 e = -dx;
                UInt16 count = dx + 1;
                dx <<= 1;
                dy <<= 1;
                while (count-->0) { plot(); left();  e += dy; if (e > 0) { up();    e -= dx; } }
            }
        }
    }
}

// One way of specifying a line:
//   x0 - 2 bytes
//   y0 - 1 byte
//   x1 - 2 bytes
//   y1 - 1 byte
//   colour - 1 byte
//   total = 7 bytes
// Another:
//   routine - 1 byte
//   major & number of pixels - 2 bytes
//   minor & initia error - 2 bytes
//   initial location - 2 bytes
//   total = 7 bytes

// Incoming:
//   DL = colour
//   DI = y0
//   CX = x0
//

// Inner loop:
//   AL = byte read or to write
//   AH = mask
//   DL = colour
//   DH = temporary
//   DI = screen memory location
//   ES = screen memory segment
//   CX = number of pixels to plot remaining
//   SI = error
//   BP = error increment
//   SP = error decrement
//   BL = inverse mask

// TODO: unroll the loop by 2 for verticals, 4 for horizontals

// Down major, right minor

// Non-unrolled
lineLoop:
  es: xor [bx], ah     ; 3 2 20 23
  add bh,020           ; 3 0 12  4
  cmp bh,040           ; 3 0 12  4
  jl oddLine           ; 2 0  8  4/16
  sub bx,03fb0         ; 4 0 16  4
oddLine:
  add si,bp            ; 2 0  8  3
  jle noAdjust         ; 2 0  8  4/16
  shr ah,1             ; 2 0  8  2
  shr ah,1             ; 2 0  8  2
  jnc noNewByte        ; 2 0  8  4/16
  mov ah,0c0           ; 2 0  8  4
  inc bx               ; 1 0  4  2
noNewByte:
  sub si,di            ; 2 0  8  3
noAdjust:
  loop lineLoop        ; 2 0  8  5/17

// Unrolled
lineLoop0:
  es: xor [bx], ah     ; 3 2 20 23
  add bh,020           ; 3 0 12  4
  add si,bp            ; 2 0  8  3
  jle noAdjust0        ; 2 0  8  4/16
  shr ah,1             ; 2 0  8  2
  shr ah,1             ; 2 0  8  2
  jnc noNewByte0       ; 2 0  8  4/16
  mov ah,0c0           ; 2 0  8  4
  inc bx               ; 1 0  4  2
noNewByte0:
  sub si,di            ; 2 0  8  3
noAdjust0:
  loop lineLoop1       ; 2 0  8  5/17
  jmp done
lineLoop1:
  es: xor [bx], ah     ; 3 2 20 23
  sub bx,01fb0         ; 4 0 16  4
  add si,bp            ; 2 0  8  3
  jle noAdjust1        ; 2 0  8  4/16
  shr ah,1             ; 2 0  8  2
  shr ah,1             ; 2 0  8  2
  jnc noNewByte1       ; 2 0  8  4/16
  mov ah,0c0           ; 2 0  8  4
  inc bx               ; 1 0  4  2
noNewByte1:
  sub si,di            ; 2 0  8  3
noAdjust1:
  loop lineLoop0       ; 2 0  8  5/17
done:

// Right major, down minor

// Non-unrolled
lineLoop:
  es: xor [bx], ah     ; 3 2 20 23
  shr ah,1             ; 2 0  8  2
  shr ah,1             ; 2 0  8  2
  jnc noNewByte        ; 2 0  8  4/16
  mov ah,0c0           ; 2 0  8  4
  inc bx               ; 1 0  4  2
noNewByte:
  add si,bp            ; 2 0  8  3
  jle noAdjust         ; 2 0  8  4/16
  add bh,020           ; 3 0 12  4
  cmp bh,040           ; 3 0 12  4
  jl oddLine           ; 2 0  8  4/16
  sub bx,03fb0         ; 4 0 16  4
oddLine:
  sub si,di            ; 2 0  8  3
noAdjust:
  loop lineLoop        ; 2 0  8  5/17

// Unrolled
lineLoop0:
  es: xor [bx], al     ; 3 2 20 23
  add si,bp            ; 2 0  8  3
  jle noAdjust0        ; 2 0  8  4/16
  add bh,020           ; 3 0 12  4
  cmp bh,040           ; 3 0 12  4
  jl oddLine0          ; 2 0  8  4/16
  sub bx,03fb0         ; 4 0 16  4
oddLine0:
  sub si,sp            ; 2 0  8  3
noAdjust0:
  loop lineLoop1       ; 2 0  8  5/17
  jmp done
lineLoop1:
  es: xor [bx], ah     ; 3 2 20 23
  add si,bp            ; 2 0  8  3
  jle noAdjust1        ; 2 0  8  4/16
  add bh,020           ; 3 0 12  4
  cmp bh,040           ; 3 0 12  4
  jl oddLine1          ; 2 0  8  4/16
  sub bx,03fb0         ; 4 0 16  4
oddLine1:
  sub si,di            ; 2 0  8  3
noAdjust1:
  loop lineLoop2       ; 2 0  8  5/17
  jmp done
lineLoop2:
  es: xor [bx], dl     ; 3 2 20 23
  add si,bp            ; 2 0  8  3
  jle noAdjust2        ; 2 0  8  4/16
  add bh,020           ; 3 0 12  4
  cmp bh,040           ; 3 0 12  4
  jl oddLine2          ; 2 0  8  4/16
  sub bx,03fb0         ; 4 0 16  4
oddLine2:
  sub si,di            ; 2 0  8  3
noAdjust2:
  loop lineLoop3       ; 2 0  8  5/17
  jmp done
lineLoop3:
  es: xor [bx], dh     ; 3 2 20 23
  inc bx               ; 1 0  4  2
  add si,bp            ; 2 0  8  3
  jle noAdjust3        ; 2 0  8  4/16
  add bh,020           ; 3 0 12  4
  cmp bh,040           ; 3 0 12  4
  jl oddLine3          ; 2 0  8  4/16
  sub bx,03fb0         ; 4 0 16  4
oddLine3:
  sub si,di            ; 2 0  8  3
noAdjust3:
  loop lineLoop0       ; 2 0  8  5/17
done:



void draw_line(int xP, int yP, int xQ, int yQ)
{
    int x = xP;
    int y = yP;
    int D = 0;
    int dx = xQ - xP;
    int dy = yQ - yP;
    int c;
    int M;
    int xinc = 1;
    int yinc = 1;
    if (dx < 0) { xinc = -1; dx = -dx; }
    if (dy < 0) { yinc = -1; dy = -dy; }
    if (dy < dx) {
        c = 2 * dx;
        M = 2 * dy;
        while (x != xQ) {
            putpix(x, y);
            x += xinc;
            D += M;
            if (D > dx) {
                y += yinc;
                D -= c;
            }
        }
    }
    else {
        c = 2 * dy;
        M = 2 * dx;
        while (y != yQ) {
            putput(x, y);
            y += yinc;
            D += M;
            if (D > dy) {
                x += xinc
                D -= c;
            }
        }
    }
}
