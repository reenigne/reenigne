signalled:
    if (r16 == 0x00) {
        Z = 0x100;
        goto mainLoop;
    }
    if (r16 == 0x80) {
        *(Z++) = 0;
        if (Z == endZ)
            goto doSampleAndSend;
        goto mainLoop;
    }
    if (r16 == 0xc0) {
        *(Z++) = 1;
        if (Z == endZ)
            goto doSampleAndSend;
        goto mainLoop;
    }
    goto mainLoop;

doSampleAndSend:
    Byte byte0 = 0;
    Byte byte1 = 0;
    Byte byte2 = 0;
    for (int i = 0; i < 8; ++i) {
        byte0 |= (memory[0x100 + i] << i);
        byte1 |= (memory[0x108 + i] << i);
        byte2 |= (memory[0x110 + i] << i);
    }
    int length = ((byte1 | (byte2 << 8)) & 0x7ff) + 1;

    // Set up channel bits
    out(5, byte0 & 7);

    Z = ((byte0 & 8) != 0 ? readC : readD) + (2048 - length)*2;

    cycleDelay((byte0 >> 4) & 2);

    Y = 0x100;

    ijmp

doSend:
    for (int i = 0; i < length; ++i) {
        while (!(UCSR0A & 0x20));  // 0x20 == UDRE0, [0xc0] == UCSR0A
        UDR0 = memory[i + 0x100];                 // [0xc6] = UDR0
    }

    out(5, 4);
    goto mainLoop;
