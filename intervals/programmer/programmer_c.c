#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

typedef uint8_t bool;
#define true 1
#define false 0

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


register uint8_t timerTick __asm__ ("r4");  // Increments every 65536 clock cycles
register uint8_t picTickLow __asm__ ("r5");
register uint8_t picTickHigh __asm__ ("r6");

void raiseVDD();
void lowerVDD();
void raiseVPP();
void lowerVPP();
void raiseClock();
void lowerClock();
void raiseData();
void lowerData();
void setDataInput();
void setDataOutput();
void wait100ns();
void wait1us();
void wait5us();
void wait100us();
void wait2ms();
void wait10ms();
void startTimer();
void stopTimer();
bool getData();

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
    return d;
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

uint16_t data[0x206];
uint16_t dataIndex = 0;

void doRead(bool all)
{
    enterProgrammingMode();
    data[0x205] = readData();
    for (int16_t pc = 0; pc < 0x205; ++pc) {
        incrementAddress();
        if (all || pc == 0x1ff)
            data[pc] = readData();
    }
    leaveProgrammingMode();
}

uint8_t spaceAvailable = true;
uint8_t sendState = 0;
uint8_t receiveState = 0;
uint8_t hexFileState = 0;
uint8_t hexLineState = 0;
uint8_t byteCount = 0;
uint16_t address = 0;
uint8_t recordType = 0;
uint8_t checkSum = 0;
uint8_t dataByte = 0;
uint8_t* dataPointer = 0;
uint8_t receivedCheckSum = 0;
uint8_t timerTickBuffer;
uint8_t picTickLowBuffer;
uint8_t picTickHighBuffer;
uint16_t zero = 0;
uint8_t errorCode;

void startNextHexLine()
{
    switch (hexFileState) {
        case 0:  // Send extended linear address record
            byteCount = 2;
            address = 0;
            recordType = 4;
            dataPointer = (uint8_t*)(&zero);
            hexFileState = 1;
            dataIndex = 0;
            break;
        case 1:  // Send normal data
            if (dataIndex == 0)
                address = 0;
            if (dataIndex < 0x1f5)
                byteCount = 0x10;
            else {
                byteCount = 0x205 - dataIndex;
                hexFileState = 2;
            }
            dataPointer = (uint8_t*)(&data[dataIndex]);
            dataIndex += byteCount;
            recordType = 0;
            break;
        case 2:  // Send configuration bits
            byteCount = 2;
            address = 0x1ffe;
            dataPointer = (uint8_t*)(&data[0x205]);
            recordType = 0;
            hexFileState = 3;
            break;
        case 3:  // Send end of file marker
            byteCount = 0;
            address = 0;
            recordType = 1;
            hexFileState = 4;
            break;
        case 4:  // Finish
            hexFileState = 255;
            break;
    }
}

uint8_t getHexByte()
{
    switch (hexLineState) {
        case 0:  // First byte of HEX line, byte count
            hexLineState = 1;
            return byteCount;
        case 1:  // Second byte of HEX line, address high
            hexLineState = 2;
            return address >> 8;
        case 2:  // Third byte of HEX line, address low
            hexLineState = 3;
            return address;
        case 3:  // Fourth byte of HEX line, record type
            hexLineState = 4;
            return recordType;
        case 4:  // Fifth and subsequent bytes of HEX line, data and checksum
            if (byteCount == 0) {
                startNextHexLine();
                hexLineState = 255;
                return -checkSum;
            }
            --byteCount;
            ++address;
            return *(dataPointer++);
    }
    return 0;
}

uint8_t encodeHexNybble(int b)
{
    if (b < 10)
        return b + '0';
    return b + 'A' - 10;
}

void sendNextByte()
{
    if (!spaceAvailable)
        return;
    switch (sendState) {
        case 1:
            UDR0 = 'K';  // Success
            sendState = 0;
            break;
        case 2:
            UDR0 = 'E';  // Failure
            sendState = 14;
            break;
        case 14:
            UDR0 = errorCode;
            sendState = 0;
            break;
        case 3:    // First character of HEX line
            UDR0 = ':';
            sendState = 4;
            checkSum = 0;
            break;
        case 4:    // First nybble of HEX byte
            dataByte = getHexByte();
            checkSum += dataByte;
            UDR0 = encodeHexNybble((dataByte >> 4) & 0x0f);
            sendState = 5;
            break;
        case 5:    // Second nybble of HEX byte
            UDR0 = encodeHexNybble(dataByte & 0x0f);
            if (hexLineState == 255)
                sendState = 6;
            else
                sendState = 4;
            break;
        case 6:    // HEX line terminator
            UDR0 = 10;
            if (hexFileState != 255) {
                hexLineState = 0;
                sendState = 3;
            }
            else
                sendState = 1;
            break;
        case 7:    // First nybble of timer tick packet
            UDR0 = encodeHexNybble((timerTickBuffer >> 4) & 0x0f);
            sendState = 8;
            break;
        case 8:
            UDR0 = encodeHexNybble(timerTickBuffer & 0x0f);
            sendState = 9;
            break;
        case 9:    // First nybble of timer tick packet
            UDR0 = encodeHexNybble((picTickHighBuffer >> 4) & 0x0f);
            sendState = 10;
            break;
        case 10:
            UDR0 = encodeHexNybble(picTickHighBuffer & 0x0f);
            sendState = 11;
            break;
        case 11:    // First nybble of timer tick packet
            UDR0 = encodeHexNybble((picTickLowBuffer >> 4) & 0x0f);
            sendState = 12;
            break;
        case 12:
            UDR0 = encodeHexNybble(picTickLowBuffer & 0x0f);
            sendState = 13;
            break;
        case 13:
            UDR0 = ';';
            sendState = 0;
            break;
    }
}

uint8_t success()
{
    sendState = 1;
    receiveState = 0;
    sendNextByte();
    return 1;
}

uint8_t failure(uint8_t code)
{
    sendState = 2;
    errorCode = code;
    receiveState = 0;
    sendNextByte();
    return 0;
}

// Currently there's no code path that calls doWrite(true). We'll add this dangerous ability only if necessary.
uint8_t doWrite(bool plusBackup)
{
    if (plusBackup) {
        enterProgrammingMode();
        for (int16_t pc = 0; pc < 0x201; ++pc)
            incrementAddress();
        bulkErase();
        leaveProgrammingMode();
    }

    enterProgrammingMode();
    if (!plusBackup)
        bulkErase();
    if (!programData(data[0x205]))
        return failure('F');  // Config fuses didn't verify
    for (int16_t pc = 0; pc < 0x205; ++pc) {
        incrementAddress();
        if (plusBackup || pc < 0x200)
            if (!programData(data[pc]))
                return failure('V');  // Program, OSCCAL, user IDs or backup OSCCAL didn't verify
    }
    leaveProgrammingMode();
    return success();
}

uint8_t processHexByte()
{
    switch (hexFileState) {
        case 0:  // byte count
            byteCount = dataByte;
            hexFileState = 1;
            break;
        case 1:  // address high
            address = dataByte << 8;
            hexFileState = 2;
            break;
        case 2:  // address low
            address |= dataByte;
            hexFileState = 3;
            if (address >= 0x1ffe)
                address += 0x40a - 0x1ffe;
            if (address + byteCount > 0x40c)
                return failure('R');  // Out of range
            dataPointer = (uint8_t*)(&data[0]) + address;
            hexFileState = 3;
            break;
        case 3:  // record type
            recordType = dataByte;
            if (recordType > 1 && recordType != 4)
                return failure('U');  // Unknown record type
            if (recordType == 1 && byteCount != 0)
                return failure('Z');  // Non-zero byte count in end-of-file marker
            hexFileState = 4;
            break;
        case 4:  // data byte
            if (recordType == 0)
                *(dataPointer++) = dataByte;
            --byteCount;
            if (byteCount == 0)
                hexFileState = 5;
            break;
        case 5:  // checksum byte
            if (checkSum != 0)
                return failure('C');  // Checksum failure
            if (recordType == 1)
                hexFileState = 255;
            else
                hexFileState = 0;
            break;
    }
    return true;
}

uint8_t decodeHexNybble(uint8_t hex)
{
    if (hex >= '0' && hex <= '9')
        return hex - '0';
    if (hex >= 'A' && hex <= 'F')
        return hex + 10 - 'A';
    if (hex >= 'a' && hex <= 'f')
        return hex + 10 - 'a';
    failure('H');  // Incorrect hex digit
    return 255;
}

uint8_t processCharacter(uint8_t received)
{
    uint8_t nybble;
    switch (receiveState) {
        case 0:  // Default state - expect a command
            switch (received) {
                case 'W':        // W: Write program from buffer to device
                case 'w':
                    doRead(false);  // Read the OSCCAL value
                    return doWrite(false);
                case 'O':        // O: Write program from buffer to device, including OSCCAL value (necessary if write failed, or if we want to use a different OSCCAL value)
                case 'o':
                    return doWrite(false);
                case 'R':        // R: Read program from device to buffer
                case 'r':
                    doRead(true);
                    return success();
                case 'U':        // U: Upload program from host to buffer
                case 'u':
                    receiveState = 1;
                    dataIndex = 0;
                    break;
                case 'L':        // L: Download program from buffer to host
                case 'l':
                    hexLineState = 0;
                    hexFileState = 0;
                    startNextHexLine();
                    sendState = 3;
                    sendNextByte();
                    break;
                case 'T':        // T: Start timing mode
                case 't':
                    raiseVDD();
                    setDataInput();
                    startTimer();
                    return success();
                case 'Q':        // Q: Stop timing mode
                case 'q':
                    lowerVDD();
                    stopTimer();
                    return success();
            }
            break;
        case 1:  // Expect first character of a HEX file line - start code or newline character
            if (received == 10 || received == 13)
                break;
            if (received != ':')
                return failure(':');  // Colon expected
            receiveState = 2;
            checkSum = 0;
            hexFileState = 0;
            break;
        case 2:  // Expect first nybble (high) of HEX file byte
            nybble = decodeHexNybble(received);
            if (nybble == 255)
                return false;
            dataByte = nybble << 4;
            receiveState = 3;
            break;
        case 3:  // Expect second nybble (low) of a HEX file byte
            nybble = decodeHexNybble(received);
            if (nybble == 255)
                return false;
            dataByte |= nybble;
            checkSum += dataByte;
            if (!processHexByte())
                return false;
            if (hexFileState == 255)
                receiveState = 0;
            else
                if (hexFileState == 0)
                    receiveState = 1;
                else
                    receiveState = 2;
            break;
    }
    return true;
}

SIGNAL(USART_RX_vect)
{
    processCharacter(UDR0);
}

SIGNAL(USART_UDRE_vect)
{
    spaceAvailable = true;
    sendNextByte();
}

void sendTimerData()
{
    if (sendState == 0) {
        timerTickBuffer = timerTick;
        picTickLowBuffer = picTickLow;
        picTickHighBuffer = picTickHigh;
        sendState = 7;
        sendNextByte();
    }
}

int main()
{
    // Initialize hardware ports

    // DDRB value:   0x0f  (Port B Data Direction Register)
    //   DDB0           1  Data                        - output
    //   DDB1           2  Clock                       - output
    //   DDB2           4  VPP                         - output
    //   DDB3           8  VDD                         - output
    //   DDB4           0
    //   DDB5           0
    //   DDB6           0
    //   DDB7           0
    DDRB = 0x0f;

    // PORTB value:  0x00  (Port B Data Register)
    //   PORTB0         0  Data                        - low
    //   PORTB1         0  Clock                       - low
    //   PORTB2         0  VPP                         - low
    //   PORTB3         0  VDD                         - low
    //   PORTB4         0
    //   PORTB5         0
    //   PORTB6         0
    //   PORTB7         0
    PORTB = 0;

    // TCCR0A value: 0xa3  (Timer/Counter 0 Control Register A)
    //   WGM00          1  } Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
    //   WGM01          2  }
    //
    //
    //   COM0B0         0  } Compare Output Mode for Channel B: non-inverting mode
    //   COM0B1      0x20  }
    //   COM0A0         0  } Compare Output Mode for Channel A: non-inverting mode
    //   COM0A1      0x80  }
    TCCR0A = 0xa3;

    // TCCR0B value: 0x01  (Timer/Counter 0 Control Register B)
    //   CS00           1  } Clock select: clkIO/1 (no prescaling)
    //   CS01           0  }
    //   CS02           0  }
    //   WGM02          0  Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
    //
    //
    //   FOC0B          0  Force Output Compare B
    //   FOC0A          0  Force Output Compare A
    TCCR0B = 0x01;

    // TIMSK0 value: 0x00  (Timer/Counter 0 Interrupt Mask Register)
    //   TOIE0          0  Timer 0 overflow:  no interrupt
    //   OCIE0A         0  Timer 0 compare A: no interrupt
    //   OCIE0B         0  Timer 0 compare B: no interrupt
    TIMSK0 = 0x00;

    // TIMSK1 value: 0x21  (Timer/Counter 1 Interrupt Mask Register)
    //   TOIE1          1  Timer 1 overflow:  interrupt
    //   OCIE1A         0  Timer 1 compare A: no interrupt
    //   OCIE1B         0  Timer 1 compare B: no interrupt
    //
    //
    //   ICIE1       0x20  Timer 1 input capture: interrupt
    TIMSK1 = 0x00;

    // TIMSK2 value: 0x00  (Timer/Counter 2 Interrupt Mask Register)
    //   TOIE2          0  Timer 2 overflow:  no interrupt
    //   OCIE2A         0  Timer 2 compare A: no interrupt
    //   OCIE2B         0  Timer 2 compare B: no interrupt
    TIMSK2 = 0x00;

    // TCCR1A value: 0x00  (Timer/Counter 1 Control Register A)
    //   WGM10          0  } Waveform Generation Mode = 0 (Normal)
    //   WGM11          0  }
    //
    //
    //   COM1B0         0  } Compare Output Mode for Channel B: normal port operation, OC1B disconnected
    //   COM1B1         0  }
    //   COM1A0         0  } Compare Output Mode for Channel A: normal port operation, OC1A disconnected
    //   COM1A1         0  }
    TCCR1A = 0x00;

    // TCCR1B value: 0x01  (Timer/Counter 1 Control Register B)
    //   CS10           1  } Clock select: clkIO/1 (no prescaling)
    //   CS11           0  }
    //   CS12           0  }
    //   WGM12          0  } Waveform Generation Mode = 0 (Normal)
    //   WGM13          0  }
    //
    //   ICES1          0  Input Capture Edge Select: falling
    //   ICNC1          0  Input Capture Noise Canceler: disabled
    TCCR1B = 0x01;

    // TCCR1C value: 0x00  (Timer/Counter 1 Control Register C)
    //
    //
    //
    //
    //
    //
    //   FOC1B          0  Force Output Compare for Channel B
    //   FOC1A          0  Force Output Compare for Channel A
    TCCR1C = 0x00;

    // UCSR0A value: 0x00  (USART Control and Status Register 0 A)
    //   MPCM0          0  Multi-processor Communcation Mode: disabled
    //   U2X0           0  Double the USART Transmission Speed: disabled
    //
    //
    //
    //
    //   TXC0           0  USART Transmit Complete: not cleared
    UCSR0A = 0x00;

    // UCSR0B value: 0xb8  (USART Control and Status Register 0 B)
    //   TXB80          0  Transmit Data Bit 8 0
    //
    //   UCSZ02         0  Character Size 0: 8 bit
    //   TXEN0          8  Transmitter Enable 0: enabled
    //   RXEN0       0x10  Receiver Enable 0: enabled
    //   UDRIE0      0x20  USART Data Register Empty Interrupt Enable 0: disabled
    //   TXCIE0         0  TX Complete Interrupt Enable 0: disabled
    //   RXCIE0      0x80  RX Complete Interrupt Enable 0: disabled
    UCSR0B = 0xb8;

    // UCSR0C value: 0x06  (USART Control and Status Register 0 C)
    //   UCPOL0         0  Clock Polarity
    //   UCSZ00         2  Character Size: 8 bit
    //   UCSZ01         4  Character Size: 8 bit
    //   USBS0          0  Stop Bit Select: 1-bit
    //   UPM00          0  Parity Mode: disabled
    //   UPM01          0  Parity Mode: disabled
    //   UMSEL00        0  USART Mode Select: asynchronous
    //   UMSEL01        0  USART Mode Select: asynchronous
    UCSR0C = 0x06;

    // UBRR0L value: 0x67  (USART Baud Rate Register Low) - 9600bps
    UBRR0L = 0x67;

    // UBRR0H value: 0x00  (USART Baud Rate Register High)
    UBRR0H = 0x00;

 	sei();

    // waveform program
    data[0x00] = 0x025;
    data[0x01] = 0xc80;
    data[0x02] = 0x002;
    data[0x03] = 0x07f;
    data[0x04] = 0xa05;
    data[0x05] = 0xa06;
    data[0x06] = 0xa07;
    data[0x07] = 0xa08;
    data[0x08] = 0xa09;
    data[0x09] = 0xa0a;
    data[0x0a] = 0x000;
    data[0x0b] = 0x2ff;
    data[0x0c] = 0xa04;
    data[0x0d] = 0xa0e;
    data[0x0e] = 0xa0f;
    data[0x0f] = 0xa10;
    data[0x10] = 0xa11;
    data[0x11] = 0xa12;
    data[0x12] = 0x506;
    data[0x13] = 0x406;
    data[0x14] = 0xa04;

    while (true);
}
