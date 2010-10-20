// 1 cycle @ 16MHz = 62.5ns
//
// MCLR rise time < 1us  (16 cycles)
// T_PPDP > 5us          (80 cycles)
// T_SET0 > 100ns        (2 cycles)
// T_HLD0 > 5us          (80 cycles)
// T_SET1 > 100ns        (2 cycles)
// T_HLD1 > 100ns        (2 cycles)
// T_DLY1 > 1us          (16 cycles)
// T_DLY2 > 1us          (16 cycles)
// T_DLY3 < 80ns         (2 cycles)
// T_ERA < 10ms          (160000 cycles)
// T_PROG < 2ms          (32000 cycles)
// T_DIS > 100us         (1600 cycles)
// T_RESET ~ 10ms        (160000 cycles)
//
// Load Data for Program Memory   x x 0 0 1 0     (0 data(14) 0)
// Read Data from Program Memory  x x 0 1 0 0     (0 data(14) 0)
// Increment Address              x x 0 1 1 0
// Begin Programming              x x 1 0 0 0
// End Programming                x x 1 1 1 0
// Bulk Erase Program Memory      x x 1 0 0 1
//
//
// 4 data lines needed
//   0 VDD
//   1 MCLR
//   2 ICSPCLK
//   3 ICSPDAT
//
// Memory
//  000-1FE - program memory
//  1FF     - reset vector (calibration MOVLW?)
//  200-203 - user ID locations
//  204     - backup OSCCAL value
//  205-23F - reserved
//  240-3FE - unimplemented
//  3FF     - configuration word, default location


register uint8_t lineInFrame __asm__ ("r4");
register uint8_t frameInBeatLow __asm__ ("r5");
register uint8_t frameInBeatHigh __asm__ ("r6");

void enterProgrammingMode()
{
    raiseVDD();
    wait5us();
    raiseVPP();
    wait5us();
}

void leaveProgrammingMode()
{
    lowerVPP();
    lowerVDD();
    wait10ms();
}

void sendBit(bool b)
{
    raiseClock();
    if (b)
        raiseData();
    else
        lowerData();
    wait100ns();
    lowerClock();
    wait100ns();
}

void sendCommand(uint8_t command)
{
    sendBit((command & 1) != 0);
    sendBit((command & 2) != 0);
    sendBit((command & 4) != 0);
    sendBit((command & 8) != 0);
    sendBit(false);
    sendBit(false);
}

bool readBit()
{
    raiseClock();
    wait100ns();  // data should be valid after 80ns
    bool r = getData();
    lowerClock();
    wait100ns();
    return r;
}

uint16_t readData()
{
    sendCommand(4);
    wait1us();
    setDataInput();
    uint16_t d = 0;
    readBit();
    if (readBit()) d |= 1;
    if (readBit()) d |= 2;
    if (readBit()) d |= 4;
    if (readBit()) d |= 8;
    if (readBit()) d |= 0x10;
    if (readBit()) d |= 0x20;
    if (readBit()) d |= 0x40;
    if (readBit()) d |= 0x80;
    if (readBit()) d |= 0x100;
    if (readBit()) d |= 0x200;
    if (readBit()) d |= 0x400;
    if (readBit()) d |= 0x800;
    readBit();
    readBit();
    readBit();
    setDataOutput();
    wait1us();
}

bool programData(uint16_t data)
{
    sendCommand(2);
    wait1us();
    sendBit(false);
    sendBit((data & 1) != 0);
    sendBit((data & 2) != 0);
    sendBit((data & 4) != 0);
    sendBit((data & 8) != 0);
    sendBit((data & 0x10) != 0);
    sendBit((data & 0x20) != 0);
    sendBit((data & 0x40) != 0);
    sendBit((data & 0x80) != 0);
    sendBit((data & 0x100) != 0);
    sendBit((data & 0x200) != 0);
    sendBit((data & 0x400) != 0);
    sendBit((data & 0x800) != 0);
    sendBit(false);
    sendBit(false);
    sendBit(false);
    wait1us();
    sendCommand(8);
    wait2ms();
    sendCommand(14);
    wait100us();
    return (readData() == data);
}

void incrementAddress()
{
    sendCommand(6);
    wait1us();
}

void bulkErase()
{
    sendCommand(9);
    wait10ms();
}

void readAll()
{
    enterProgrammingMode();
    data[0x205] = readData();
    for (int16_t pc = 0; pc < 0x205; ++pc) {
        incrementAddress();
        data[pc] = readData();
    }
    leaveProgrammingMode();
}

bool writeAll()
{
    enterProgrammingMode();
    for (int16_t pc = 0; pc < 0x201; ++pc)
        incrementAddress();
    bulkErase();
    leaveProgrammingMode();

    enterProgrammingMode();
    if (!programData(data[0x205]))
        return false;
    for (int16_t pc = 0; pc < 0x205; ++pc) {
        incrementAddress();
        if (!programData(data[pc]))
            return false;
    }
    leaveProgrammingMode();
}


