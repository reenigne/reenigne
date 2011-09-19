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
bool getData();
bool getClock();

void sendBit(uint8_t bit)
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

void sendByte(uint8_t data)
{
    while (!getClock() || !getData());
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
}

uint8_t serialBuffer[0x100];
uint8_t receiveBuffer[0x100];
uint8_t programBuffer[0x400];

uint8_t serialBufferPointer;
uint16_t serialBufferCharacters;
uint8_t receiveBufferPointer;
uint16_t receiveBufferCharacters;

bool spaceAvailable = true;

bool shift = false;
bool ctrl = false;
bool alt = false;
bool asciiMode = true;
bool testerMode = false;
bool escape = false;
bool sendBlocked = false;
bool receiveBlocked = false;

void sendNextByte()
{
    if (!spaceAvailable || sendBlkocked)
        return;
    if (serialBufferCharacters > 0) {
        UDR0 = serialBuffer[serialBufferPointer];
        ++serialBufferPointer;
        --serialBufferCharacters;
        spaceAvailable = false;
    }
}

void sendSerialByte(uint8_t byte)
{
    serialBuffer[(serialBufferPointer + serialBufferCharacters) & 0xff] = byte;
    ++serialBufferCharacters;
}

void processCharacter(uint8_t received)
{
    if (received == 0 && !escape) {
        escape = true;
        return;
    }
    if ((received == 17 || received == 19) && !escape) {
        sendBlocked = (received == 19);
        escape = false;
        return;
    }
    escape = false;

    // TODO
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

    do {
        if (!getClock()) {
            sendByte(0xaa);  // 0x65
            wait2us();
            while (!getData());
            wait250ms();
        }
        if (uartByteAvailable())
            sendByte(getByteFromUART());
    } while (true);
}
