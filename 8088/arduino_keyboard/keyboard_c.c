#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

typedef uint8_t bool;
#define true 1
#define false 0

// 1 cycle @ 16MHz = 62.5ns

void raiseClock();
void lowerClock();
void raiseData();
void lowerData();
void setDataInput();
void setDataOutput();
void setClockInput();
void setClockOutput();
void wait2us();
void wait50us();
void wait1ms();
void wait250ms();
bool getData();
bool getClock();
uint8_t receiveKeyboardByte();

extern uint8_t PROGMEM asciiToScancodes[0x5f];

uint8_t serialBuffer[0x100];
uint8_t keyboardBuffer[0x100];
uint8_t programBuffer[0x400];

uint8_t serialBufferPointer;
uint16_t serialBufferCharacters;
uint8_t keyboardBufferPointer;
uint16_t keyboardBufferCharacters;

bool spaceAvailable = true;

bool shift = false;
bool ctrl = false;
bool alt = false;
bool asciiMode = true;
bool testerMode = false;
bool receivedEscape = false;
bool receivedXOff = false;
bool sentXOff = false;
bool needXOff = false;
bool needXOn = false;
uint8_t rawBytesRemaining = 0;
uint16_t programCounter = 0;
uint16_t programBytes = 0;
uint16_t programBytesRemaining = 0;
bool ramProgram = false;
bool expectingRawCount = false;
bool sentEscape = false;

void sendKeyboardBit(uint8_t bit)
{
    if (bit != 0)
        raiseData();
    else
        lowerData();
    wait50us();
    lowerClock();
    wait50us();
    raiseClock();
}

void enqueueKeyboardByte(uint8_t byte)
{
    keyboardBuffer[(keyboardBufferPointer + keyboardBufferCharacters) & 0xff]
        = byte;
    ++keyboardBufferCharacters;
    // If our buffer is getting too full, tell the host to stop sending.
    if (keyboardBufferCharacters >= 0xf0 && !sendXOff) {
        needXOff = true;
        sendSerialByte();
    }
}

void sendSerialByte()
{
    if (!spaceAvailable)
        return;
    // We should be able to send XOn/XOff even if we've received XOff.
    if (!needXOff && !needXOn && receivedXOff)
        return;
    uint8_t c;
    if (needXOff) {
        c = 19;
        sentXOff = true;
        needXOff = false;
    }
    else {
        if (needXOn) {
            c = 17;
            sentXOff = false;
            needXOn = false;
        }
        else {
            if (serialBufferCharacters == 0) {
                // There's nothing we need to send!
                return;
            }
            c = serialBuffer[serialBufferPointer];
            if (c == 0 || c == 17 || c == 19)
                if (!sentEscape) {
                    c = 0;
                    sentEscape = true;
                }
                else {
                    sentEscape = false;
                    ++serialBufferPointer;
                    --serialBufferCharacters;
                }
            }

        }
    }
    // Actually send the byte
    UDR0 = c;
    spaceAvailable = false;
}

void enqueueSerialByte(uint8_t byte)
{
    serialBuffer[(serialBufferPointer + serialBufferCharacters) & 0xff] =
        byte;
    ++serialBufferCharacters;
    sendSerialByte();
}

void processCharacter(uint8_t received)
{
    if (received == 0 && !receivedEscape) {
        receivedEscape = true;
        return;
    }
    if ((received == 17 || received == 19) && !receivedEscape) {
        receivedXOff = (received == 19);
        receivedEscape = false;
        return;
    }
    receivedEscape = false;

    if (expectingRawCount) {
        rawBytesRemaining = received;
        expectingRawCount = false;
        return;
    }
    if (rawBytesRemaining > 0) {
        enqueueKeyboardByte(received);
        --rawBytesRemaining;
        return;
    }
    if (programBytesRemaining == 0xffff) {
        programBytes = received;
        --programBytesRemaining;
        return;
    }
    if (programBytesRemaining == 0xfffe) {
        programBytes |= received << 8;
        programBytesRemaining = programBytes;
        return;
    }
    if (programBytesRemaining > 0) {
        programBuffer[programBytes - programBytesRemaining] = received;
        --programBytesRemaining;
        return;
    }
    if (received == 0) {
        asciiMode = true;
        return;
    }
    if (received == 0x7f) {
        asciiMode = false;
        return;
    }
    if (!asciiMode) {
        switch (received) {
            case 0x71:
                testerMode = false;
                break;
            case 0x72:
                testerMode = true;
                break;
            case 0x73:
                ramProgram = true;
                programBytesRemaining = 0xffff;
                break;
            case 0x74:
                ramProgram = false;
                break;
            case 0x75:
                expectingRawCount = true;
                break;
            default:
                enqueueKeyboardByte(received);
                break;
        }
        return;
    }
    switch (received) {
        case 1:
            testerMode = false;
            break;
        case 2:
            testerMode = true;
            break;
        case 3:
            ramProgram = true;
            programBytesRemaining = 0xffff;
            break;
        case 4:
            ramProgram = false;
            break;
        case 5:
            expectingRawCount = true;
            break;
        default:
            {
                uint8_t scanCode;
                switch (received) {
                    case 8:
                        scanCode = 0x0e;
                        break;
                    case 9:
                        scanCode = 0x0f;
                        break;
                    case 10:
                        // We handle LF and CRLF line endings by treating LF
                        // as a press of Enter and ignoring CR. Revisit if
                        // this causes problems.
                        scanCode = 0x1c;
                        break;
                    case 27:
                        scanCode = 0x01;
                        break;
                    default:
                        if (received >= 0x20 && received <= 0x7e) {
                            scanCode = pgm_read_byte(
                                &asciiToScancodes[received - 0x20]);
                        }
                        else {
                            // Handle invalid ASCII codes by ignoring them.
                            return;
                        }
                        // TODO: Add codes for cursor movement, function keys
                        // etc.
                }
                bool shifted = (scancode & 0x80) != 0;
                if (shifted != shift) {
                    // We always use the left shift key for typing shifted
                    // characters.
                    enqueueKeyboardByte(0x2a | (shifted ? 0 : 0x80));
                    shift = shifted;
                }
                enqueueKeyboardByte(scancode);
                enqueueKeyboardByte(scancode | 0x80);
            }
            break;
    }
}

SIGNAL(USART_RX_vect)
{
    processCharacter(UDR0);
}

SIGNAL(USART_UDRE_vect)
{
    spaceAvailable = true;
    sendSerialByte();
}

void clearInterruptedKeystroke()
{
    setClockOutput();
    while (getData()) {
        lowerClock();
        wait50us();
        raiseClock();
        wait50us();
    }
    setClockInput();
}


bool sendKeyboardByte(uint8_t data)
{
    // We read the clock as high immediately before entering this routine.
    // The XT keyboard hardware holds the data line low to signal that the
    // previous byte has not yet been acknowledged by software.
    while (!getData());
    if (!getClock()) {
        // Uh oh - the clock went low - the XT wants something (send byte or
        // reset). This should never happen during a reset, so we can just
        // abandon this byte.
        return false;
    }
    setClockOutput();
    setDataOutput();
    sendBit(0);
    sendBit(1);
    for (uint8_t i = 0; i < 8; ++i) {
        sendBit(data & 1);
        data >>= 1;
    }
    raiseData();
    setClockInput();
    setDataInput();
    if (!getClock()) {
        // The clock went low while we were sending - retry this byte once
        // the reset or send-requested condition has been resolved.
        return false;
    }
    // The byte went through.
    return true;
}

int main()
{
    // Initialize hardware ports

    // DDRB value:   0x00  (Port B Data Direction Register)
    //   DDB0           0  Data                        - input
    //   DDB1           0  Clock                       - input
    //   DDB2           0
    //   DDB3           0
    //   DDB4           0
    //   DDB5           0
    //   DDB6           0
    //   DDB7           0
    DDRB = 0x00;

    // PORTB value:  0x03  (Port B Data Register)
    //   PORTB0         1  Data                        - high
    //   PORTB1         2  Clock                       - high
    //   PORTB2         0
    //   PORTB3         0
    //   PORTB4         0
    //   PORTB5         0
    //   PORTB6         0
    //   PORTB7         0
    PORTB = 3;

    DDRD |= 0x60;

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
    //   UDRIE0      0x20  USART Data Register Empty Interrupt Enable 0:
    //                       disabled
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

    // All the keyboard interface stuff is done on the main thread.
    do {
        if (!getClock()) {
            wait1ms();
            if (!getClock()) {
                // If the clock line is held low for this long it means the XT
                // is resetting the keyboard.
                while (!getClock());  // Wait for clock to go high again.
                // There are 4 different things the BIOS recognizes here:
                // 0xaa - a keyboard
                // 0x65 - tester doodad: download the code from it and run it.
                // 0x00 - boot in test mode - this affects the diagnostic
                //   beeps and flashes an LED via the keyboard clock line. I
                //   don't think it's useful for this program.
                // Everything else - keyboard error. Also not useful.
                if (testerMode) {
                    // We assume the BIOS won't be foolish enough to pull the
                    // clock line low during a reset. If it does the data will
                    // be corrupted as we don't attempt to remember where we
                    // got to and retry from there.
                    sendKeyboardByte(0x65);
                    if (ramProgram) {
                        sendKeyboardByte(programBytes & 0xff);
                        sendKeyboardByte(programBytes >> 8);
                        for (uint16_t i = 0; i < programBytes; ++i)
                            sendKeyboardByte(programBuffer[i]);
                    }
                    else {
                        uint16_t programBytes =
                            pgm_read_byte(&defaultProgram[0]);
                        programBytes |=
                            (uint16_t)(pgm_read_byte(&defaultProgram[1]))
                            << 8;
                        for (uint16_t i = 0; i < programBytes; ++i)
                            sendKeyboardByte(
                                pgm_read_byte(&defaultProgram[i+2]));
                    }
                }
                else {
                    sendKeyboardByte(0xaa);
                    wait2us();
                    while (!getData());
                    // If we send anything in the first 250ms after a reset,
                    // the BIOS will assume it's a stuck key.
                    wait250ms();
                }
                // End of reset code
            }
            else {
                // A short clock-low pulse. This is the XT trying to send us
                // some data.
                while (!getClock());  // Wait for the clock to go high again.
                clearInterrupted();  // Clear out any partial keystrokes
                sendKeyboardByte(0x00);  // Clear to send
                // Send the number of bytes that the XT can safely send us.
                sendKeyboardByte(serialBufferCharacters == 0 ? 255 :
                    256-serialBufferCharacters);
                uint8_t count = receiveKeyboardByte();
                for (uint8_t i = 0; i < count; ++i)
                    enqueueSerialByte(receiveKeyboardByte());
            }
        }
        else {
            // Clock is high - we're free to send if we have data.
            if (keyboardBufferCharacters != 0) {
                if (sendKeyboardByte(keyboardBuffer[keyboardBufferPointer])) {
                    // Successfully sent - remove this character from the
                    // buffer.
                    ++keyboardBufferPointer;
                    --keyboardBufferCharacters;
                    // If we've made enough space in the buffer, allo
                    //  receiving again.
                    if (keyboardBufferCharacters < 0xf0 && sentXOff) {
                        needXOn = true;
                        sendSerialByte();
                    }
                }
            }
        }
    } while (true);
}
