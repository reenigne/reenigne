#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

//typedef uint8_t bool;
//#define true 1
//#define false 0
//
//typedef struct
//{
//    bool bit0:1;
//    bool bit1:1;
//    bool bit2:1;
//    bool bit3:1;
//    bool bit4:1;
//    bool bit5:1;
//    bool bit6:1;
//    bool bit7:1;
//} flags_t;
//
//volatile extern uint8_t lightBuffers[2][0x100];
//volatile extern int8_t waterBuffers[2][0x100];
//volatile extern uint8_t switchBuffer[0x100];
extern uint8_t frameBuffer[336];
extern uint8_t PROGMEM words[700];
extern uint8_t PROGMEM seven[10];
extern uint8_t PROGMEM font[26*8]

//extern uint8_t editor;
//extern uint8_t waveformPreset;
//volatile extern uint8_t sendState;
//volatile extern uint8_t receiveState;
//volatile extern uint16_t debugAddress;
//volatile extern uint8_t debugValue;
//extern uint8_t mode;
//extern uint16_t framesPerBeatOverride;
//extern uint8_t decayConstant;
//extern uint8_t decayConstantOverride;
//extern flags_t flags2;
//extern uint8_t beatsPerPattern;
//extern uint8_t patternsPerLoop;
//extern uint8_t noiseUpdatePointer;
//extern uint8_t microtoneKeys[0x10];
//volatile extern uint8_t serialLED;
//
//extern int8_t PROGMEM sineTable[0x100];
//extern uint16_t PROGMEM tuningMultipliers[0x400];
//extern uint16_t PROGMEM microtoneFrequencies[0x100];
//extern uint8_t PROGMEM escapeMenu[0x20];
//extern uint8_t PROGMEM miscellaneousMenu[0x20];
//extern uint8_t PROGMEM microtoneScreen[0x20];
//extern int8_t PROGMEM speakerPositions[0x10];
//extern uint8_t PROGMEM redLED[0x10];
//extern uint8_t PROGMEM greenLED[0x10];
//extern uint16_t PROGMEM blueLED[0x10];
//
//// To save a few cycles in the interrupt routine, a lot of interesting
//// variables are kept in registers. We need to tell the compiler not to
//// overwrite them.
//register uint8_t const0 __asm__ ("r2");
//register uint8_t sampleInLine __asm__ ("r3");
//register uint8_t lineInFrame __asm__ ("r4");
//register uint8_t frameInBeatLow __asm__ ("r5");
//register uint8_t frameInBeatHigh __asm__ ("r6");
//register uint8_t framesPerBeatLow __asm__ ("r7");
//register uint8_t framesPerBeatHigh __asm__ ("r8");
//register uint8_t beatInPattern __asm__ ("r9");
//register uint8_t lightPage __asm__ ("r10");
//register uint8_t switchInFrame __asm__ ("r11");
//register uint8_t lastSwitch __asm__ ("r12");
//register uint8_t switchesTouched __asm__ ("r13");
//register uint8_t beatsPerPattern __asm__ ("r14");
//register uint8_t waterPage __asm__ ("r15");
register uint8_t random __asm__ ("r16");
//register flags_t flags __asm__ ("r17");
//register uint8_t const1 __asm__ ("r18");
//register uint8_t const0x10 __asm__ ("r19");
//// We don't keep anything interesting in these - we just forbid the compiler
//// from using them so that the interrupt routine doesn't have to save and
//// restore them.
//register uint8_t scratch20 __asm__ ("r20");
//register uint8_t scratch21 __asm__ ("r21");
//register uint8_t scratch22 __asm__ ("r22");
//register uint8_t scratch23 __asm__ ("r23");
//
//#define getLatest(type, reg) ({        \
//    register type val __asm__ (reg);   \
//    __asm__ volatile ("" : "=r"(val)); \
//    val;                               \
//})
//
//#define setLifeMode() __asm__ ("ori r17, 2")
//#define clearLifeMode() __asm__ ("andi r17, 0xfd")
//#define setRandomMode() __asm__ ("ori r17, 0x20")
//#define clearRandomMode() __asm__ ("andi r17, 0xdf")
//#define setPatternMode() __asm__ ("ori r17, 0x40")
//#define clearPatternMode() __asm__ ("andi r17, 0xbf")
//#define setSwitchTouched() __asm__ ("ori r17, 0x80")
//#define clearSwitchTouched() __asm__ ("andi r17, 0x7f")
//
//#define getBeatInPattern() getLatest(uint8_t, "r9")
//#define getLightPage() getLatest(uint8_t, "r10")
//#define getLastSwitch() getLatest(uint8_t, "r12")
//#define getSwitchesTouched() getLatest(uint8_t, "r13")
//#define getWaterPage() getLatest(uint8_t, "r15")
//#define getFlags() getLatest(flags_t, "r17")
//
//#define waitingForSync (flags.bit0)
//#define lifeMode (flags.bit1)
//#define receivedSync (flags.bit2)
//#define outputSyncPulseActive (flags.bit3)
//#define gotRandom (flags.bit4)
//#define randomMode (flags.bit5)
//#define patternMode (flags.bit6)
//#define switchTouched (flags.bit7)
//
//#define getLifeMode() (getFlags().bit1)
//#define getRandomMode() (getFlags().bit5)
//#define getPatternMode() (getFlags().bit6)
//#define getSwitchTouched() (getFlags().bit7)
//
//#define fingerDown (flags2.bit0)
//#define microtoneMode (flags2.bit1)
//#define spaceAvailable (flags2.bit2)
//#define fixedTuning (flags2.bit3)
//#define updateNoise (flags2.bit4)
//#define clearMode (flags2.bit5)
//#define escapeTouched (flags2.bit6)
//
//extern void adcComplete();
//extern uint8_t randomByte();
//
//static const uint8_t normalBrightness = 1;
//static const uint8_t fullBrightness = 15;
//
//uint8_t encodeHex(uint8_t value)
//{
//    if (value < 10)
//        return value + '0';
//    return value + 'A' - 10;
//}
//
//void sendNextByte()
//{
//    if (!spaceAvailable)
//        return;
//    switch (sendState) {
//        case 1:
//            UDR0 = encodeHex(debugValue >> 4);
//            sendState = 2;
//            break;
//        case 2:
//            UDR0 = encodeHex(debugValue & 0x0f);
//            sendState = 0;
//            break;
//        case 3:
//            if (getPatternMode()) {
//                debugValue = waterBuffers[getWaterPage()&1][serialLED] >> 1;
//                if (lightBuffers[getLightPage()&1][serialLED]) {
//                    if ((serialLED & 0x0f) == getBeatInPattern())
//                        debugValue = 0x0f;
//                    else
//                        debugValue += 10;
//                }
//                if (debugValue >= 0x0f && debugValue <= 0x7f)
//                    debugValue = 0x0f;
//                if (debugValue >= 0x80 && debugValue <= 0xff)
//                    debugValue = 0x00;
//            }
//            else
//                debugValue = frameBuffer[serialLED];
//            UDR0 = encodeHex(debugValue);
//            ++serialLED;
//            if (serialLED == 0)
//                sendState = 0;
//            else
//                sendState = 3;
//            break;
//    }
//}
//
//uint8_t decodeHex(uint8_t received)
//{
//    if (received >= '0' && received <= '9')
//        return received-'0';
//    if (received >= 'A' && received <= 'F')
//        return received+10-'A';
//    if (received >= 'a' && received <= 'f')
//        return received+10-'a';
//    return 0xff;
//}
//
//SIGNAL(USART_RX_vect)
//{
//    uint8_t received = UDR0;
//    uint8_t value = decodeHex(received);
//    switch (receiveState) {
//        case 0:
//            if (value != 0xff) {
//                debugAddress = value << 8;
//                receiveState = 1;
//            }
//            else {
//                if (received == 'L') {
//                    serialLED = 0;
//                    sendState = 3;
//                    sendNextByte();
//                }
//                receiveState = 0;
//            }
//            break;
//        case 1:
//            if (value != 0xff) {
//                debugAddress |= value << 4;
//                receiveState = 2;
//            }
//            else
//                receiveState = 0;
//            break;
//        case 2:
//            if (value != 0xff) {
//                debugAddress |= value;
//                receiveState = 3;
//            }
//            else {
//                if (received == 'P') {
//                    debugAddress >>= 4;
//                    if (!switchBuffer[debugAddress]) {
//                        switchBuffer[debugAddress] = true;
//                        switchesTouched = getSwitchesTouched() + 1;
//                        setSwitchTouched();
//                        lastSwitch = debugAddress;
//                    }
//                }
//                else
//                    if (received == 'R') {
//                        debugAddress >>= 4;
//                        if (switchBuffer[debugAddress]) {
//                            switchBuffer[debugAddress] = false;
//                            switchesTouched = getSwitchesTouched() - 1;
//                            clearSwitchTouched();
//                            lastSwitch = debugAddress;
//                        }
//                    }
//                receiveState = 0;
//            }
//            break;
//        case 3:
//            if (received == '?') {
//                debugValue = *(uint8_t*)(debugAddress);
//                receiveState = 0;
//                sendState = 1;
//                sendNextByte();
//                break;
//            }
//            if (received == '=') {
//                receiveState = 4;
//                break;
//            }
//            receiveState = 0;
//            break;
//        case 4:
//            if (value == 0xff) {
//                receiveState = 0;
//                return;
//            }
//            debugValue = value << 4;
//            receiveState = 5;
//            break;
//        case 5:
//            if (value != 0xff) {
//                debugValue |= value;
//                *(uint8_t*)(debugAddress) = debugValue;
//            }
//            receiveState = 0;
//            break;
//    }
//}
//
//SIGNAL(USART_TX_vect)
//{
//    spaceAvailable = true;
//    sendNextByte();
//}
//
//void copySine()
//{
//    for (int i = 0; i < 0x100; ++i)
//        waveform[i] = pgm_read_byte(&sineTable[i]);
//}
//
//void drawMenu(uint8_t* menu)
//{
//    for (int byte = 0; byte < 0x20; ++byte) {
//        uint8_t data = pgm_read_byte(&menu[byte]);
//        for (int column = 0; column < 8; ++column)
//            frameBuffer[(byte<<3) | column] = ((data & (1 << column)) != 0 ? normalBrightness : 0);
//    }
//}
//
//void highlight(uint8_t command)
//{
//    uint8_t topLeft = ((command << 2) & 0x0c) | ((command << 4) & 0xc0);
//    for (uint8_t y = 0; y < 4; ++y)
//        for (uint8_t x = 0; x < 4; ++x) {
//            uint8_t led = topLeft | (y << 4) | x;
//            if (frameBuffer[led] != 0)
//                frameBuffer[led] = 0x0f;
//        }
//}
//
//uint8_t coarseWaveformValue(uint8_t part)
//{
//    part <<= 4;
//    int16_t total = 0x800;
//    for (uint8_t xx = 0; xx < 0x10; ++xx)
//        total += waveform[xx | part];
//    return ((total >> 8) & 0x0f);
//}
//
//void drawEscapeMenu()
//{
//    drawMenu(escapeMenu);
//    highlight(waveformPreset);
//    highlight(editor + 5);
//    if (getLifeMode())
//        highlight(0x0c);
//    if (getRandomMode())
//        highlight(0x0d);
//    if (microtoneMode)
//        highlight(0x0e);
//}
//
//void drawEEPROMScreen()
//{
//    for (int x = 0; x < 0x100; ++x) {
//        bool on = false;
//        uint16_t total = 0;
//        for (uint8_t y = 0; y < 4; ++y) {
//            uint8_t byte = eeprom_read_byte((uint8_t*)((x << 2) | y));
//            if (byte != 0) {
//                total += byte;
//                on = true;
//            }
//        }
//        if (on) {
//            total >>= 6;
//            if (total == 0)
//                total = 1;
//            frameBuffer[x] = total;
//        }
//        else
//            frameBuffer[x] = 0;
//    }
//}
//
//void startPatternEditor()
//{
//    if (microtoneMode) {
//        clearPatternMode();
//        drawMenu(microtoneScreen);
//    }
//    else
//        setPatternMode();
//}
//
//void startCoarseEditor()
//{
//    int x;
//    for (x = 0; x < 0x100; ++x)
//        frameBuffer[x] = 0;
//    for (x = 0; x < 0x10; ++x)
//        frameBuffer[x | (coarseWaveformValue(x) << 4)] = normalBrightness;
//}
//
//void startFineEditor()
//{
//    for (int i = 0; i < 0x100; ++i)
//        frameBuffer[i] = (waveform[i] + 0x80) >> 4;
//}
//
//void startTuningEditor()
//{
//    uint8_t x, y;
//    for (y = 0; y < 0x10; ++y)
//        for (x = 0; x < 0x10; ++x)
//            frameBuffer[(y << 4) | x] = (frequencies[y] & (1 << x)) != 0 ? normalBrightness : 0;
//}
//
//void startMiscellaneousEditor()
//{
//    uint8_t x;
//    drawMenu(miscellaneousMenu);
//    for (x = 0; x < 0x10; ++x)
//        frameBuffer[((x << 2) & 0x30) + (x & 3) + 0x11] = (framesPerBeatOverride & (1 << x)) != 0 ? normalBrightness : 0;
//    for (x = 0; x < 8; ++x)
//        frameBuffer[((x << 2) & 0x30) + (x & 3) + 0x81] = (decayConstantOverride & (1 << x)) != 0 ? normalBrightness : 0;
//    for (x = 0; x < 4; ++x)
//        frameBuffer[((x << 1) & 0x30) + (x & 1) + 0xd1] = (beatsPerPattern & (1 << x)) != 0 ? normalBrightness : 0;
//    for (x = 0; x < 5; ++x)
//        frameBuffer[((x << 4) & 0x70) + 0x1e] = (patternsPerLoop & (1 << x)) != 0 ? normalBrightness : 0;
//    frameBuffer[0x9e] = fixedTuning ? normalBrightness : 0;
//    frameBuffer[0xde] = updateNoise ? normalBrightness : 0;
//}
//
//void startEditor()
//{
//    mode = 0;
//    switch (editor) {
//        case 0: startPatternEditor();       break;
//        case 1: startCoarseEditor();        break;
//        case 2: startFineEditor();          break;
//        case 3: startTuningEditor();        break;
//        case 4: startMiscellaneousEditor(); break;
//    }
//}
//
//void microtoneModeTouch(uint8_t lastSwitch)
//{
//    uint8_t minVolume = 0xff;
//    uint8_t minVolumeChannel = 0;
//    for (uint8_t channel = 0; channel < 0x10; ++channel) {
//        if (microtoneKeys[channel] == lastSwitch) {
//            minVolumeChannel = channel;
//            break;
//        }
//        if (volumes[channel] < minVolume) {
//            minVolume = volumes[channel];
//            minVolumeChannel = channel;
//        }
//    }
//    microtoneKeys[minVolumeChannel] = lastSwitch;
//    volumes[minVolumeChannel] = 0xff;
//}
//
//void patternEditorTouch(uint8_t lastSwitch)
//{
//    if (microtoneMode) {
//        microtoneModeTouch(lastSwitch);
//        return;
//    }
//    // Touch in pattern editor mode
//    uint8_t lightOn = lightBuffers[getLightPage()&1][lastSwitch];
//    if (!fingerDown) {
//        clearMode = lightOn;
//        fingerDown = true;
//    }
//    if (!lightOn) {
//        if (!clearMode) {
//            lightBuffers[getLightPage()&1][lastSwitch] = 1;
//            ++lightsLit;
//            waterBuffers[getWaterPage()&1][lastSwitch] = 16;
//        }
//    }
//    else {
//        if (clearMode) {
//            lightBuffers[getLightPage()&1][lastSwitch] = 0;
//            --lightsLit;
//            waterBuffers[getWaterPage()&1][lastSwitch] = 16;
//        }
//    }
//}
//
//void coarseEditorTouch(uint8_t lastSwitch)
//{
//    // Touch in coarse editor mode
//    uint8_t x = lastSwitch & 0x0f;
//    int8_t position = pgm_read_byte(&speakerPositions[lastSwitch >> 4]);
//    for (uint8_t sample = 0; sample < 0x10; ++sample)
//        waveform[sample + (x << 4)] = position;
//    for (uint8_t y = 0; y < 0x10; ++y)
//        frameBuffer[(y << 4) | x] = 0;
//    frameBuffer[lastSwitch] = normalBrightness;
//}
//
//void fineEditorTouch(uint8_t lastSwitch)
//{
//    uint8_t position = ((waveform[lastSwitch] >> 4) + 9) & 0x0f;
//    waveform[lastSwitch] = pgm_read_byte(&speakerPositions[position]);
//    frameBuffer[lastSwitch] = position;
//}
//
//void tuningEditorTouch(uint8_t lastSwitch)
//{
//    frameBuffer[lastSwitch] = (frameBuffer[lastSwitch] != 0) ? 0 : fullBrightness;
//    if (frameBuffer[lastSwitch] != 0)
//        frequencies[lastSwitch >> 4] |= 1 << (lastSwitch & 0x0f);
//    else
//        frequencies[lastSwitch >> 4] &= ~(1 << (lastSwitch & 0x0f));
//}
//
//void miscellaneousEditorTouch(uint8_t lastSwitch)
//{
//    uint8_t x = lastSwitch & 0x0f;
//    uint8_t y = lastSwitch >> 4;
//    uint8_t z;
//    if (x >= 1 && x <= 4) {
//        if (y >= 1 && y <= 4) {
//            // frames per beat override
//            z = ((y - 1) << 2) | (x - 1);
//            frameBuffer[lastSwitch] = (frameBuffer[lastSwitch] != 0) ? 0 : fullBrightness;
//            if (frameBuffer[lastSwitch] != 0)
//                framesPerBeatOverride |= 1 << z;
//            else
//                framesPerBeatOverride &= ~(1 << z);
//        }
//        if (y == 8 || y == 9) {
//            // envelope decay constant override
//            z = ((y - 8) << 2) | (x - 1);
//            frameBuffer[lastSwitch] = (frameBuffer[lastSwitch] != 0) ? 0 : fullBrightness;
//            if (frameBuffer[lastSwitch] != 0)
//                decayConstantOverride |= 1 << z;
//            else
//                decayConstantOverride &= ~(1 << z);
//        }
//        if (x <= 2 && (y == 0x0d || y == 0x0e)) {
//            // beats per pattern
//            z = ((y - 0x0d) << 2) | (x - 1);
//            frameBuffer[lastSwitch] = (frameBuffer[lastSwitch] != 0) ? 0 : fullBrightness;
//            if (frameBuffer[lastSwitch] != 0)
//                beatsPerPattern |= 1 << z;
//            else
//                beatsPerPattern &= ~(1 << z);
//        }
//    }
//    if (x == 14) {
//        if (y >= 1 && y <= 5) {
//            // patterns per loop
//            z = y - 1;
//            frameBuffer[lastSwitch] = (frameBuffer[lastSwitch] != 0) ? 0 : fullBrightness;
//            if (frameBuffer[lastSwitch] != 0)
//                patternsPerLoop |= 1 << z;
//            else
//                patternsPerLoop &= ~(1 << z);
//        }
//        if (y == 9) {
//            frameBuffer[lastSwitch] = (frameBuffer[lastSwitch] != 0) ? 0 : fullBrightness;
//            if (frameBuffer[lastSwitch] != 0)
//                fixedTuning = true;
//            else
//                fixedTuning = false;
//        }
//        if (y == 0x0d) {
//            frameBuffer[lastSwitch] = (frameBuffer[lastSwitch] != 0) ? 0 : fullBrightness;
//            if (frameBuffer[lastSwitch] != 0)
//                updateNoise = true;
//            else
//                updateNoise = false;
//        }
//    }
//}
//
//void editorTouch(uint8_t lastSwitch)
//{
//    switch (editor) {
//        case 0: patternEditorTouch(lastSwitch);       break;
//        case 1: coarseEditorTouch(lastSwitch);        break;
//        case 2: fineEditorTouch(lastSwitch);          break;
//        case 3: tuningEditorTouch(lastSwitch);        break;
//        case 4: miscellaneousEditorTouch(lastSwitch); break;
//    }
//}
//
//void escapeModeTouch(uint8_t lastSwitch)
//{
//    int i;
//    switch (((lastSwitch & 0x0c) >> 2) | ((lastSwitch & 0xc0) >> 4)) {
//        case 0x00:  // sine
//            waveformPreset = 0;
//            copySine();
//            drawEscapeMenu();
//            break;
//        case 0x01:  // square
//            waveformPreset = 1;
//            for (i = 0; i < 0x80; ++i)
//                waveform[i] = 0x80;
//            for (i = 0; i < 0x80; ++i)
//                waveform[i + 0x80] = 0x7f;
//            drawEscapeMenu();
//            break;
//        case 0x02:  // triangle
//            waveformPreset = 2;
//            for (i = 0; i < 0x80; ++i)
//                waveform[i] = (i << 1) - 0x80;
//            for (i = 0; i < 0x80; ++i)
//                waveform[i + 0x80] = 0x7f - (i << 1);
//            drawEscapeMenu();
//            break;
//        case 0x03:  // sawtooth
//            waveformPreset = 3;
//            for (i = 0; i < 0x100; ++i)
//                waveform[i] = i;
//            drawEscapeMenu();
//            break;
//        case 0x04:  // noise
//            waveformPreset = 4;
//            for (i = 0; i < 0x100; ++i)
//                waveform[i] = randomByte();
//            drawEscapeMenu();
//            break;
//        case 0x05:  // pattern
//            editor = 0;
//            drawEscapeMenu();
//            break;
//        case 0x06:  // coarse
//            editor = 1;
//            drawEscapeMenu();
//            break;
//        case 0x07:  // fine
//            editor = 2;
//            drawEscapeMenu();
//            break;
//        case 0x08:  // tuning
//            editor = 3;
//            drawEscapeMenu();
//            break;
//        case 0x09:  // miscellaneous
//            editor = 4;
//            drawEscapeMenu();
//            break;
//        case 0x0a:  // save
//            mode = 2;
//            drawEEPROMScreen();
//            break;
//        case 0x0b:  // load
//            mode = 3;
//            drawEEPROMScreen();
//            break;
//        case 0x0c:  // life
//            if (getLifeMode())
//                clearLifeMode();
//            else
//                setLifeMode();
//            drawEscapeMenu();
//            break;
//        case 0x0d:  // random
//            if (getRandomMode())
//                clearRandomMode();
//            else
//                setRandomMode();
//            drawEscapeMenu();
//            break;
//        case 0x0e:  // microtone
//            microtoneMode ^= 1;
//            drawEscapeMenu();
//            break;
//        case 0x0f:  // nothing
//            break;
//    }
//}
//
//void savePattern(uint8_t lastSwitch)
//{
//    // Pattern is 16*16/8 = 32 bytes (8 LEDs: 4x2)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xec) << 2);
//    for (uint8_t i = 0; i < 0x20; ++i) {
//        uint8_t x = 0;
//        for (uint8_t y = 0; y < 8; ++y)
//            if (lightBuffers[getLightPage()&1][(i << 3) | y] != 0)
//                x |= (1 << y);
//        eeprom_write_byte(p + ((i & 0x10) << 2) + (i & 0x0f), x);
//    }
//}
//
//void saveCoarse(uint8_t lastSwitch)
//{
//    // Coarse is 8 bytes (2 LEDs: 2x1)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xfe) << 2);
//    for (uint8_t i = 0; i < 8; ++i) {
//        uint8_t x = coarseWaveformValue(i << 1) | (coarseWaveformValue((i << 1) | 1) << 4);
//        eeprom_write_byte(p + i, x);
//    }
//}
//
//void saveFine(uint8_t lastSwitch)
//{
//    // Fine is 128 bytes (32 LEDs: 8x4)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xc8) << 2);
//    for (uint8_t i = 0; i < 0x80; ++i) {
//        uint8_t x = (waveform[i << 1] >> 4) | (waveform[(i << 1) | 1] & 0xf0);
//        eeprom_write_byte(p + ((i & 0x60) << 1) + (i & 0x1f), x^0x88);
//    }
//}
//
//void saveTuning(uint8_t lastSwitch)
//{
//    // Tuning is 32 bytes (8 LEDs: 4x2)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xec) << 2);
//    for (uint8_t i = 0; i < 0x10; ++i) {
//        eeprom_write_byte(p, frequencies[i]);
//        eeprom_write_byte(p + 0x40, frequencies[i] >> 8);
//    }
//}
//
//void saveMiscellaneous(uint8_t lastSwitch)
//{
//    // Miscellaneous is 8 bytes
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xfe) << 2);
//    eeprom_write_byte(p, framesPerBeatOverride);
//    eeprom_write_byte(p + 1, framesPerBeatOverride >> 8);
//    eeprom_write_byte(p + 2, decayConstantOverride);
//    eeprom_write_byte(p + 3, beatsPerPattern);
//    eeprom_write_byte(p + 4, patternsPerLoop);
//    eeprom_write_byte(p + 5, fixedTuning);
//    eeprom_write_byte(p + 6, updateNoise);
//}
//
//void startEscapeMode()
//{
//    mode = 1;
//    clearPatternMode();
//    drawEscapeMenu();
//}
//
//void save(uint8_t lastSwitch)
//{
//    // Turn interrupts off while we save because in pattern
//    // mode the interrupt routine will write to the EEPROM
//    // address register, possibly messing up our writes. This
//    // will lead to audio/visual glitches, but I think that's
//    // a reasonable trade-off. We do need to make sure the
//    // LED current isn't too high for too long but the same
//    // is true at system startup and reprogramming, when the
//    // LEDs could potentially be on for a second or so.
//    cli();
//    switch (editor) {
//        case 0: savePattern(lastSwitch);       break;
//        case 1: saveCoarse(lastSwitch);        break;
//        case 2: saveFine(lastSwitch);          break;
//        case 3: saveTuning(lastSwitch);        break;
//        case 4: saveMiscellaneous(lastSwitch); break;
//    }
//    sei();
//    startEscapeMode();
//}
//
//void loadPattern(uint8_t lastSwitch)
//{
//    // Pattern is 16*16/8 = 32 bytes (8 LEDs: 4x2)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xec) << 2);
//    lightsLit = 0;
//    for (uint8_t i = 0; i < 0x20; ++i) {
//        uint8_t x = eeprom_read_byte(p + ((i & 0x10) << 2) + (i & 0x0f));
//        for (uint8_t y = 0; y < 8; ++y) {
//            bool t = ((x & (1 << y)) != 0);
//            if (t)
//                ++lightsLit;
//            lightBuffers[getLightPage()&1][(i << 3) | y] = (t ? 1 : 0);
//        }
//    }
//}
//
//void loadCoarse(uint8_t lastSwitch)
//{
//    // Coarse is 8 bytes (2 LEDs: 2x1)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xfe) << 2);
//    for (uint8_t i = 0; i < 8; ++i) {
//        uint8_t y = eeprom_read_byte(p + i);
//        for (uint8_t x = 0; x < 0x10; ++x) {
//            waveform[(i << 5) | x] = pgm_read_byte(&speakerPositions[y & 0x0f]);
//            waveform[(i << 5) | x | 0x10] = pgm_read_byte(&speakerPositions[y >> 4]);
//        }
//    }
//}
//
//void loadFine(uint8_t lastSwitch)
//{
//    // Fine is 128 bytes (32 LEDs: 8x4)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xc8) << 2);
//    for (uint8_t i = 0; i < 0x80; ++i) {
//        uint8_t x = eeprom_read_byte(p + ((i & 0x60) << 1) + (i & 0x1f));
//        waveform[i << 1] = pgm_read_byte(&speakerPositions[x & 0x0f]);
//        waveform[(i << 1) | 1] = pgm_read_byte(&speakerPositions[x >> 4]);
//    }
//}
//
//void loadTuning(uint8_t lastSwitch)
//{
//    // Tuning is 32 bytes (8 LEDs: 4x2)
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xec) << 2);
//    for (uint8_t i = 0; i < 0x10; ++i)
//        frequencies[i] = (eeprom_read_byte(p + 0x40) << 8) | eeprom_read_byte(p);
//}
//
//void loadMiscellaneous(uint8_t lastSwitch)
//{
//    // Miscellaneous is 8 bytes
//    uint8_t* p = (uint8_t*)((lastSwitch & 0xfe) << 2);
//    framesPerBeatOverride = eeprom_read_byte(p) | (eeprom_read_byte(p + 1) << 8);
//    decayConstantOverride = eeprom_read_byte(p + 2);
//    beatsPerPattern = eeprom_read_byte(p + 3);
//    patternsPerLoop = eeprom_read_byte(p + 4);
//    fixedTuning = eeprom_read_byte(p + 5);
//    updateNoise = eeprom_read_byte(p + 6);
//}
//
//void load(uint8_t lastSwitch)
//{
//    switch (editor) {
//        case 0: loadPattern(lastSwitch);       break;
//        case 1: loadCoarse(lastSwitch);        break;
//        case 2: loadFine(lastSwitch);          break;
//        case 3: loadTuning(lastSwitch);        break;
//        case 4: loadMiscellaneous(lastSwitch); break;
//            break;
//    }
//    startEscapeMode();
//}
//
//void touch(uint8_t lastSwitch)
//{
//    switch (mode) {
//        case 0: editorTouch(lastSwitch);     break;
//        case 1: escapeModeTouch(lastSwitch); break;
//        case 2: save(lastSwitch);            break;
//        case 3: load(lastSwitch);            break;
//    }
//}
//
//void idleLoop()
//{
//    while (true) {
//        // Is the ADC complete?
//        if ((ADCSRA & 0x40) == 0)
//            adcComplete();
//
//        // Read the escape switch
//        if ((PINB & 0x10) != 0)
//            escapeTouched = false;
//        else
//            if (!escapeTouched) {
//                escapeTouched = true;
//                if (mode != 1)
//                    startEscapeMode();
//                else
//                    startEditor();
//            }
//
//        // Check for a no switches touched condition
//        if (getSwitchesTouched() == 0)
//            fingerDown = false;
//
//        // Attack any microtone channels with touched keys
//        if (microtoneMode)
//            for (uint8_t x = 0; x < 0x10; ++x)
//                if (switchBuffer[microtoneKeys[x]])
//                    volumes[x] = 0xff;
//
//        // Read the switches
//        if (getSwitchTouched()) {
//            // A switch has been touched. Act on it.
//            uint8_t lastSwitch = getLastSwitch();
//            clearSwitchTouched();
//            touch(lastSwitch);
//        }
//
//        // Update the waveform if we're in update noise mode
//        if (updateNoise)
//            waveform[noiseUpdatePointer++] = randomByte();
//        else
//            randomByte();  // Get a new random number anyway to keep it unpredictable
//
//        // Update the RGB LEDs
//        OCR0B = pgm_read_byte(&redLED[getBeatInPattern()]);
//        OCR0A = pgm_read_byte(&greenLED[getBeatInPattern()]);
//        OCR1A = pgm_read_word(&blueLED[getBeatInPattern()]);
//    }
//}


int a[7];
const char *chars;
bool won = false;
int m = 0;

void updateDigit(Byte* frameBuffer, int d)
{
    Byte b = seven[d];
    for (int i = 0; i < 7; ++i)
        frameBuffer[i] = (b & (1 << i)) != 0 ? 255 : 0;
}

void updateFrame()
{
    Byte* screen = &frameBuffer[0];
    int i;
    for (i = 0; i < 7; ++i) {
        Byte* cp = &font[(chars[a[i]] - 'A') << 3];
        Byte* line = screen;
        for (int y = 0; y < 7; ++i) {
            Byte* p = line;
            Byte b = *cp++;
            for (int x = 0; x < 5; ++i) {
                if ((b & (1 << x)) != 0)
                    *p = 255;
                else
                    *p = 0;
                ++p;
            }
            line += 24;
        }
        screen += 5;
    }
    updateDigit(&frameBuffer[8*24 + 15], m/10);
    updateDigit(&frameBuffer[9*24 + 15], m%10);

    for (i = 0; i < 7; ++i)
        if (a[i] != i)
            break;
    if (i == 7)
        won = true;


}

void zPressed()
{
    int t = a[1];
    for (int i = 1; i < 6; ++i)
        a[i] = a[i + 1];
    a[6] = t;
    if (m < 99)
        ++m;
    updateFrame();
}

void xPressed()
{
    t = a[0];
    a[0] = a[1];
    a[1] = t;
    if (m < 99)
        ++m;
    updateFrame();
}

void startPressed()
{
    // Pick a random word and scramble it
    chars = &words[random];
    for (int i = 0; i < 7; ++i)
        a[i] = i;
    do {
        for (int i = 0; i < 7; ++i) {
            int r = random(7);
            int t = a[i];
            a[i] = a[r];
            a[r] = t;
        }
        for (int i = 0; i < 7; ++i)
            if (a[i] != i)
                break;
    } while (i == 7);
    m = 0;
    won = false;
}

