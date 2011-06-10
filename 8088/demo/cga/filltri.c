void filltri(UInt16 x0, UInt16 y0, UInt16 x1, UInt16 y1, UInt16 x2, UInt16 y2, UInt16 c)
{
    // Sprt the points into y order
    AX = y0;
    BX = y1;
    if (AX >= BX) {
        y1 = AX;
        y0 = BX;
        AX = x0;
        BX = x1;
        x1 = AX;
        x0 = BX;
    }
    AX = y1;
    BX = y2;
    if (AX >= BX) {
        y2 = AX;
        y1 = BX;
        AX = x1;
        BX = x2;
        x2 = AX;
        x1 = BX;
    }
    AX = y0;
    BX = y1;
    if (AX >= BX) {
        y1 = AX;
        y0 = BX;
        AX = x0;
        BX = x1;
        x1 = AX;
        x0 = BX;
    }
    xa = x0;
    b0 = x0;
    xc = x1;
    AX = y0;
    SI = y1;
    DI = y2;
    DX = DI - AX;
    Ddb = DX;
    DI -= SI;
    Ddc = DI;
    SI -= AX;
    Dda = SI;
    DX <<= 1;
    DI <<= 1;
    SI <<= 1;
    ca = SI;
    cb = DX;
    cc = DI;
    AL = 0xeb;     // JMP (short)
    AH = AL;
    BL = AL;
    BH = 0x7e;     // JLE
    if (SI != 0)
        AL = BH;
    askip = AL;
    if (DX != 0)
        AH = BH;
    bskip = AH;
    if (DI != 0)
        BL = BH;
    cskip = BL;
    AX = x0;
    SI = x1;
    DI = x2;
    DX = DI;
    DX -= AX;
    DI -= SI;
    SI -= AX;
    DX <<= 1;
    DI <<= 1;
    SI <<= 1;
    AL = 0x43;   // INC BX
    AH = AL;
    BL = AL;
    BH = 0x4b;   // DEC BX
    if (SI < 0) {
        SI = -SI;
        AL = BH;
    }
    Ma = SI;
    xia = AL;
    if (DX < 0) {
        DX = -DX;
        AH = BH;
    }
    Mb = DX;
    xib = AH;
    if (DI < 0) {
        DI = -DI;
        BL = BH;
    }
    Mc = DI;
    xic = BL;
    AX = y0;
    do {
        SI = xb;
        DI = SI;
        DX = SI;
        BX = xa;
        if (AX <= y1) {
            if (BX > SI)
                SI = BX;
            if (BX < DI)
                DI = BX;
            CX = Dda;
            CX += Ma;
            if (askip == 0x7e)
                while (CX > 0) {
                    BX += (xia == 0x43 ? 1 : -1);
                    CX -= ca;
                }
        }
        xa = BX;
        Dda = CX;
        BX = DX;
        CX = Ddb;
        CX += Mb;
        if (bskip == 0x7e)
            while (CX > 0) {
                BX += (xib == 0x43 ? 1 : -1);
                CX -= cb;
            }
        xb = BX;
        Ddb = CX;
        BX = xc;
        if (AX >= y1) {
            if (BX > SI)
                SI = BX;
            if (BX > DI)
                DI = BX;
            CX = Ddx;
            CX += Mc;
            if (cskip == 0x7e)
                while (CX > 0) {
                    BX += (xic == 0x43 ? 1 : -1);
                    CX -= cc;
                }
        }
        xc = BX;
        Ddc = CX;
        DX = c;
        horline(DI, SI, AX, c);
        ++AX;
    } while (AX < y2);
}



void fillTriangle(UInt16 x0, UInt16 y0, UInt16 x1, UInt16 y1, UInt16 x2, UInt16 y2)
{
    AX = y0;
    BX = y1;
    if (AX >= BX) {
        y1 = AX;
        y0 = BX;
        AX = x0;
        BX = x1;
        x1 = AX;
        x0 = BX;
    }
    AX = y1;
    BX = y2;
    if (AX >= BX) {
        y2 = AX;
        y1 = BX;
        AX = x1;
        BX = x2;
        x2 = AX;
        x1 = BX;
    }
    AX = y0;
    BX = y1;
    if (AX >= BX) {
        y1 = AX;
        y0 = BX;
        AX = x0;
        BX = x1;
        x1 = AX;
        x0 = BX;
    }

	if (y1 != y0) dA = ((x1 - x0)<<8)/(y1 - y0) else dA = (x1 - x0)<<8;
	if (y2 != y0) dB = ((x2 - x0)<<8)/(y2 - y0) else dB = 0;
	if (y2 != y1) dC = ((x2 - x1)<<8)/(y2 - y1) else dC = 0;

    xL = x0<<8;
    xR = xL;
    y = y0;

	if (dA > dB) {
        UInt8 count = 1 + y1 - y0;
        while (count-->0) {
			hLine(xL>>8, xR>>8, y);
            xL += dB;
            xR += dA;
            ++y;
        }
		xR = x1;
        UInt8 count = y2 - y1;
        while (count-->0) {
			hLine(xL>>8, xR>>8, y);
            xL += dB;
            xR += dC;
            ++y;
        }
	} else {
        UInt8 count = 1 + y1 - y0;
        while (count-->0) {
			hLine(xL>>8, xR>>8, y);
            xL += dA;
            xR += dB;
            ++y;
        }
		xR = x1;
        UInt8 count = y2 - y1;
        while (count-->0) {
			hLine(xL>>8, xR>>8, y);
            xL += dC;
            xR += dB;
            ++y;
        }
	}
}

// Assume screen width of 256 pixels or less
// Storing a triangle sequence: coordinates and colour: 7 bytes per triangle
// Processed:
//   Initial y
//   x0
//   xA, xB, xC (2 bytes each)
//   top part scanline count
//   bottom part scanline count
