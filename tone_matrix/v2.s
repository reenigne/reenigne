; Some useful macros

.macro unroll macro
  \macro 0x00
  \macro 0x01
  \macro 0x02
  \macro 0x03
  \macro 0x04
  \macro 0x05
  \macro 0x06
  \macro 0x07
  \macro 0x08
  \macro 0x09
  \macro 0x0a
  \macro 0x0b
  \macro 0x0c
  \macro 0x0d
  \macro 0x0e
  \macro 0x0f
.endm


; Generates a random number in the range 0..r27-1 and returns it in r22.
; Takes 103-116 cycles.
random:                          ; 3  (for rcall)
  ; Generate 16-bit random number (56 cycles)
  lds r28, randomData            ; 2  a0
  lds r29, randomData+1          ; 2  a1
  lds r30, randomData+2          ; 2  a2
  lds r31, randomData+3          ; 2  a3
  ldi r24, 0xfd                  ; 1  b0
  ldi r25, 0x43                  ; 1  b1
  ldi r26, 0x03                  ; 1  b2
  mul r28, r24                   ; 2  r1:r0 = a0*b0
  movw r20, r0                   ; 1  c1:c0 = a0*b0
  mul r29, r25                   ; 2  r1:r0 = a1*b1
  movw r22, r0                   ; 1  c3:c2 = a1*b1
  mul r30, r24                   ; 2  r1:r0 = a2*b0
  add r22, r0                    ; 1
  adc r23, r1                    ; 1  c3:c2 += a2*b0

  mul r28, r26                   ; 2  r1:r0 = a0*b2
  add r22, r0                    ; 1
  adc r23, r1                    ; 1  c3:c2 += a0*b2

  mul r31, r24                   ; 2  r1:r0 = a3*b0
  add r23, r0                    ; 1  c3 += a3*b0

  mul r30, r25                   ; 2  r1:r0 = a2*b1
  add r23, r0                    ; 1  c3 += a2*b1

  mul r29, r26                   ; 2  r1:r0 = a1*b2
  add r23, r0                    ; 1  c3 += a1*b2

  mul r29, r24                   ; 2  r1:r0 = a1*b0
  add r21, r0                    ; 1
  adc r22, r1                    ; 1
  adc r23, r2                    ; 1  c3:c2:c1 += a1*b0

  mul r28, r25                   ; 2  r1:r0 = a0*b1
  add r21, r0                    ; 1
  adc r22, r1                    ; 1
  adc r23, r2                    ; 1  c3:c2:c1 += a0*b1

  subi r20, 0x3d                 ; 1
  sbci r21, 0x61                 ; 1
  sbci r22, 0xd9                 ; 1
  sbci r23, 0xff                 ; 1  c3:c2:c1:c0 += 0x00269ec3
  sts randomData, r20            ; 2
  sts randomData+1, r21          ; 2
  sts randomData+2, r22          ; 2
  sts randomData+3, r23          ; 2  randomData = c3:c2:c1:c0

  ; We now have a 16-bit random number in r23:r22
  ; r25:r24 = M1:M0
  ; r27 = n
  ; r29:r28:r26 = p3:p2:p1

  ; Multiply c3:c2 by M and jump to the appropriate helper function (37 cycles)
  mov r30, r27                         ; 1  lo8(Z) = n
  mov r31, r2                          ; 1  hi8(Z) = 0
  add r30, r30                         ; 1
  adc r31, r31                         ; 1  Z <<= 1
  subi r31, hi8(-(divisionMultiplier)) ; 1  Z = &divisionMultiplier[n]
  lpm r24, Z+                          ; 3
  lpm r25, Z                           ; 3
  mul r22, r24                         ; 2  r1:r0 = c2*M0
  mov r26, r1                          ; 1  p1 = (c2*M0)>>8
  mul r23, r25                         ; 2  r1:r0 = c3*M1
  movw r28, r0                         ; 1  p3:p2 = c3*M1
  mul r22, r25                         ; 2  r1:r0 = c2*M1
  add r26, r0                          ; 1
  adc r28, r1                          ; 1
  adc r29, r2                          ; 1  p3:p2:p1 += c2*M1
  mul r23, r24                         ; 2  r1:r0 = c3*M0
  add r26, r0                          ; 1
  adc r28, r1                          ; 1
  adc r29, r2                          ; 1  p3:p2:p1 += c2*M1
  subi r30, 1                          ; 1  --Z
  subi r31, hi8(divisionMultiplier-divisionHelper) ; 1  Z = &divisionHelper[n]
  lpm r24, Z+                          ; 3
  lpm r25, Z                           ; 3  r23:r22 = divisionHelper[n]
  movw r30, r24                        ; 1  Z = divisionHelper[n]
  ijmp                                 ; 2  goto divisionHelper[n]

divAS0:               ; 4
  add r28, r22                         ; 1
  adc r29, r23                         ; 1  p3:p2 += c
  rjmp divNS0                          ; 2
divAS3:               ; 10
  add r28, r22                         ; 1
  adc r29, r23                         ; 1  p3:p2 += c
  rjmp divNS3                          ; 2
divAS4:               ; 11
  add r28, r22                         ; 1
  adc r29, r23                         ; 1  p3:p2 += c
  rjmp divNS4                          ; 2
divAS5:               ; 13
  add r28, r22                         ; 1
  adc r29, r23                         ; 1  p3:p2 += c
  rjmp divNS5                          ; 2
divAS6:               ; 13
  add r28, r22                         ; 1
  adc r29, r23                         ; 1  p3:p2 += c
  rjmp divNS6                          ; 2
divAS7:               ; 10
  add r28, r22                         ; 1
  adc r29, r23                         ; 1  p3:p2 += c
  rjmp divNS7                          ; 2
divAS8:               ; 5
  add r28, r22                         ; 1
  adc r29, r23                         ; 1  p3:p2 += c
  mov r28, r29                         ; 1  p2 = p3
  rjmp divNS0                          ; 2
divNS7:               ; 6
  lsl r29                              ; 1
  bst r28, 7                           ; 1
  bld r29, 0                           ; 1
  mov r28, r29                         ; 1  p2 = (p3:p2) >> 7
  rjmp divNS0                          ; 2
divNS6:               ; 9
  lsl r29                              ; 1
  lsl r29                              ; 1
  bst r28, 7                           ; 1
  bld r29, 1                           ; 1
  bst r28, 6                           ; 1
  bld r29, 0                           ; 1
  mov r28, r29                         ; 1  p2 = (p3:p2) >> 6
  rjmp divNS0                          ; 2
divNS5:               ; 9
  lsr r29                              ; 1
  ror r28                              ; 1  p2 = (p3:p2) >> 5
divNS4:               ; 7
  swap r29                             ; 1
  swap r28                             ; 1
  andi r28, 0x0f                       ; 1
  andi r29, 0xf0                       ; 1
  or r28, r29                          ; 1  p2 = (p3:p2) >> 4
  rjmp divNS0                          ; 2
divNS3:               ; 6
  lsr r29                              ; 1
  ror r28                              ; 1  p2 = (p3:p2) >> 3
divNS2:               ; 4
  lsr r29                              ; 1
  ror r28                              ; 1  p2 = (p3:p2) >> 2
divNS1:               ; 2
  lsr r29                              ; 1
  ror r28                              ; 1  p2 = (p3:p2) >> 1
divNS0:               ; 0
  mul r28, r27                         ; 2  r1:r0 = p2*n
  sub r22, r0                          ; 1  c2 -= p2*n
  ret                                  ; 4


; Timer 1 interrupt vector. Executes 15,625 times per second (every 1024 cycles - so it must not take longer than this).
; Here where we do all the things which must be done at a particular time.
; The LED output, audio output and switch reading code is mixed up so that we don't need to explicitly wait for the SPI transmits and the switch column selection.

;SIGNAL(TIMER1_OVF_vect)
;{
.global __vector_13              ; 7
__vector_13:  ; TIMER1_OVF_vect
  push r31                       ; 2
  push r30                       ; 2
  push r29                       ; 2
  push r28                       ; 2
  push r27                       ; 2
  push r26                       ; 2
  push r25                       ; 2
  push r24                       ; 2
  push r1                        ; 2
  push r0                        ; 2
  in r0, 0x3f                    ; 1
  push r0                        ; 2

;    // Output the LED data
;    uint16_t leds = 0;
;    uint16_t rowRegister = 1 << lineInFrame;
;    for (uint8_t column = 0; column < 0x10; ++column)
;        if ((lineBuffer[column]--) > 0)
;            leds |= (1 << column);
;    SPDR = rowRegister >> 8;
;    // delay at least 16 cycles
;    SPDR = rowRegister;
;    // delay at least 16 cycles
;    SPDR = leds >> 8;
;    // delay at least 16 cycles
;    SPDR = leds;
;    // delay at least 16 cycles

  ldi r31, hi8(rowRegisterData)  ; 1  hi8(Z) = hi8(rowRegisterData)
  ldi r30, lo8(rowRegisterData)  ; 1  lo8(Z) = lo8(rowRegisterData)
  add r30, r4                    ; 1  lo8(Z) = lo8(rowRegisterData) + lineInFrame
  add r30, r4                    ; 1  lo8(Z) = lo8(rowRegisterData) + 2*lineInFrame
  lpm r23, Z+                    ; 3  r23 = lo8(rowRegisterData[lineInFrame])
  out 0x2e, r23                  ; 1  SPDR = r23  (Output shift register data for lower rows. Need to wait at least 16 cycles before loading next byte.)
  lpm r23, Z                     ; 2  r23 = hi8(rowRegisterData[lineInFrame])

  ldi r30, lo8(lineBuffer)       ; 1  lo8(Z) = lo8(lineBuffer) = 0x70
  ldi r31, hi8(lineBuffer)       ; 1  hi8(Z) = hi8(lineBuffer) = 0x02

  eor r25, r25                   ; 1  r25 = 0

.macro illuminate column
  ldd r24, Z+\column             ; 2    r24 = lineBuffer[column]
  cp r2, r24                     ; 1    if (lineBuffer[column] <= 0)
  brlt 1f                        ; 2 1
  ori r25, 1<<(\column&7)        ; 0 1      r25 |= (1<<column)
1:
  subi r24, 1                    ; 1    --r24
  std Z+\column, r24             ; 2    --lineBuffer[column]
.endm

  illuminate 0x08                ; 8
  illuminate 0x09                ; 8
  illuminate 0x0a                ; 8
  illuminate 0x0b                ; 8

  out 0x2e, r23                  ; 1  SPDR = r23  (Output shift register data for upper rows.)

  illuminate 0x0c                ; 8
  illuminate 0x0d                ; 8
  illuminate 0x0e                ; 8
  illuminate 0x0f                ; 8

  out 0x2e, r25                  ; 1  SPDR = r25  (Output shift register data for right columns.)
  eor r25, r25                   ; 1

  illuminate 0x00                ; 8
  illuminate 0x01                ; 8
  illuminate 0x02                ; 8
  illuminate 0x03                ; 8
  illuminate 0x04                ; 8
  illuminate 0x05                ; 8
  illuminate 0x06                ; 8
  illuminate 0x07                ; 8

  out 0x2e, r25                  ; 1  SPDR = r25  (Output shift register data for left columns.)

;    // Output the audio data
;    int32_t total = 0;

  eor r22, r22                   ; 1  r22 = 0
  eor r23, r23                   ; 1  r23 = 0
  eor r24, r24                   ; 1  r24 = 0
  ldi r30, lo8(positions)        ; 1  lo(Z) = lo8(positions) = 0x40  (hi8(positions) == 0x02 is already in r31 == hi8(Z))
  ldi r29, hi8(volumes)          ; 1  hi8(Y) = hi8(volumes) = 0x02
  ldi r28, lo8(volumes)          ; 1  lo8(Y) = lo8(volumes) = 0x80
  ldi r27, hi8(waveform)         ; 1  hi8(X) = hi8(waveform) = 0x01  (lo8(waveform) == 0x00)

;    for (uint8_t channel = 0; channel < 0x10; ++channel) {
;        positions[channel] += velocities[channel];
;        total += waveform[positions[channel]>>8]*volumes[channel];
;    }

  ; Z(r31:r30)  = channels - array of (position, velocity) structs
  ; Y(r29:r28)  = volumes
  ; X(r27:r26)  = wave data pointer (r27 = wave data page number (wave must be 256-byte aligned))
  ; r26:r25     = new position (overlaps with X deliberately)
  ; r24:r23:r22 = total sample
  ; r20         = sample value, velocity low
  ; r21         = sample volume, velocity high

.macro mix channel
  ldd r25, Z+\channel*2          ; 2   r25 = lo8(positions[channel])
  ldd r26, Z+\channel*2+1        ; 2   r26 = hi8(positions[channel])  Load position
  ldd r20, Z+0x20+\channel*2     ; 2   r20 = lo8(velocities[channel])
  ldd r21, Z+0x21+\channel*2     ; 2   r21 = hi8(velocities[channel])  Load velocity
  add r25, r20                   ; 1
  adc r26, r21                   ; 1   r26:r25 += r21:r20  (position += velocity)
  std Z+\channel*2, r25          ; 2   lo8(positions[channel]) = r25
  std Z+\channel*2+1, r26        ; 2   hi8(positions[channel]) = r26  Store position  (positions[channel] += velocities[channel])
  ld r20, X                      ; 2   r20 = waveform[r27:r26]  Load wave data
  ldd r21, Y+\channel            ; 2   r21 = volumes[channel]  Load volume
  mulsu r20, r21                 ; 2   r1:r0 = waveform[positions[channel]>>8]*volumes[channel]
  add r22, r0                    ; 1
  adc r23, r1                    ; 1
  adc r24, r2                    ; 1
  sbrc r1, 7                     ; 1 2
  subi r24, 1                    ; 1 0 r24:r23:r22 += r1:r0  (total += sample*volume)
.endm

  mix 0x00                       ; 25

;    PORTB = 0x11;  // output shift register reset low
;    PORTB = 0x15;  // output shift register reset high

  ; That last shift register should be done by now
  ldi r25, 0x11                  ; 1
  out 0x05, r25                  ; 1   PORTB = 0x11  (output shift register reset low)
  ldi r25, 0x15                  ; 1
  out 0x05, r25                  ; 1   PORTB = 0x15  (output shift register reset high)

  mix 0x01                       ; 25
  mix 0x02                       ; 25
  mix 0x03                       ; 25
  mix 0x04                       ; 25
  mix 0x05                       ; 25
  mix 0x06                       ; 25
  mix 0x07                       ; 25
  mix 0x08                       ; 25
  mix 0x09                       ; 25
  mix 0x0a                       ; 25
  mix 0x0b                       ; 25
  mix 0x0c                       ; 25
  mix 0x0d                       ; 25
  mix 0x0e                       ; 25
  mix 0x0f                       ; 25

;    OCR2B = (total >> 12) + 0x80;

  ;    r24        r23        r22
  ; .... ....  .... ....  .... ....
  ;     |----------|
  swap r23                       ; 1
  swap r24                       ; 1
  andi r23, 0x0f                 ; 1
  andi r24, 0xf0                 ; 1
  or r23, r24                    ; 1  r23 = (r24:r23) >> 4  (= signed sample (80..7f))
  subi r23, -0x80                ; 1  r23 += 0x80  (TODO: add this to timings)
  sts 0xb4, r23                  ; 2  OCR2B = r23  (output final sample)

;    // Read the switches phase II
;    bool touched;
;    if ((sampleInLine & 8) == 0)
;        touched = ((PIND & 0x10) != 0);
;    else
;        touched = ((PIND & 0x80) != 0);
;    if (touched) {
;        if (!switchBuffer[switchInFrame]) {
;            switchBuffer[switchInFrame] = true;
;            ++switchesTouched;
;            switchTouched = true;
;            lastSwitch = switchInFrame;
;        }
;    }
;    else {
;        if (switchBuffer[switchInFrame]) {
;            switchBuffer[switchInFrame] = false;
;            --switchesTouched;
;            switchTouched = false;
;            lastSwitch = switchInFrame;
;        }
;    }

; Left switch touched new    16
; Left switch touched old    12
; Left switch untouched new  17
; Left switch untouched old  11
; Right switch touched new   16
; Right switch touched old   12
; Right switch untouched new 19
; Right switch untouched old 13

  ldi r31, hi8(switchBuffer)     ; 1 1 1 1 1 1 1 1  hi8(Z) = hi8(switchBuffer)
  mov r30, r11                   ; 1 1 1 1 1 1 1 1  lo8(Z) = switchInFrame
  ld r26, Z                      ; 2 2 2 2 2 2 2 2  r26 = switchBuffer[switchInFrame]

  mov r24, r3                    ; 1 1 1 1 1 1 1 1  r24 = sampleInLine
  inc r24                        ; 1 1 1 1 1 1 1 1  r24 = sampleInLine + 1

  in r23, 0x09                   ; 1 1 1 1 1 1 1 1  r23 = PIND  (Read switch bits (bits 4 and 7 of port D for left and right halves respectively)

  out 0x08, r24                  ; 1 1 1 1 1 1 1 1  PORTC = sampleInLine + 1   (low 3 bits of next column number - give it a full 1023 cycles to settle down))

  sbrc r3, 3                     ; 2 2 2 2 1 1 1 1  if (sampleInLine & 3)
  rjmp checkRightSwitches        ; 0 0 0 0 2 2 2 2      goto checkRightSwitches
  ; Check left switches
  sbrs r23, 4                    ; 1 1 2 2 0 0 0 0  if ((PIND & 0x10) == 0)
  rjmp switchTouched             ; 2 2 0 0 0 0 0 0      goto switchTouched
switchUntouched:                 ;                  else
  ; Switch is not touched. Is this new?
  sbrs r26, 0                    ; 0 0 2 1 0 0 2 1      if ((switchBuffer[switchInFrame] & 1) != 0)
  rjmp updateWater               ; 0 0 0 2 0 0 0 2
  ; Switch was touched last frame and now is not touched.
  st Z, r2                       ; 0 0 2 0 0 0 2 0          switchBuffer[switchInFrame] = 0
  dec r13                        ; 0 0 1 0 0 0 1 0          --switchesTouched
  andi r17, 0x7f                 ; 0 0 1 0 0 0 1 0          switchTouched = false
  mov r12, r11                   ; 0 0 1 0 0 0 1 0          lastSwitch = switchInFrame
  rjmp updateWater               ; 0 0 2 0 0 0 2 0
checkRightSwitches:
  ; Check right switches
  sbrc r23, 7                    ; 0 0 0 0 2 2 1 1  if ((PIND & 0x80) != 0)
  rjmp switchUntouched           ; 0 0 0 0 0 0 2 2      goto switchUntouched
switchTouched:                   ;                  else
  ; Switch is touched. Is this new?
  sbrc r26, 0                    ; 2 1 0 0 2 1 0 0      if ((switchBuffer[switchInFrame] & 1) == 0)
  rjmp updateWater               ; 0 2 0 0 0 2 0 0
  ; Switch was not touched last frame and now is touched.
  st Z, r18                      ; 2 0 0 0 2 0 0 0          switchBuffer[switchInFrame] = 1
  inc r13                        ; 1 0 0 0 1 0 0 0          ++switchesTouched
  ori r17, 0x80                  ; 1 0 0 0 1 0 0 0          switchTouched = true
  mov r12, r11                   ; 1 0 0 0 1 0 0 0          lastSwitch = switchInFrame

;    // Update water
;    if (lightBuffers[lightPage][switchInFrame] != 0 && sampleInLine == beatInPattern && (frameInBeat>>8) < 4) {
;        // Put energy into the water as we play
;        waterBuffers[waterPage^1][switchInFrame] = 16;
;    }
;    else {
;        // Allow the energy to spread out
;        uint16_t total = 0;
;        if (sampleInLine > 0)
;            total = waterBuffers[waterPage][switchInFrame - 1];
;        if (sampleInLine < 0x0f)
;            total += waterBuffers[waterPage][switchInFrame + 1];
;        if (lineInFrame > 0)
;            total += waterBuffers[waterPage][switchInFrame - 0x10];
;        if (lineInFrame < 0x0f)
;            total += waterBuffers[waterPage][switchInFrame + 0x10];
;        total >>= 1;
;        total -= waterBuffers[waterPage^1][switchInFrame];
;        total -= (total >> 8);
;        waterBuffers[waterPage^1][switchInFrame] = total;
;    }

updateWater:
  mov r31, r10                   ; 1 1 1 1  hi8(Z) = lightPage
  ld r25, Z                      ; 2 2 2 2  Z = lightBuffers[lightPage][switchInFrame]
  mov r31, r15                   ; 1 1 1 1  hi8(Z) = waterPage
  tst r25                        ; 1 1 1 1  if (!lightOn)
  breq waveEquation              ; 2 1 1 1      goto waveEquation
  cp r3, r9                      ; 0 1 1 1  if (sampleInLine != beatInPattern)
  brne waveEquation              ; 0 2 1 1      goto waveEquation
  mov r24, r6                    ; 0 0 1 1  r24 = frameInBeat >> 8
  cpi r24, 4                     ; 0 0 1 1  if ((frameInBeat >> 8) >= 4)
  brsh waveEquation              ; 0 0 2 1      goto waveEquation
  ldi r23, 16                    ; 0 0 0 1  r23 = 16
  eor r31, r18                   ; 0 0 0 1  hi8(Z) ^= 1
  rjmp storeWaterLevel           ; 0 0 0 2  goto storeWaterLevel
waveEquation:
  ldi r24, 0                     ; 1
  ldi r23, 0                     ; 1        r24:r23 = 0  (total = 0)
  subi r30, 1                    ; 1        lo8(Z) = switchInFrame - 1
  cp r3, r2                      ; 1        if (sampleInLine != 0)
  breq doneLeft                  ; 2 1
  ld r23, Z                      ; 0 2          r23 = waterBuffers[waterPage][switchInFrame - 1]
  sbrc r23, 7                    ; 0 1 2
  subi r24, 1                    ; 0 1 0        r24:r23 = waterBuffers[waterPage][switchInFrame - 1]  (sign extend)
doneLeft:
  mov r21, r3                    ; 1
  subi r30, -2                   ; 1        lo8(Z) = switchInFrame + 1
  cpi r21, 15                    ; 1        if (sampleInLine != 0x0f)
  breq doneRight                 ; 2 1
  ld r25, Z                      ; 0 2          r25 = waterBuffers[waterPage][switchInFrame + 1]
  muls r25, r18                  ; 0 2          r1:r0 = waterBuffers[waterPage][switchInFrame + 1]*1 (sign extend)
  add r23, r0                    ; 0 1
  adc r24, r1                    ; 0 1          r24:r23 += waterBuffers[waterPage][switchInFrame + 1]
doneRight:
  mov r20, r4                    ; 1
  subi r30, 0x11                 ; 1        lo8(Z) = switchInFrame - 0x10
  cpi r20, 0                     ; 1        if (lineInFrame != 0)
  breq doneUp                    ; 2 1
  ld r25, Z                      ; 0 2          r25 = waterBuffers[waterPage][switchInFrame - 0x10]
  muls r25, r18                  ; 0 2          r1:r0 = waterBuffers[waterPage][switchInFrame - 0x10]*1 (sign extend)
  add r23, r0                    ; 0 1
  adc r24, r1                    ; 0 1          r24:r23 += waterBuffers[waterPage][switchInFrame - 0x10]
doneUp:
  subi r30, -0x20                ; 1        lo8(Z) = switchInFrame + 0x10
  cpi r20, 15                    ; 1        if (lineInFrame != 0x0f)
  breq doneDown                  ; 2 1
  ld r25, Z                      ; 0 2          r25 = waterBuffers[waterPage][switchInFrame + 0x10]
  muls r25, r18                  ; 0 2          r1:r0 = waterBuffers[waterPage][switchInFrame + 0x10]*1 (sign extend)
  add r23, r0                    ; 0 1
  adc r24, r1                    ; 0 1          r24:r23 += waterBuffers[waterPage][switchInFrame - 0x10]
doneDown:
  subi r30, 0x10                 ; 1        lo8(Z) = switchInFrame
  asr r24                        ; 1
  ror r23                        ; 1        r24:r23 >>= 1  (total >>= 1)
  eor r31, r18                   ; 1        hi8(Z) ^= 1
  ld r25, Z                      ; 2        r25 = waterBuffers[1-waterPage][switchInFrame]
  muls r25, r18                  ; 2        r1:r0 = waterBuffers[1-waterPage][switchInFrame]*1 (sign extend)
  sub r23, r0                    ; 1
  sbc r24, r1                    ; 2        r24:r23 -= waterBuffers[1-waterPage][switchInFrame]
  sub r23, r24                   ; 2        total -= (total >> 8)
storeWaterLevel:
  st Z, r23                      ; 2        waterBuffers[1-waterPage][switchInFrame] = total

;    // Update counters
;    ++switchInFrame;
;    ++sampleInLine;
;    if (sampleInLine == 0x10) {
;        // Per line code
;        sampleInLine = 0;
;        ++lineInFrame;
;        if (lineInFrame == 0x10) {
;            // Per frame code
;            lineInFrame = 0;
;            if (outputSyncPulseActive) {
;                outputSyncPulseActive = false;
;                PORTD = 0x91;
;            }
;            waterPage ^= 1;
;            if (!waitingForSync) {
;                frameInBeat += 0x100;
;                if (frameInBeat >= framesPerBeat) {
;                    // Per beat code
;                    frameInBeat -= framesPerBeat;
;                    framesPerBeat = nextFramesPerBeat;
;                    ++beatInPattern;
;                    beatInPattern &= 0x0f;
;                    if (beatInPattern == beatsPerPattern) {
;                        beatInPattern = 0x10;
;                        waitingForSync = true;
;                        outputSyncPulseActive = true;
;                        PORTD = 0x95;
;                        if (lifeMode)
;                            lightPage ^= 1;
;                        else {
;                            ++patternInLoop;
;                            if (patternInLoop == patternsPerLoop)
;                                patternInLoop = 0;
;                        }
;                    }
;                }
;            }
;        }
;        if (waitingForSync)
;            if ((PINB & 1) != 0)
;                receivedSync = true;
;        if (lineInFrame == 0 && receivedSync) {
;            receivedSync = false;
;            waitingForSync = false;
;            beatInPattern = 0;
;        }
;        // Update line buffer
;        if (patternMode)
;            for (uint8_t column = 0; column < 0x10; ++column) {
;                uint8_t led = (lineInFrame<<4) | column;
;                uint8_t value = waterBuffers[waterPage][led] >> 1;
;                if (lightBuffers[lightPage][led]) {
;                    if (column == beatInPattern)
;                        value = 0x0f;
;                    else
;                        value += 10;
;                }
;                lineBuffer[column] = value;
;            }
;        else
;            for (uint8_t column = 0; column < 0x10; ++column)
;                lineBuffer[column] = frameBuffer[(lineInFrame<<4) | column];
;    }

  inc r11                        ; 1 1 1 1 1 1 1 1              ++switchInframe
  inc r3                         ; 1 1 1 1 1 1 1 1              ++sampleInLine
  cp r3, r19                     ; 1 1 1 1 1 1 1 1              if (sampleInLine == 0x10)
  breq .+2                       ; 1 2 2 2 2 2 2 2
  rjmp checkDecay                ; 2 0 0 0 0 0 0 0
  mov r3, r2                     ; 0 1 1 1 1 1 1 1                  sampleInLine = 0
  inc r4                         ; 0 1 1 1 1 1 1 1                  ++lineInFrame
  cp r4, r19                     ; 0 1 1 1 1 1 1 1                  if (lineInFrame == 0x10)
  brne checkWaitingForSync       ; 0 2 1 1 1 1 1 1
  mov r4, r2                     ; 0 0 1 1 1 1 1 1                      lineInFrame = 0
  sbrs r17, 3                    ; 0 0 1 1 1 1 1 1                      if (flags & 8)  (outputSyncPulseActive)                       2
  rjmp outputSyncPulseNotActive  ; 0 0 2 2 2 2 2 2                                                                                    0
  andi r17, 0xf7                 ; 0 0 0 0 0 0 0 0                          flags &= 0xf7  (outputSyncPulseActive = false)            1
  ldi r31, 0x91                  ; 0 0 0 0 0 0 0 0                          r31 = 0x91                                                1
  out 0x0b, r31                  ; 0 0 0 0 0 0 0 0                          PORTD = 0x91                                              1
outputSyncPulseNotActive:
  eor r15, r18                   ; 0 0 1 1 1 1 1 1                      waterPage ^= 1
  sbrc r17, 0                    ; 0 0 1 2 2 2 2 2                      if (!waitingForSync)
  rjmp waitingForSync            ; 0 0 2 0 0 0 0 0
  inc r6                         ; 0 0 0 1 1 1 1 1                          ++hi8(frameInBeat)
  cp r5, r7                      ; 0 0 0 1 1 1 1 1
  cpc r6, r8                     ; 0 0 0 1 1 1 1 1                          if (frameInBeat >= framesPerBeat)
  brlt checkReceivedSync         ; 0 0 0 2 1 1 1 1
  sub r5, r7                     ; 0 0 0 0 1 1 1 1
  sbc r6, r8                     ; 0 0 0 0 1 1 1 1                              frameInBeat -= framesPerBeat
  lds r7, nextFramesPerBeatLow   ; 0 0 0 0 2 2 2 2
  lds r8, nextFramesPerBeatHigh  ; 0 0 0 0 2 2 2 2                              framesPerBeat = nextFramesPerBeat
  inc r9                         ; 0 0 0 0 1 1 1 1                              ++beatInPattern
  ldi r31, 0x0f                  ; 0 0 0 0 1 1 1 1
  and r9, r31                    ; 0 0 0 0 1 1 1 1                              beatInPattern &= 0x0f;
  cp r9, r14                     ; 0 0 0 0 1 1 1 1                              if (beatInPattern == beatsPerPattern)
  brne checkReceivedSync         ; 0 0 0 0 2 1 1 1
  mov r9, r19                    ; 0 0 0 0 0 1 1 1                                  beatInPattern = 0x10
  ori r17, 9                     ; 0 0 0 0 0 1 1 1                                  flags |= 9  (waitingForSync = true, outputSyncPulseActive = true)
  ldi r31, 0x95                  ; 0 0 0 0 0 1 1 1                                  r31 = 0x95
  out 0x0b, r31                  ; 0 0 0 0 0 1 1 1                                  PORTD = 0x95
  sbrs r17, 1                    ; 0 0 0 0 0 1 2 1                                  if ((flags & 2) != 0)  (lifeMode != 0)
  rjmp notLifeMode               ; 0 0 0 0 0 2 0 2
  eor r10, r18                   ; 0 0 0 0 0 0 1 0                                      lightPage ^= 1
  rjmp waitingForSync            ; 0 0 0 0 0 0 2 0                                  else
notLifeMode:
  lds r31, patternInLoop         ; 0 0 0 0 0 2 0 2
  inc r31                        ; 0 0 0 0 0 1 0 1                                      ++patternInLoop
  lds r30, patternsPerLoop       ; 0 0 0 0 0 2 0 2
  cp r31, r30                    ; 0 0 0 0 0 1 0 1                                      if (patternInLoop == patternsPerLoop)
  brne noRestartLoop             ; 0 0 0 0 0 2 0 1
  ldi r31, 0                     ; 0 0 0 0 0 0 0 1                                          patternInLoop = 0
noRestartLoop:
  sts patternInLoop, r31         ; 0 0 0 0 0 2 0 2
  rjmp waitingForSync            ; 0 0 0 0 0 2 0 2
checkWaitingForSync:
  sbrs r17, 0                    ; 1 2 2 2 2 2                      if (flags & 1)  (waitingForSync)
  rjmp checkReceivedSync         ; 2 0 0 0 0 0
waitingForSync:
  in r31, 0x03                   ; 0 1 1 1 1 1                          r31 = PINB
  sbrs r31, 0                    ; 0 1 2 1 2 1                          if ((PINB & 1) != 0)
  rjmp checkReceivedSync         ; 0 2 0 2 0 2
  ori r17, 4                     ; 0 0 1 0 1 0                              flags |= 4  (receivedSync = true)
  rjmp receivedSync              ; 0 0 2 0 2 0
checkReceivedSync:
  sbrs r17, 2                    ; 0 1 0 2 0 2                      if ((flags & 4) != 0)  (receivedSync)
  rjmp updateLineBuffer          ; 0 2 0 0 0 0
receivedSync:
  cp r4, r2                      ; 0 0 1 1 1 1                          if (lineInFrame == 0)
  brne updateLineBuffer          ; 0 0 2 2 1 1
  andi r17, 0xfa                 ; 0 0 0 0 1 1                              flags &= 0xfa  (waitingForSync = false, receivedSync = false)
  mov r9, r2                     ; 0 0 0 0 1 1                              beatInPattern = 0

updateLineBuffer:
  sbrs r17, 6                    ; 1 2                              if ((flags & 0x40) != 0)  (patternMode != 0)
  rjmp frameBufferMode           ; 2 0

  mov r30, r4                    ; 0 1                                  lo8(Z) = lineInFrame
  swap r30                       ; 0 1                                  lo8(Z) = lineInFrame<<4
  mov r31, r15                   ; 0 1                                  hi8(Z) = waterPage
  mov r29, r10                   ; 0 1                                  hi8(Y) = lightPage
  mov r28, r30                   ; 0 1                                  lo8(Y) = lineInFrame<<4

.macro initRow column          ; 9
  ldd r25, Z+\column           ; 2 2  r25 = waterBuffers[waterPage][(lineInFrame << 4) | column]
  ldd r24, Y+\column           ; 2 2  r24 = lightBuffers[lightPage][(lineInFrame << 4) | column]
  lsr r25                      ; 1 1  r25 = waterBuffers[waterPage][(lineInFrame << 4) | column] >> 1
  sbrc r24, 0                  ; 1 2  if (lightBuffers[lightPage][(lineInFrame << 4) | column])
  subi r25, -10                ; 1 0      r25 += 10
  sts lineBuffer+\column, r25  ; 2 2  lineBuffer[column] = r25
.endm

  unroll initRow                 ; 0 144

  add r28, r9                    ; 0 1    lo8(Y) = (lineInFrame<<4) | beatInPattern
  ld r24, Y                      ; 0 2    r24 = lightBuffers[lightPage][(lineInFrame << 4) | column]
  tst r24                        ; 0 1    if (lightBuffers[lightPage][(lineInFrame << 4) | column])
  breq checkDecay                ; 0 2 1
  ldi r31, hi8(lineBuffer)       ; 0 0 1
  ldi r30, lo8(lineBuffer)       ; 0 0 1
  add r30, r9                    ; 0 0 1
  ldi r25, 0x0f                  ; 0 0 1
  st Z, r25                      ; 0 0 2

  rjmp checkDecay                ; 0 2
frameBufferMode:                 ;                              else
  ldi r26, 10                    ; 1 0                              r26 = 10
  ldi r27, 0                     ; 1 0                              r27 = 0
  ldi r31, hi8(frameBuffer)      ; 1 0                              hi8(Z) = hi8(frameBuffer)
  mov r30, r4                    ; 1 0                              lo8(Z) = lineInFrame
  swap r30                       ; 1 0                              lo8(Z) = lineInFrame << 4

.macro initFromFrameBuffer column  ; 4
  ld r24, Z+                   ; 2  r24 = frameBuffer[(lineInFrame << 4) | column]
  sts lineBuffer+\column, r24  ; 2  lineBuffer[column] = frameBuffer[(lineInFrame << 4) | column]
.endm

  unroll initFromFrameBuffer     ; 64 0

;    // Sound decay
;    if (sampleInLine == 1 && lineInFrame == 0)
;        for (uint8_t channel = 0; channel < 0x10; ++channel)
;            volumes[channel] = (volumes[channel]*decayConstant)>>8;

checkDecay:
  cp r11, r18                    ; 1 1  if (switchInFrame == 1)
  breq .+2                       ; 1 2
  rjmp checkAttack               ; 2 0
  lds r22, decayConstant         ; 0 2      r22 = decayConstant

.macro decay channel           ; 6
  lds r24, volumes+\channel  ; 2  r24 = volumes[channel]
  mul r24, r22               ; 2  r1:r0 = volumes[channel]*decayConstant
  sts volumes+\channel, r1   ; 2  volumes[channel] = (volumes[channel]*decayConstant)>>8
.endm

  unroll decay                   ; 0 96

;    // Sound attack
;    if ((frameInBeat >> 8) == 0 && sampleInLine == 2 && !waitingForSync)
;        for (uint8_t channel = 0; channel < 0x10; ++channel)
;            if (lightBuffers[lightPage][(channel<<4) | beatInPattern])
;                volumes[channel] = 0xff;

checkAttack:
  cp r6, r2                      ; 1 1 1 1    if ((frameInBeat >> 8) == 0)
  breq .+2                       ; 1 2 2 2
  rjmp checkLastFrame            ; 2 0 0 0
  mov r22, r11                   ; 0 1 1 1        r22 = switchInFrame
  cpi r22, 2                     ; 0 1 1 1        if (switchInFrame == 2)
  breq .+2                       ; 0 1 2 2
  rjmp checkLastFrame            ; 0 2 0 0
  sbrc r17, 0                    ; 0 0 1 2            if ((flags & 1) != 0)  (waitingForSync)
  rjmp checkLastFrame            ; 0 0 2 0
  mov r30, r9                    ; 0 0 0 1                lo8(Z) = beatInPattern
  mov r31, r10                   ; 0 0 0 1                hi8(Z) = lightPage
  ldi r28, lo8(volumes)          ; 0 0 0 1                lo8(Y) = lo8(volumes)
  ldi r29, hi8(volumes)          ; 0 0 0 1                hi8(Y) = hi8(volumes)
  ldi r25, 0xff                  ; 0 0 0 1                r25 = 0xff

.macro attack channel
  ld r24, Z                      ; 2    r24 = lightBuffers[lightPage][r30]
  sbrc r24, 0                    ; 1 2  if ((lightBuffers[lightPage][r30] & 1) != 0)
  std Y+\channel, r25            ; 2 0      volumes[Y] = 0xff
.if \channel < 0x0f
  subi r30, -0x10                ; 1    r30 += 0x10
.endif
.endm

  unroll attack                  ; (5..6)*16 - 1 = 79..95

;    // Last frame processing
;    if (beatInPattern == ((beatsPerPattern - 1)&0x0f) && frameInBeat+0x100 >= framesPerBeat) {
;        if (lifeMode) {
;            uint8_t total = lightBuffers[lightPage][(switchInFrame - 1)&0xff];
;            total += lightBuffers[lightPage][(switchInFrame + 1)&0xff];
;            total += lightBuffers[lightPage][(switchInFrame - 17)&0xff];
;            total += lightBuffers[lightPage][(switchInFrame - 16)&0xff];
;            total += lightBuffers[lightPage][(switchInFrame - 15)&0xff];
;            total += lightBuffers[lightPage][(switchInFrame + 17)&0xff];
;            total += lightBuffers[lightPage][(switchInFrame + 16)&0xff];
;            total += lightBuffers[lightPage][(switchInFrame + 15)&0xff];
;            if (total == 3)
;                total = 1;
;            else
;                if (total == 2)
;                    total = lightBuffers[lightPage][switchInFrame];
;                else
;                    total = 0;
;            lightBuffers[lightPage^1][switchInFrame] = total;
;            nextLightsLit += total;
;        }
;        else
;            if (patternsPerLoop != 1) {
;                uint8_t l = (eeprom_read_byte((uint8_t*)(((patternInLoop & 0x1c) << 5) + ((patternInLoop & 3) << 4) + ((switchInFrame & 0x80) >> 1) + ((switchInFrame & 0x78) >> 3))) >> (switchInFrame & 0x07)) & 1;
;                lightBuffers[lightPage][switchInFrame] = l;
;                nextLightsLit += total;
;            }
;        if (switchInFrame == 0xff) {
;            lightsLit = nextLightsLit;
;            nextLightsLit = 0;
;        }
;    }

checkLastFrame:
  mov r31, r14                   ; 1 1 1 1 1 1 1 1  r31 = beatsPerPattern
  dec r31                        ; 1 1 1 1 1 1 1 1  r31 = beatsPerPattern - 1
  andi r31, 0x0f                 ; 1 1 1 1 1 1 1 1  r31 = (beatsPerPattern - 1) & 0x0f
  cp r31, r9                     ; 1 1 1 1 1 1 1 1  if ((beatsPerPattern - 1) & 0x0f == beatInPattern) {
  brne checkRandom2              ; 2 1 1 1 1 1 1 1
  mov r31, r6                    ; 0 1 1 1 1 1 1 1      r31 = hi8(frameInBeat)
  inc r31                        ; 0 1 1 1 1 1 1 1      r31 = hi8(frameInBeat) + 1
  cp r5, r7                      ; 0 1 1 1 1 1 1 1
  cpc r31, r8                    ; 0 1 1 1 1 1 1 1      if (frameInBeat + 0x100 >= framesPerBeat)
  brsh lastFrame                 ; 0 1 2 2 2 2 2 2
checkRandom2:
  rjmp checkRandom               ; 2 2 0 0 0 0 0 0
lastFrame:
  sbrs r17, 1                    ; 0 0 1 2 2 2 1 1          if ((flags & 2) != 0)  (lifeMode)
  rjmp checkPatternLoop          ; 0 0 2 0 0 0 2 2

  mov r30, r11                   ; 0 0 0 1 1 1 0 0              lo8(Z) = switchInFrame
  subi r30, 0x11                 ; 0 0 0 1 1 1 0 0              lo8(Z) = switchInFrame - 0x11
  mov r31, r10                   ; 0 0 0 1 1 1 0 0              hi8(Z) = lightPage
  ld r25, Z                      ; 0 0 0 2 2 2 0 0              r25 = lightBuffers[lightPage][switchInFrame - 0x11]

.macro life offset
  subi r30, -\offset  ; 1  lo8(Z) += offset
  ld r24, Z           ; 2  r24 = lightBuffers[lightPage][lo8(Z)]
  add r25, r24        ; 1  r25 += lightBuffers[lightPage][lo8(Z)]
.endm

  life 1                         ; 0 0 0 4 4 4 0 0              r25 += lightBuffers[lightPage][switchInFrame - 0x10]
  life 1                         ; 0 0 0 4 4 4 0 0              r25 += lightBuffers[lightPage][switchInFrame - 0x0f]
  life 0x0e                      ; 0 0 0 4 4 4 0 0              r25 += lightBuffers[lightPage][switchInFrame - 0x01]
  life 2                         ; 0 0 0 4 4 4 0 0              r25 += lightBuffers[lightPage][switchInFrame + 0x01]
  life 0x0e                      ; 0 0 0 4 4 4 0 0              r25 += lightBuffers[lightPage][switchInFrame + 0x0f]
  life 1                         ; 0 0 0 4 4 4 0 0              r25 += lightBuffers[lightPage][switchInFrame + 0x10]
  life 1                         ; 0 0 0 4 4 4 0 0              r25 += lightBuffers[lightPage][switchInFrame + 0x11]
  mov r30, r11                   ; 0 0 0 1 1 1 0 0              lo8(Z) = switchInFrame
  cpi r25, 3                     ; 0 0 0 1 1 1 0 0              if (total == 3)
  brne noBirth                   ; 0 0 0 2 1 1 0 0
  ldi r25, 1                     ; 0 0 0 0 1 1 0 0                  total = 1
  rjmp storeLife                 ; 0 0 0 0 2 2 0 0              else
noBirth:
  cpi r25, 2                     ; 0 0 0 1 0 1 0 0                  if (total == 2)
  brne death                     ; 0 0 0 2 0 1 0 0
  ld r25, Z                      ; 0 0 0 0 0 2 0 0                      total = lightBuffers[lightPage][switchInFrame]
  rjmp storeLife                 ; 0 0 0 0 0 2 0 0                  else
death:
  ldi r25, 0                     ; 0 0 0 1 0 0 0 0                      total = 0
storeLife:
  eor r31, r18                   ; 0 0 0 1 1 1 0 0              hi8(Z) ^= 1
  st Z, r25                      ; 0 0 0 2 2 2 0 0              lightBuffers[1-lightPage] = total
  lds r31, nextLightsLit         ; 0 0 0 2 2 2 0 0              r31 = nextLightsLit
  add r31, r25                   ; 0 0 0 1 1 1 0 0              r31 = nextLightsLit + total
  sts nextLightsLit, r31         ; 0 0 0 2 2 2 0 0              nextLightsLit += total
  rjmp updateLightsLit           ; 0 0 0 2 2 2 0 0

checkPatternLoop:                ;                          else
  lds r31, patternsPerLoop       ; 0 0 2 0 0 0 2 2              if (patternsPerLoop != 1)
  cpi r31, 1                     ; 0 0 1 0 0 0 1 1
  breq doPatternLoop             ; 0 0 1 0 0 0 2 2
  rjmp checkRandom               ; 0 0 2 0 0 0 0 0

; EEPROM address: 9876543210
; patternInLoop:  432 10
; switchInFrame:     7  6543

doPatternLoop:
  mov r31, r11                   ; 0 0 0 0 0 0 1 1                  if ((switchInFrame & 7) == 0) {
  andi r31, 7                    ; 0 0 0 0 0 0 1 1
  brne noSetEEAR                 ; 0 0 0 0 0 0 1 2

  ; Compute EEPROM address high
  lds r31, patternInLoop         ; 0 0 0 0 0 0 2 0                      r31 = patternInLoop
  mov r30, r31                   ; 0 0 0 0 0 0 1 0                      r30 = patternInLoop
  lsr r31                        ; 0 0 0 0 0 0 1 0                      r31 = patternInLoop >> 1
  lsr r31                        ; 0 0 0 0 0 0 1 0                      r31 = patternInLoop >> 2
  lsr r31                        ; 0 0 0 0 0 0 1 0                      r31 = patternInLoop >> 3
  out 0x22, r31                  ; 0 0 0 0 0 0 1 0                      EEARH (EEPROM Address Register High) = patternInLoop >> 3

  ; Compute EEPROM address low
  swap r30                       ; 0 0 0 0 0 0 1 0                      r30 = (patternInLoop >> 4) | (patternInLoop << 4)
  bst r30, 6                     ; 0 0 0 0 0 0 1 0                      T = (patternInLoop & 4)
  andi r30, 0x30                 ; 0 0 0 0 0 0 1 0                      r30 = (patternInLoop << 4) & 0x30
  bld r30, 7                     ; 0 0 0 0 0 0 1 0                      r30 = ((patternInLoop << 4) & 0x30) | (T ? 0x80 : 0)
  bst r11, 7                     ; 0 0 0 0 0 0 1 0                      T = (switchInFrame & 0x80)
  bld r30, 6                     ; 0 0 0 0 0 0 1 0                      r30 = ((patternInLoop << 4) & 0x30) | ((patternInLoop & 4) ? 0x80 : 0) | (T ? 0x40 : 0)
  mov r31, r11                   ; 0 0 0 0 0 0 1 0                      r31 = switchInFrame
  lsr r31                        ; 0 0 0 0 0 0 1 0                      r31 = switchInFrame >> 1
  lsr r31                        ; 0 0 0 0 0 0 1 0                      r31 = switchInFrame >> 2
  lsr r31                        ; 0 0 0 0 0 0 1 0                      r31 = switchInFrame >> 3
  andi r31, 0x0f                 ; 0 0 0 0 0 0 1 0                      r31 = (switchInFrame >> 3) & 0x0f
  or r30, r31                    ; 0 0 0 0 0 0 1 0                      r30 = ((patternInLoop << 4) & 0x30) | ((patternInLoop & 4) ? 0x80 : 0) | ((switchInFrame & 0x80) ? 0x40 : 0) | ((switchInFrame >> 3) & 0x0f)
  out 0x21, r30                  ; 0 0 0 0 0 0 1 0                      EEARL (EEPROM Address Register Low) = ((patternInLoop << 4) & 0x30) | ((patternInLoop & 4) ? 0x80 : 0) | ((switchInFrame & 0x80) ? 0x40 : 0) | ((switchInFrame >> 3) & 0x0f)

noSetEEAR:
  sbi 0x1f, 0                    ; 0 0 0 0 0 0 1 1                      set EERE (EEPROM Read Enable) in EECR (EEPROM Control Register)
  in r27, 0x20                   ; 0 0 0 0 0 0 1 1                      r27 = EEDR (EEPROM Data Register)

  mov r30, r11                   ; 0 0 0 0 0 0 1 1                      lo8(Z) = switchInFrame
  andi r30, 0x07                 ; 0 0 0 0 0 0 1 1                      lo8(Z) = switchInFrame & 7

  mov r31, r2                    ; 0 0 0 0 0 0 1 1                      hi8(Z) = 0
  lsl r30                        ; 0 0 0 0 0 0 1 1                      lo8(Z) = (switchInFrame & 7) << 1
  subi r30, lo8(-(columnTable))  ; 0 0 0 0 0 0 1 1
  sbci r31, hi8(-(columnTable))  ; 0 0 0 0 0 0 1 1                      Z = &columnTable[switchInFrame & 7]
  lpm r28, Z+                    ; 0 0 0 0 0 0 3 3
  lpm r29, Z+                    ; 0 0 0 0 0 0 3 3                      Y = *Z
  movw r30, r28                  ; 0 0 0 0 0 0 1 1                      Z = *Z
  ijmp                           ; 0 0 0 0 0 0 2 2                      goto columnTable[switchInFrame & 7]  switch (switchInFrame & 7) {
column0:                         ;                                          case 0:
  bst r27, 0                     ; 0 0 0 0 0 0 1 1                              T = r27 & 1
  rjmp doneColumn                ; 0 0 0 0 0 0 2 2                              break;
column1:                         ;                                          case 1:
  bst r27, 1                     ;                                              T = r27 & 2
  rjmp doneColumn                ;                                              break;
column2:                         ;                                          case 2:
  bst r27, 2                     ;                                              T = r27 & 4
  rjmp doneColumn                ;                                              break;
column3:                         ;                                          case 3:
  bst r27, 3                     ;                                              T = r27 & 8
  rjmp doneColumn                ;                                              break;
column4:                         ;                                          case 4:
  bst r27, 4                     ;                                              T = r27 & 0x10
  rjmp doneColumn                ;                                              break;
column5:                         ;                                          case 5:
  bst r27, 5                     ;                                              T = r27 & 0x20
  rjmp doneColumn                ;                                              break;
column6:                         ;                                          case 6:
  bst r27, 6                     ;                                              T = r27 & 0x40
  rjmp doneColumn                ;                                              break;
column7:                         ;                                          case 7:
  bst r27, 7                     ;                                              T = r27 & 0x80
doneColumn:                      ;                                      }
  mov r31, r10                   ; 0 0 0 0 0 0 1 1                      hi8(Z) = lightPage
  mov r30, r11                   ; 0 0 0 0 0 0 1 1                      lo8(Z) = switchInFrame
  mov r29, r2                    ; 0 0 0 0 0 0 1 1                      r29 = 0
  brtc .+2                       ; 0 0 0 0 0 0 1 2                      if (T)
  mov r29, r18                   ; 0 0 0 0 0 0 1 0                          r29 = 1
  st Z, r29                      ; 0 0 0 0 0 0 2 2                      lightBuffers[lightPage][switchInFrame] = T ? 1 : 0
  lds r31, nextLightsLit         ; 0 0 0 0 0 0 2 2                      r31 = nextLightsLit
  add r31, r29                   ; 0 0 0 0 0 0 1 1                      r31 = nextLightsLit + (T ? 1 : 0)
  sts nextLightsLit, r31         ; 0 0 0 0 0 0 2 2                      nextLightsLit += (T ? 1 : 0)

updateLightsLit:
  mov r31, r11                   ; 1 1
  cpi r31, 0xff                  ; 1 1
  brne checkRandom               ; 2 1
  lds r16, nextLightsLit         ; 0 2
  sts nextLightsLit, r2          ; 0 2

;    // Random mode
;    if (randomMode && beatInPattern == 0) {
;        if ((frameInBeat >> 8) == 0 && switchInFrame == 0xff) {
;            if (lightsLit != 0) {
;                randomOff = random(lightsLit);
;                randomOn = random(0xff - lightsLit);
;                gotRandom = true;
;            }
;            else
;                gotRandom = false;
;        }
;        else
;            if ((frameInBeat >> 8) == 1 && gotRandom) {
;                if (lightBuffers[lightPage][switchInFrame] != 0) {
;                    if (randomOff == 0)
;                        lightBuffers[lightPage][switchInFrame] = 0;
;                    --randomOff;
;                }
;                else {
;                    if (randomOn == 0)
;                        lightBuffers[lightPage][switchInFrame] = 1;
;                    --randomOn;
;                }
;            }
;    }

checkRandom:
  sbrs r17, 5                    ; 1 2 2 2 2 2 2 2 2 2 2  if (flags & 0x20)  (randomMode)
  rjmp doneInterrupt             ; 2 0 0 0 0 0 0 0 0 0 0
  cp r9, r2                      ; 0 1 1 1 1 1 1 1 1 1 1      if (beatInPattern == 0)
  brne doneInterrupt             ; 0 2 1 1 1 1 1 1 1 1 1
  cp r6, r2                      ; 0 0 1 1 1 1 1 1 1 1 1          if (hi8(frameInBeat) == 0)
  brne notFrame0                 ; 0 0 2 1 1 1 2 2 2 2 2
  mov r31, r11                   ; 0 0 0 1 1 1 0 0 0 0 0              r31 = switchInFrame
  cpi r31, 0xff                  ; 0 0 0 1 1 1 0 0 0 0 0              if (switchInFrame == 0xff)
  brne doneInterrupt             ; 0 0 0 2 1 1 0 0 0 0 0
  cpi r16, 0                     ; 0 0 0 0 1 1 0 0 0 0 0                  if (lightsLit != 0)
  breq noLightsLit               ; 0 0 0 0 2 1 0 0 0 0 0
  mov r27, r16                   ; 0 0 0 0 0 1 0 0 0 0 0
  rcall random                   ; 0 0 0 0 0 103-116 0 0 0 0 0
  sts randomOff, r22             ; 0 0 0 0 0 2 0 0 0 0 0                      randomOff = random(lightsLit)
  ldi r27, 0                     ; 0 0 0 0 0 1 0 0 0 0 0
  sub r27, r16                   ; 0 0 0 0 0 1 0 0 0 0 0
  rcall random                   ; 0 0 0 0 0 103-116 0 0 0 0 0
  sts randomOn, r22              ; 0 0 0 0 0 2 0 0 0 0 0                      randomOn = random(0x100 - lightsLit)
  ori r17, 0x10                  ; 0 0 0 0 0 1 0 0 0 0 0                      gotRandom = true
  rjmp doneInterrupt             ; 0 0 0 0 0 2 0 0 0 0 0
noLightsLit:                     ;                                        else
  andi r17, 0xef                 ; 0 0 0 0 1 0 0 0 0 0 0                      gotRandom = false
  rjmp doneInterrupt             ; 0 0 0 0 2 0 0 0 0 0 0
notFrame0:                       ;                                else
  cp r6, r18                     ; 0 0 0 1 0 0 1 1 1 1 1              if (hi8(frameInBeat) == 1)
  brne doneInterrupt             ; 0 0 0 2 0 0 1 1 1 1 1
  sbrs r17, 4                    ; 0 0 0 0 0 0 2 2 2 2 1                  if (gotRandom)
  rjmp doneInterrupt             ; 0 0 0 0 0 0 0 0 0 0 2
  mov r31, r10                   ; 0 0 0 0 0 0 1 1 1 1 0                      hi8(Z) = lightPage
  mov r30, r11                   ; 0 0 0 0 0 0 1 1 1 1 0                      lo8(Z) = switchInFrame
  ld r28, Z                      ; 0 0 0 0 0 0 2 2 2 2 0                      r28 = lightBuffers[lightPage][switchInFrame]
  cpi r28, 0                     ; 0 0 0 0 0 0 1 1 1 1 0                      if (lightBuffers[lightPage][switchInFrame] != 0)
  breq checkRandomOn             ; 0 0 0 0 0 0 2 1 1 2 0
  lds r29, randomOff             ; 0 0 0 0 0 0 0 2 2 0 0                          r29 = randomOff
  cpi r29, 0                     ; 0 0 0 0 0 0 0 1 1 0 0                          if (randomOff == 0)
  brne decrementRandomOff        ; 0 0 0 0 0 0 0 2 1 0 0
  st Z, r2                       ; 0 0 0 0 0 0 0 0 2 0 0                              lightBuffers[lightPage][switchInFrame] = 0
  dec r16                        ; 0 0 0 0 0 0 0 0 1 0 0                              --lightsLit
decrementRandomOff:              ;
  dec r29                        ; 0 0 0 0 0 0 0 1 1 0 0                          --r29
  sts randomOff, r29             ; 0 0 0 0 0 0 0 2 2 0 0                          --randomOff
  rjmp doneInterrupt             ; 0 0 0 0 0 0 0 2 2 0 0
checkRandomOn:                   ;                                            else
  lds r29, randomOn              ; 0 0 0 0 0 0 2 0 0 2 0                          r29 = randomOn
  cpi r29, 0                     ; 0 0 0 0 0 0 1 0 0 1 0                          if (randomOn == 0)
  brne decrementRandomOn         ; 0 0 0 0 0 0 2 0 0 1 0
  st Z, r18                      ; 0 0 0 0 0 0 0 0 0 2 0                              lightBuffers[lightPage][switchInFrame] = 1
  inc r16                        ; 0 0 0 0 0 0 0 0 0 1 0                              ++lightsLit
decrementRandomOn:               ;
  dec r29                        ; 0 0 0 0 0 0 1 0 0 1 0                          --r29
  sts randomOn, r29              ; 0 0 0 0 0 0 2 0 0 2 0                          --randomOn

doneInterrupt:
  sei                            ; 1  Enable interrupts a bit early so that we can postpone the epilogue until after the next interrupt if we need to.
  pop r0                         ; 2
  out 0x3f, r0                   ; 1
  pop r0                         ; 2
  pop r1                         ; 2
  pop r24                        ; 2
  pop r25                        ; 2
  pop r26                        ; 2
  pop r27                        ; 2
  pop r28                        ; 2
  pop r29                        ; 2
  pop r30                        ; 2
  pop r31                        ; 2
  reti                           ; 4


.global adcComplete
adcComplete:                             ; Uses 25 bytes of stack (4 here, 21 for checkSerialInput)
  push r28
  push r29
  lds r30, 0x78      ; lo8(Z) = ADCL
  lds r31, 0x79      ; hi8(Z) = ADCH

  lds r29, adcChannel
  cpi r29, 2
  breq adcCompletedChannel5
  cpi r29, 1
  breq adcCompletedChannel4

adcCompletedChannel3:
  ldi r29, 0x44
  sts 0x7c, r29  ; ADMUX = 0x44  (set channel to 4)
  ldi r29, 0xc7
  sts 0x7a, r29  ; ADCSRA = 0xc7  (start conversion)

  lds r29, decayConstantOverride
  cpi r29, 0
  brne overrideDecayConstant

  lsr r31
  ror r30
  lsr r31
  ror r30
  ldi r31, 0xff
  sub r31, r30
  sts decayConstant, r31

doneChannel3:
  ldi r29, 1
  rjmp adcCompleteEnd

overrideDecayConstant:
  sts decayConstant, r29
  rjmp doneChannel3

adcCompletedChannel4:
  ldi r29, 0x45
  sts 0x7c, r29  ; ADMUX = 0x45  (set channel to 5)
  ldi r29, 0xc7
  sts 0x7a, r29  ; ADCSRA = 0xc7  (start conversion)

  lds r29, framesPerBeatOverride+1
  cpi r29, 2
  brge overrideFramesPerBeat

  add r30, r30
  adc r31, r31
  add r30, r30
  adc r31, r31                    ; Z = analogRead(4) << 2
  subi r31, -2                    ; Z = (analogRead(4) << 2) + 0x200  (gives a range of 2-18 frames per beat, or 0.5-4.7 seconds per 16 beats)
  sts nextFramesPerBeatLow, r30
  sts nextFramesPerBeatHigh, r31  ; framesPerBeat = (analogRead(4) << 2) + 0x200

doneChannel4:
  ldi r29, 2
  rjmp adcCompleteEnd

overrideFramesPerBeat:
  sts nextFramesPerBeatHigh, r29
  lds r29, framesPerBeatOverride
  sts nextFramesPerBeatLow, r29
  rjmp doneChannel4

adcCompletedChannel5:
  ldi r29, 0x43
  sts 0x7c, r29  ; ADMUX = 0x43  (set channel to 3)
  ldi r29, 0xc7
  sts 0x7a, r29  ; ADCSRA = 0xc7  (start conversion)

  ldi r29, 0x03
  ldi r28, 0xff
  sub r28, r30
  sbc r29, r31
  add r28, r28
  adc r29, r29
  ldi r30, lo8(tuningMultipliers)   ; lo8(Z) = lo8(tuningMultipliers)
  ldi r31, hi8(tuningMultipliers)   ; hi8(Z) = lo8(tuningMultipliers)
  add r30, r28
  adc r31, r29                      ; Z = ((0x3ff - analogRead(5)) << 1) + tuningMultipliers
  lpm r28, Z+
  lpm r29, Z+                       ; Y = tuningMultipliers[analogRead(5)]

  lds r27, flags2
  sbrs r27, 3
  rjmp gotTuningMultiplier

  ldi r28, 0
  ldi r29, 0x80

gotTuningMultiplier:

  sbrs r27, 1
  rjmp notMicroToneMode

.macro microToneTune channel
  lds r30, microtoneKeys+\channel
  ldi r31, hi8(microtoneFrequencies)
  lsl r30
  adc r31, r2
  lpm r26, Z+
  lpm r27, Z+
  movw r30, r26
  mul r31, r29
  mov r27, r1
  mov r26, r0
  mul r31, r28
  mov r31, r0
  add r26, r1
  adc r27, r2
  mul r30, r29
  add r31, r0
  adc r26, r1
  adc r27, r2
  mul r30, r28
  add r31, r1
  adc r26, r2
  adc r27, r2                          ; unsigned multiply (r27:r26:r31 = (r31:r30*r29:r28)>>8)

  sts velocities+(\channel*2), r26
  sts velocities+(\channel*2)+1, r27   ; velocities[channel] = frequencies[channel]*tuningMultiplier
.endm

  unroll microToneTune

  ldi r29, 0
  rjmp adcCompleteEnd

notMicroToneMode:

.macro fineTune channel
  lds r30, frequencies+(\channel*2)
  lds r31, frequencies+(\channel*2)+1  ; 1  ; Z = frequencies[channel]
  mul r31, r29
  mov r27, r1
  mov r26, r0
  mul r31, r28
  mov r31, r0
  add r26, r1
  adc r27, r2
  mul r30, r29
  add r31, r0
  adc r26, r1
  adc r27, r2
  mul r30, r28
  add r31, r1
  adc r26, r2
  adc r27, r2                          ; unsigned multiply (r27:r26:r31 = (r31:r30*r29:r28)>>8)

  sts velocities+(\channel*2), r26
  sts velocities+(\channel*2)+1, r27   ; velocities[channel] = frequencies[channel]*tuningMultiplier
.endm

  unroll fineTune

  ldi r29, 0

adcCompleteEnd:
  sts adcChannel, r29
  pop r29
  pop r28
  clr r1
  ret


; randomData = randomData * 0x000343fd + 0x00269ec3;
; returns (randomData >> 16) & 0xff in r24
; This is the same linear congruential generator as used in Microsoft Visual
; C++ 2008. Because the multiplier has high byte 0, it's slightly more
; efficient than most others.
; c3:c2:c1:c0 is in r25:r24:r27:r26
; Even though we don't use c3 here, we calculate it anyway because we use the
; same randomData as the interrupt routine, which uses all 32 bits.
; There is a race condition between this and the random number routine used by
; the interrupt routine. That's okay, though - when this happens it will
; either return the same number for interrupt and main, or (very rarely)
; re-seed.
.global randomByte
randomByte:
  push r28
  push r29

  lds r28, randomData    ; a0
  lds r29, randomData+1  ; a1

  ldi r31, 0x43          ; b1
  mul r29, r31           ; r1:r0 = a1*b1
  movw r24, r0           ; c3:c2 = a1*b1

  ldi r31, 0xfd          ; b0
  mul r28, r31           ; r1:r0 = a0*b0
  movw r26, r0           ; c1:c0 = a0*b0

  lds r30, randomData+3  ; a3
  mul r30, r31           ; r1:r0 = a3*b0
  adc r25, r0            ; c3 += a3*b0

  lds r30, randomData+2  ; a2
  mul r30, r31           ; r1:r0 = a2*b0
  add r24, r0            ;
  adc r25, r1            ; c3:c2 += a2*b0

  mul r29, r31           ; r1:r0 = a1*b0
  add r27, r0            ;
  adc r24, r1            ;
  adc r25, r2            ; c3:c2:c1 += a1*b0

  ldi r31, 0x43          ; b1
  mul r30, r31           ; r1:r0 = a2*b1
  add r25, r0            ; c3 += a2*b1

  mul r28, r31           ; r1:r0 = a0*b1
  add r27, r0            ;
  adc r24, r1            ;
  adc r25, r2            ; c3:c2:c1 += a0*b1

  ldi r31, 0x03          ; b2
  mul r28, r31           ; r1:r0 = a0*b2
  add r24, r0            ;
  adc r25, r1            ; c3:c2 += a0*b2

  mul r29, r31           ; r1:r0 = a1*b2
  add r25, r0            ; c3 += a1*b2

  subi r26, 0x3d
  sbci r27, 0x61
  sbci r24, 0xd9
  sbci r25, 0xff        ; c3:c2:c1:c0 += 0x00269ec3

  sts randomData, r26
  sts randomData+1, r27
  sts randomData+2, r24
  sts randomData+3, r25 ; randomData = c3:c2:c1:c0
  clr r1
  pop r29
  pop r28
  ret


; The toolchain links in code that at the start of the program, sets SP = 0x8ff (top of RAM) and the status flags all to 0 before calling main.

.global main
main:

  ; Initialize hardware ports

  ; DDRB value:   0x2e  (Port B Data Direction Register)
  ;   DDB0           0  Sync in                     - input
  ;   DDB1           2  Blue LED (OC1A)             - output
  ;   DDB2           4  Shift register reset        - output
  ;   DDB3           8  Shift register data (MOSI)  - output
  ;   DDB4           0  Escape switch               - input
  ;   DDB5        0x20  Shift register clock (SCK)  - output
  ;   DDB6           0  XTAL1/TOSC1
  ;   DDB7           0  XTAL2/TOSC2
  ldi r31, 0x2e
  out 0x04, r31

  ; PORTB value:  0x11  (Port B Data Register)
  ;   PORTB0         1  Sync in                     - pull-up enabled
  ;   PORTB1         0  Blue LED (OC1A)
  ;   PORTB2         0  Shift register reset        - low
  ;   PORTB3         0  Shift register data (MOSI)
  ;   PORTB4      0x10  Escape switch               - pull-up enabled
  ;   PORTB5         0  Shift register clock (SCK)
  ;   PORTB6         0  XTAL1/TOSC1
  ;   PORTB7         0  XTAL2/TOSC2
  ldi r31, 0x11
  out 0x05, r31

  ; DDRC value:   0x07  (Port C Data Direction Register)
  ;   DDC0           1  Switch selector A           - output
  ;   DDC1           2  Switch selector B           - output
  ;   DDC2           4  Switch selector C           - output
  ;   DDC3           0  Decay potentiometer (ADC3)  - input
  ;   DDC4           0  Tempo potentiometer (ADC4)  - input
  ;   DDC5           0  Tuning potentiometer (ADC5) - input
  ;   DDC6           0  ~RESET
  ldi r31, 0x07
  out 0x07, r31

  ; PORTC value:  0x00  (Port C Data Register)
  ;   PORTC0         0  Switch selector A           - low
  ;   PORTC1         0  Switch selector B           - low
  ;   PORTC2         0  Switch selector C           - low
  ;   PORTC3         0  Volume potentiometer (ADC3)
  ;   PORTC4         0  Tempo potentiometer (ADC4)
  ;   PORTC5         0  Tuning potentiometer (ADC5)
  ;   PORTC6         0  ~RESET
  ldi r31, 0x00
  out 0x08, r31

  ; DDRD value:   0x6c  (Port D Data Direction Register)
  ;   DDD0           0  Debugging (RXD)
  ;   DDD1           0  Debugging (TXD)
  ;   DDD2           4  Sync out                    - output
  ;   DDD3           8  Audio output (OC2B)         - output
  ;   DDD4           0  Switch input 0              - input
  ;   DDD5        0x20  Red LED (OC0B)              - output
  ;   DDD6        0x40  Green LED (OC0A)            - output
  ;   DDD7           0  Switch input 1              - input
  ldi r31, 0x6c
  out 0x0a, r31

  ; PORTD value:  0x91  (Port D Data Register)
  ;   PORTD0         1  Debugging (RXD)             - pull-up enabled
  ;   PORTD1         0  Debugging (TXD)
  ;   PORTD2         0  Sync out                    - low
  ;   PORTD3         0  Audio output (OC2B)
  ;   PORTD4      0x10  Switch input 0              - pull-up enabled
  ;   PORTD5         0  Red LED (OC0B)
  ;   PORTD6         0  Green LED (OC0A)
  ;   PORTD7      0x80  Switch input 1              - pull-up enabled
  ldi r31, 0x91
  out 0x0b, r31

  ; TCCR0A value: 0xa3  (Timer/Counter 0 Control Register A)
  ;   WGM00          1  } Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
  ;   WGM01          2  }
  ;
  ;
  ;   COM0B0         0  } Compare Output Mode for Channel B: non-inverting mode
  ;   COM0B1      0x20  }
  ;   COM0A0         0  } Compare Output Mode for Channel A: non-inverting mode
  ;   COM0A1      0x80  }
  ldi r31, 0xa3
  out 0x24, r31

  ; TCCR0B value: 0x01  (Timer/Counter 0 Control Register B)
  ;   CS00           1  } Clock select: clkIO/1 (no prescaling)
  ;   CS01           0  }
  ;   CS02           0  }
  ;   WGM02          0  Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
  ;
  ;
  ;   FOC0B          0  Force Output Compare B
  ;   FOC0A          0  Force Output Compare A
  ldi r31, 0x01
  out 0x25, r31

  ; OCR0A value: green LED brightness
  ldi r31, 0x00
  out 0x27, r31
  ; OCR0B value: red LED brightness
  out 0x28, r31

  ; SPCR value:   0x50  (SPI Control Register)
  ;   SPR0           0  } SPI Clock Rate Select: fOSC/2
  ;   SPR1           0  }
  ;   CPHA           0  Clock Phase: sample on leading edge, setup on trailing edge
  ;   CPOL           0  Clock Polarity: leading edge = rising, trailing edge = falling
  ;   MSTR        0x10  Master/Slave Select: master
  ;   DORD           0  Data Order: MSB first
  ;   SPE         0x40  SPI Enable: enabled
  ;   SPIE           0  SPI Interrupt Enable: disabled
  ldi r31, 0x50
  out 0x2c, r31

  ; SPSR value:   0x01  (SPI Status Register)
  ;  SPI2X           1  SPI Clock Rate Select: fOSC/2
  ldi r31, 0x01
  out 0x2d, r31

  ; Set up stack:
  ldi r31, lo8(stackEnd-1)
  out 0x3d, r31
  ldi r31, hi8(stackEnd-1)
  out 0x3e, r31

  ; SPDR (SPI data register) port 0x2e - shift register data

  ; TIMSK0 value: 0x00  (Timer/Counter 0 Interrupt Mask Register)
  ;   TOIE0          0  Timer 0 overflow:  no interrupt
  ;   OCIE0A         0  Timer 0 compare A: no interrupt
  ;   OCIE0B         0  Timer 0 compare B: no interrupt
  ldi r31, 0x00
  sts 0x6e, r31

  ; TIMSK1 value: 0x01  (Timer/Counter 1 Interrupt Mask Register)
  ;   TOIE1          1  Timer 1 overflow:  interrupt
  ;   OCIE1A         0  Timer 1 compare A: no interrupt
  ;   OCIE1B         0  Timer 1 compare B: no interrupt
  ;
  ;
  ;   ICIE1          0  Timer 1 input capture: no interrupt
  ldi r31, 0x01
  sts 0x6f, r31

  ; TIMSK2 value: 0x00  (Timer/Counter 2 Interrupt Mask Register)
  ;   TOIE2          0  Timer 2 overflow:  no interrupt
  ;   OCIE2A         0  Timer 2 compare A: no interrupt
  ;   OCIE2B         0  Timer 2 compare B: no interrupt
  ldi r31, 0x00
  sts 0x70, r31

  ; ADCSRA value: 0xc7  (ADC Control and Status Register A)
  ;   ADPS0          1  ADC Prescaler Bit 0  }
  ;   ADPS1          2  ADC Prescaler Bit 1  } 16MHz/128 = 125KHz (between 50KHz and 200KHz)
  ;   ADPS2          4  ADC Prescaler Bit 2  }
  ;   ADIE           0  ADC Interrupt Enable: no interrupt
  ;   ADIF           0  ADC Interrupt Flag: no ack
  ;   ADATE          0  ADC Auto Trigger Enable: disabled
  ;   ADSC        0x40  ADC Start Conversion: not started
  ;   ADEN        0x80  ADC Enable: enabled
  ; ldi r31, 0xc7      ; Do this after programming ADMUX so the ADC knows which channel to read
  ; sts 0x7a, r31

  ; ADCSRB value: 0x00  (ADC Control and Status Register B)
  ;   ADTS0          0  ADC Auto Trigger Source Bit 0
  ;   ADTS1          0  ADC Auto Trigger Source Bit 1
  ;   ADTS2          0  ADC Auto Trigger Source Bit 2
  ;
  ;
  ;
  ;   ACME           0  Analog Comparator Multiplexer Enable: disabled
  ;
  ldi r31, 0x00
  sts 0x7b, r31

  ; ADMUX value:  0x44  (ADC Multiplexer Selection Register)
  ;   MUX0           0  Analog Channel Selection Bit 0  }
  ;   MUX1           0  Analog Channel Selection Bit 1  } Source is ADC4
  ;   MUX2           4  Analog Channel Selection Bit 2  }
  ;   MUX3           0  Analog Channel Selection Bit 3  }
  ;
  ;   ADLAR          0  ADC Left Adjust Result: not shifted
  ;   REFS0       0x40  Reference Selection Bit 0  } Voltage reference is AVCC
  ;   REFS1          0  Reference Selection Bit 1  }
  ldi r31, 0x44
  sts 0x7c, r31
  ldi r31, 0xc7   ; postponed from above
  sts 0x7a, r31

  ; DIDR0 value:  0x30  (Digital Input Disable Register 0)
  ;   ADC0D          0  ADC0 Digital Input Disable
  ;   ADC1D          0  ADC1 Digital Input Disable
  ;   ADC2D          0  ADC2 Digital Input Disable
  ;   ADC3D          0  ADC3 Digital Input Disable
  ;   ADC4D       0x10  ADC4 Digital Input Disable
  ;   ADC5D       0x20  ADC5 Digital Input Disable
  ldi r31, 0x30
  sts 0x7e, r31

  ; TCCR1A value: 0x82  (Timer/Counter 1 Control Register A)
  ;   WGM10          0  } Waveform Generation Mode = 14 (Fast PWM, TOP=ICR1)
  ;   WGM11          2  }
  ;
  ;
  ;   COM1B0         0  } Compare Output Mode for Channel B: normal port operation, OC1B disconnected
  ;   COM1B1         0  }
  ;   COM1A0         0  } Compare Output Mode for Channel A: non-inverting mode
  ;   COM1A1      0x80  }
  ldi r31, 0x82
  sts 0x80, r31

  ; TCCR1B value: 0x19  (Timer/Counter 1 Control Register B)
  ;   CS10           1  } Clock select: clkIO/1 (no prescaling)
  ;   CS11           0  }
  ;   CS12           0  }
  ;   WGM12          8  } Waveform Generation Mode = 14 (Fast PWM, TOP=ICR1)
  ;   WGM13       0x10  }
  ;
  ;   ICES1          0  Input Capture Edge Select: falling
  ;   ICNC1          0  Input Capture Noise Canceler: disabled
  ldi r31, 0x19
  sts 0x81, r31

  ; TCCR1C value: 0x00  (Timer/Counter 1 Control Register C)
  ;
  ;
  ;
  ;
  ;
  ;
  ;   FOC1B          0  Force Output Compare for Channel B
  ;   FOC1A          0  Force Output Compare for Channel A
  ldi r31, 0x00
  sts 0x82, r31

  ; ICR1 value: 0x0400  (Timer/Counter 1 Input Capture Register)
  ;   Timer 1 overflow frequency (15.625KHz at 16MHz clock frequency)
  ldi r31, 0x04
  sts 0x87, r31
  ldi r31, 0x00
  sts 0x86, r31

  ; OCR1A value: blue LED brightness
  ldi r31, 0x00
  sts 0x89, r31
  sts 0x88, r31

  ; TCCR2A value: 0x23  (Timer/Counter 2 Control Register A)
  ;   WGM20          1  } Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
  ;   WGM21          2  }
  ;
  ;
  ;   COM2B0         0  } Compare Output Mode for Channel B: non-inverting mode
  ;   COM2B1      0x20  }
  ;   COM2A0         0  } Compare Output Mode for Channel A: normal port operation, OC2A disconnected
  ;   COM2A1         0  }
  ldi r31, 0x23
  sts 0xb0, r31

  ; TCCR2B value: 0x01  (Timer/Counter 2 Control Register B)
  ;   CS20           1  } Clock select: clkIO/1 (no prescaling)
  ;   CS21           0  }
  ;   CS22           0  }
  ;   WGM22          0  Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
  ;
  ;
  ;   FOC2B          0  Force Output Compare B
  ;   FOC2A          0  Force Output Compare A
  ldi r31, 0x01
  sts 0xb1, r31

  ; OCR2B value: audio data
  ldi r31, 0x00
  sts 0xb4, r31

  ; UCSR0A value: 0x00  (USART Control and Status Register 0 A)
  ;   MPCM0          0  Multi-processor Communcation Mode: disabled
  ;   U2X0           0  Double the USART Transmission Speed: disabled
  ;
  ;
  ;
  ;
  ;   TXC0           0  USART Transmit Complete: not cleared
  ldi r31, 0x00
  sts 0xc0, r31

  ; UCSR0B value: 0xd8  (USART Control and Status Register 0 B)
  ;   TXB80          0  Transmit Data Bit 8 0
  ;
  ;   UCSZ02         0  Character Size 0: 8 bit
  ;   TXEN0          8  Transmitter Enable 0: enabled
  ;   RXEN0       0x10  Receiver Enable 0: enabled
  ;   UDRIE0         0  USART Data Register Empty Interrupt Enable 0: disabled
  ;   TXCIE0      0x40  TX Complete Interrupt Enable 0: enabled
  ;   RXCIE0      0x80  RX Complete Interrupt Enable 0: enabled
  ldi r31, 0xd8
  sts 0xc1, r31

  ; UCSR0C value: 0x06  (USART Control and Status Register 0 C)
  ;   UCPOL0         0  Clock Polarity
  ;   UCSZ00         2  Character Size: 8 bit
  ;   UCSZ01         4  Character Size: 8 bit
  ;   USBS0          0  Stop Bit Select: 1-bit
  ;   UPM00          0  Parity Mode: disabled
  ;   UPM01          0  Parity Mode: disabled
  ;   UMSEL00        0  USART Mode Select: asynchronous
  ;   UMSEL01        0  USART Mode Select: asynchronous
  ldi r31, 0x06
  sts 0xc2, r31

  ; UBRR0L value: 0x67  (USART Baud Rate Register Low) - 9600bps
  ldi r31, 0x67
  sts 0xc4, r31

  ; UBRR0H value: 0x00  (USART Baud Rate Register High)
  ldi r31, 0x00
  sts 0xc5, r31


; Initialize registers
  eor r2, r2                      ; r2 = 0
  mov r3, r2                      ; sampleInLine = 0
  mov r4, r2                      ; lineInFrame = 0
  mov r5, r2
  mov r6, r2                      ; frameInBeat = 0
  mov r7, r2
  ldi r31, 8
  mov r8, r31                     ; framesPerBeat = 0x800
  mov r9, r2                      ; beatInPattern = 0
  ldi r31, 6
  mov r10, r31                    ; lightPage = 6
  mov r11, r2                     ; switchInFrame = 0
  mov r12, r2                     ; (unused) = 0
  mov r13, r2                     ; (unused) = 0
  mov r14, r2                     ; beatsPerPattern = 0
  ldi r31, 4
  mov r15, r31                    ; waterPage = 4
  mov r16, r2                     ; lastSwitchPressed = 0
  ldi r31, 0x40
  mov r17, r31                    ; flags = 0  (waitingForSync = false,  lifeMode = false,  receivedSync = false,  outputSyncPulseActive = false,  gotRandom = false,  randomMode = false,  patternMode = true,  switchTouched = false)
  ldi r18, 1                      ; r18 = 1
  ldi r19, 0x10                   ; r19 = 0x10

; Initialize variables
  sts patternsPerLoop, r18
  ldi r31, 4
  sts flags2, r31  ; spaceAvailable = true


; Clear RAM. libgcc's __do_clear_bss will do this but then the entry code will
; call main, leaving the return address on the stack (in memory we want to
; use).
  ldi r31, 1
  ldi r30, 0
clearLoop:
  st Z+, r2
  cpi r31, 9
  brne clearLoop


; Copy some data from program memory to RAM. We do this here instead of
; putting it in .data and using libgcc's __do_copy_data because we might want
; to reference the program memory sine table again later (so that we can make
; the waveform user-modifyable, but still put it back to a sinewave without a
; reset).

  rcall copySine

  ldi r26, lo8(frequencies)        ; lo8(X)
  ldi r27, hi8(frequencies)        ; hi8(X)
  ldi r30, lo8(defaultFrequencies) ; lo8(Z)
  ldi r31, hi8(defaultFrequencies) ; hi8(Z)
copyDataLoop:                      ; do {
  lpm r0, Z+                       ;   r0 = flash[Z++]
  st X+, r0                        ;   ram[X++] = flash[Z++]
  cpi r26, lo8(frequencies + 0x20) ;
  brne copyDataLoop                ; } while (lo8(X) != frequencies + 0x20);

; Initialize microtoneKeys (the exact values don't matter - they just need to be all different)
  ldi r29, 0
  ldi r30, lo8(microtoneKeys)
  ldi r31, hi8(microtoneKeys)
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29
  inc r29
  st Z+, r29

  sei  ; enable interrupts

  jmp idleLoop


.section .bss

.global waveform
waveform:              ; 100-200
  .skip 0x100
.global frequencies
frequencies:           ; 200-220
  .skip 0x20
positions:             ; 220-240
  .skip 0x20
.global velocities
velocities:            ; 240-260
  .skip 0x20
.global volumes
volumes:               ; 260-270
  .skip 0x10
lineBuffer:            ; 270-280
  .skip 0x10
.global microtoneKeys
microtoneKeys:         ; 280-290
  .skip 0x10
randomData:            ; 290-294
  .skip 4
randomOn:              ; 294-295
  .skip 1
randomOff:             ; 295-296
  .skip 1
.global noiseUpdatePointer
noiseUpdatePointer:    ; 296-297
  .skip 1
patternInLoop:         ; 297-298
  .skip 1
.global patternsPerLoop
patternsPerLoop:       ; 298-299
  .skip 1
adcChannel:            ; 299-29a
  .skip 1
.global editor
editor:                ; 29a-29b
  .skip 1
.global waveformPreset
waveformPreset:        ; 29b-29c
  .skip 1
.global decayConstant
decayConstant:         ; 29c-29d
  .skip 1
.global decayConstantOverride
decayConstantOverride: ; 29d-29e
  .skip 1
.global framesPerBeatOverride
framesPerBeatOverride: ; 29e-2a0
  .skip 2
.global flags2
flags2:                ; 2a0-2a1  bit 0: fingerDown  bit 1: microtoneMode  bit 2: spaceAvailable  bit 3: fixedTuning  bit 4: updateNoise  bit 5: clearMode  bit 6: escapeTouched
  .skip 1
.global sendState
sendState:             ; 2a1-2a2
  .skip 1
.global receiveState
receiveState:          ; 2a2-2a3
  .skip 1
.global debugValue
debugValue:            ; 2a3-2a4
  .skip 1
.global debugAddress
debugAddress:          ; 2a4-2a6
  .skip 2
.global serialLED
serialLED:             ; 2a6-2a7
  .skip 1
.global mode
mode:                  ; 2a7-2a8
  .skip 1
nextFramesPerBeatLow:  ; 2a8-2a9
  .skip 1
nextFramesPerBeatHigh: ; 2a9-2aa
  .skip 1
nextLightsLit:         ; 2aa-2ab
  .skip 1
stack:                 ; 2ab-300  ; 85 bytes
  .skip 0x55
stackEnd:
.global frameBuffer
frameBuffer:           ; 300-400
  .skip 0x100
.global waterBuffers
waterBuffers:          ; 400-600
  .skip 0x200
.global lightBuffers
lightBuffers:          ; 600-800
  .skip 0x200
.global switchBuffer
switchBuffer:          ; 800-900
  .skip 0x100


; Register allocation:
;   multiplication result low   r0
;   multiplication result high  r1
;   0                           r2
;   sampleInLine                r3  (only 4 bits used - duplicates low bits of r11)
;   lineInFrame                 r4  (only 4 bits used - duplicates high bits of r11)
;   frameInBeat low             r5
;   frameInBeat high            r6
;   framesPerBeat low           r7
;   framesPerBeat high          r8
;   beatInPattern               r9  (only 4 bits used)
;   lightPage                   r10 (only 1 bit significant)
;   switchInFrame               r11
;   lastSwitch                  r12
;   switchesTouched             r13
;   beatsPerPattern             r14
;   waterPage                   r15 (only 1 bit significant)
;   lightsLit                   r16
;   flags                       r17 bit 0: waitingForSync  bit 1: lifeMode  bit 2: receivedSync  bit 3: outputSyncPulseActive  bit 4: gotRandom  bit 5: randomMode  bit 6: patternMode  bit 7: switchTouched
;   1                           r18
;   0x10                        r19
;   (scratch)                   r20
;   (scratch)                   r21
;   (scratch)                   r22
;   (scratch)                   r23
;   (scratch)                   r24
;   (scratch)                   r25
;   X low                       r26
;   X high                      r27
;   Y low                       r28
;   Y high                      r29
;   Z low                       r30
;   Z high                      r31


.section .progmem.data,"a",@progbits

.align 8

; table of 2^x for x between 14.5 and 15.5 in steps of 1/1024.
.global tuningMultipliers
tuningMultipliers:    ; 0000 - 0800
  .hword 0x5a82, 0x5a92, 0x5aa1, 0x5ab1, 0x5ac1, 0x5ad1, 0x5ae0, 0x5af0
  .hword 0x5b00, 0x5b10, 0x5b1f, 0x5b2f, 0x5b3f, 0x5b4f, 0x5b5f, 0x5b6e
  .hword 0x5b7e, 0x5b8e, 0x5b9e, 0x5bae, 0x5bbe, 0x5bce, 0x5bde, 0x5bee
  .hword 0x5bfd, 0x5c0d, 0x5c1d, 0x5c2d, 0x5c3d, 0x5c4d, 0x5c5d, 0x5c6d
  .hword 0x5c7d, 0x5c8d, 0x5c9d, 0x5cad, 0x5cbe, 0x5cce, 0x5cde, 0x5cee
  .hword 0x5cfe, 0x5d0e, 0x5d1e, 0x5d2e, 0x5d3e, 0x5d4f, 0x5d5f, 0x5d6f
  .hword 0x5d7f, 0x5d8f, 0x5da0, 0x5db0, 0x5dc0, 0x5dd0, 0x5de1, 0x5df1
  .hword 0x5e01, 0x5e11, 0x5e22, 0x5e32, 0x5e42, 0x5e53, 0x5e63, 0x5e73
  .hword 0x5e84, 0x5e94, 0x5ea5, 0x5eb5, 0x5ec5, 0x5ed6, 0x5ee6, 0x5ef7
  .hword 0x5f07, 0x5f18, 0x5f28, 0x5f39, 0x5f49, 0x5f5a, 0x5f6a, 0x5f7b
  .hword 0x5f8b, 0x5f9c, 0x5fac, 0x5fbd, 0x5fce, 0x5fde, 0x5fef, 0x5fff
  .hword 0x6010, 0x6021, 0x6031, 0x6042, 0x6053, 0x6063, 0x6074, 0x6085
  .hword 0x6096, 0x60a6, 0x60b7, 0x60c8, 0x60d9, 0x60e9, 0x60fa, 0x610b
  .hword 0x611c, 0x612d, 0x613e, 0x614e, 0x615f, 0x6170, 0x6181, 0x6192
  .hword 0x61a3, 0x61b4, 0x61c5, 0x61d6, 0x61e7, 0x61f8, 0x6209, 0x621a
  .hword 0x622b, 0x623c, 0x624d, 0x625e, 0x626f, 0x6280, 0x6291, 0x62a2
  .hword 0x62b3, 0x62c4, 0x62d5, 0x62e6, 0x62f8, 0x6309, 0x631a, 0x632b
  .hword 0x633c, 0x634d, 0x635f, 0x6370, 0x6381, 0x6392, 0x63a4, 0x63b5
  .hword 0x63c6, 0x63d8, 0x63e9, 0x63fa, 0x640b, 0x641d, 0x642e, 0x6440
  .hword 0x6451, 0x6462, 0x6474, 0x6485, 0x6497, 0x64a8, 0x64b9, 0x64cb
  .hword 0x64dc, 0x64ee, 0x64ff, 0x6511, 0x6522, 0x6534, 0x6545, 0x6557
  .hword 0x6569, 0x657a, 0x658c, 0x659d, 0x65af, 0x65c1, 0x65d2, 0x65e4
  .hword 0x65f6, 0x6607, 0x6619, 0x662b, 0x663c, 0x664e, 0x6660, 0x6672
  .hword 0x6683, 0x6695, 0x66a7, 0x66b9, 0x66ca, 0x66dc, 0x66ee, 0x6700
  .hword 0x6712, 0x6724, 0x6736, 0x6747, 0x6759, 0x676b, 0x677d, 0x678f
  .hword 0x67a1, 0x67b3, 0x67c5, 0x67d7, 0x67e9, 0x67fb, 0x680d, 0x681f
  .hword 0x6831, 0x6843, 0x6855, 0x6867, 0x6879, 0x688c, 0x689e, 0x68b0
  .hword 0x68c2, 0x68d4, 0x68e6, 0x68f8, 0x690b, 0x691d, 0x692f, 0x6941
  .hword 0x6954, 0x6966, 0x6978, 0x698a, 0x699d, 0x69af, 0x69c1, 0x69d4
  .hword 0x69e6, 0x69f8, 0x6a0b, 0x6a1d, 0x6a2f, 0x6a42, 0x6a54, 0x6a67
  .hword 0x6a79, 0x6a8c, 0x6a9e, 0x6ab1, 0x6ac3, 0x6ad6, 0x6ae8, 0x6afb
  .hword 0x6b0d, 0x6b20, 0x6b32, 0x6b45, 0x6b57, 0x6b6a, 0x6b7d, 0x6b8f
  .hword 0x6ba2, 0x6bb5, 0x6bc7, 0x6bda, 0x6bed, 0x6bff, 0x6c12, 0x6c25
  .hword 0x6c38, 0x6c4a, 0x6c5d, 0x6c70, 0x6c83, 0x6c96, 0x6ca8, 0x6cbb
  .hword 0x6cce, 0x6ce1, 0x6cf4, 0x6d07, 0x6d1a, 0x6d2c, 0x6d3f, 0x6d52
  .hword 0x6d65, 0x6d78, 0x6d8b, 0x6d9e, 0x6db1, 0x6dc4, 0x6dd7, 0x6dea
  .hword 0x6dfd, 0x6e10, 0x6e24, 0x6e37, 0x6e4a, 0x6e5d, 0x6e70, 0x6e83
  .hword 0x6e96, 0x6ea9, 0x6ebd, 0x6ed0, 0x6ee3, 0x6ef6, 0x6f09, 0x6f1d
  .hword 0x6f30, 0x6f43, 0x6f57, 0x6f6a, 0x6f7d, 0x6f90, 0x6fa4, 0x6fb7
  .hword 0x6fcb, 0x6fde, 0x6ff1, 0x7005, 0x7018, 0x702c, 0x703f, 0x7052
  .hword 0x7066, 0x7079, 0x708d, 0x70a0, 0x70b4, 0x70c7, 0x70db, 0x70ef
  .hword 0x7102, 0x7116, 0x7129, 0x713d, 0x7151, 0x7164, 0x7178, 0x718c
  .hword 0x719f, 0x71b3, 0x71c7, 0x71da, 0x71ee, 0x7202, 0x7216, 0x7229
  .hword 0x723d, 0x7251, 0x7265, 0x7279, 0x728d, 0x72a0, 0x72b4, 0x72c8
  .hword 0x72dc, 0x72f0, 0x7304, 0x7318, 0x732c, 0x7340, 0x7354, 0x7368
  .hword 0x737c, 0x7390, 0x73a4, 0x73b8, 0x73cc, 0x73e0, 0x73f4, 0x7408
  .hword 0x741c, 0x7430, 0x7444, 0x7459, 0x746d, 0x7481, 0x7495, 0x74a9
  .hword 0x74be, 0x74d2, 0x74e6, 0x74fa, 0x750f, 0x7523, 0x7537, 0x754c
  .hword 0x7560, 0x7574, 0x7589, 0x759d, 0x75b1, 0x75c6, 0x75da, 0x75ef
  .hword 0x7603, 0x7618, 0x762c, 0x7640, 0x7655, 0x7669, 0x767e, 0x7693
  .hword 0x76a7, 0x76bc, 0x76d0, 0x76e5, 0x76f9, 0x770e, 0x7723, 0x7737
  .hword 0x774c, 0x7761, 0x7775, 0x778a, 0x779f, 0x77b4, 0x77c8, 0x77dd
  .hword 0x77f2, 0x7807, 0x781b, 0x7830, 0x7845, 0x785a, 0x786f, 0x7884
  .hword 0x7899, 0x78ae, 0x78c2, 0x78d7, 0x78ec, 0x7901, 0x7916, 0x792b
  .hword 0x7940, 0x7955, 0x796a, 0x797f, 0x7994, 0x79a9, 0x79bf, 0x79d4
  .hword 0x79e9, 0x79fe, 0x7a13, 0x7a28, 0x7a3d, 0x7a53, 0x7a68, 0x7a7d
  .hword 0x7a92, 0x7aa7, 0x7abd, 0x7ad2, 0x7ae7, 0x7afd, 0x7b12, 0x7b27
  .hword 0x7b3d, 0x7b52, 0x7b67, 0x7b7d, 0x7b92, 0x7ba8, 0x7bbd, 0x7bd2
  .hword 0x7be8, 0x7bfd, 0x7c13, 0x7c28, 0x7c3e, 0x7c53, 0x7c69, 0x7c7f
  .hword 0x7c94, 0x7caa, 0x7cbf, 0x7cd5, 0x7ceb, 0x7d00, 0x7d16, 0x7d2c
  .hword 0x7d41, 0x7d57, 0x7d6d, 0x7d83, 0x7d98, 0x7dae, 0x7dc4, 0x7dda
  .hword 0x7def, 0x7e05, 0x7e1b, 0x7e31, 0x7e47, 0x7e5d, 0x7e73, 0x7e89
  .hword 0x7e9f, 0x7eb4, 0x7eca, 0x7ee0, 0x7ef6, 0x7f0c, 0x7f22, 0x7f38
  .hword 0x7f4f, 0x7f65, 0x7f7b, 0x7f91, 0x7fa7, 0x7fbd, 0x7fd3, 0x7fe9
  .hword 0x8000, 0x8016, 0x802c, 0x8042, 0x8058, 0x806f, 0x8085, 0x809b
  .hword 0x80b1, 0x80c8, 0x80de, 0x80f4, 0x810b, 0x8121, 0x8138, 0x814e
  .hword 0x8164, 0x817b, 0x8191, 0x81a8, 0x81be, 0x81d5, 0x81eb, 0x8202
  .hword 0x8218, 0x822f, 0x8245, 0x825c, 0x8272, 0x8289, 0x82a0, 0x82b6
  .hword 0x82cd, 0x82e4, 0x82fa, 0x8311, 0x8328, 0x833f, 0x8355, 0x836c
  .hword 0x8383, 0x839a, 0x83b0, 0x83c7, 0x83de, 0x83f5, 0x840c, 0x8423
  .hword 0x843a, 0x8451, 0x8468, 0x847e, 0x8495, 0x84ac, 0x84c3, 0x84da
  .hword 0x84f1, 0x8509, 0x8520, 0x8537, 0x854e, 0x8565, 0x857c, 0x8593
  .hword 0x85aa, 0x85c1, 0x85d9, 0x85f0, 0x8607, 0x861e, 0x8636, 0x864d
  .hword 0x8664, 0x867b, 0x8693, 0x86aa, 0x86c1, 0x86d9, 0x86f0, 0x8707
  .hword 0x871f, 0x8736, 0x874e, 0x8765, 0x877d, 0x8794, 0x87ac, 0x87c3
  .hword 0x87db, 0x87f2, 0x880a, 0x8821, 0x8839, 0x8851, 0x8868, 0x8880
  .hword 0x8898, 0x88af, 0x88c7, 0x88df, 0x88f6, 0x890e, 0x8926, 0x893e
  .hword 0x8955, 0x896d, 0x8985, 0x899d, 0x89b5, 0x89cd, 0x89e5, 0x89fc
  .hword 0x8a14, 0x8a2c, 0x8a44, 0x8a5c, 0x8a74, 0x8a8c, 0x8aa4, 0x8abc
  .hword 0x8ad4, 0x8aec, 0x8b04, 0x8b1d, 0x8b35, 0x8b4d, 0x8b65, 0x8b7d
  .hword 0x8b95, 0x8bad, 0x8bc6, 0x8bde, 0x8bf6, 0x8c0e, 0x8c27, 0x8c3f
  .hword 0x8c57, 0x8c70, 0x8c88, 0x8ca0, 0x8cb9, 0x8cd1, 0x8cea, 0x8d02
  .hword 0x8d1a, 0x8d33, 0x8d4b, 0x8d64, 0x8d7c, 0x8d95, 0x8dad, 0x8dc6
  .hword 0x8ddf, 0x8df7, 0x8e10, 0x8e28, 0x8e41, 0x8e5a, 0x8e72, 0x8e8b
  .hword 0x8ea4, 0x8ebc, 0x8ed5, 0x8eee, 0x8f07, 0x8f20, 0x8f38, 0x8f51
  .hword 0x8f6a, 0x8f83, 0x8f9c, 0x8fb5, 0x8fce, 0x8fe6, 0x8fff, 0x9018
  .hword 0x9031, 0x904a, 0x9063, 0x907c, 0x9095, 0x90af, 0x90c8, 0x90e1
  .hword 0x90fa, 0x9113, 0x912c, 0x9145, 0x915e, 0x9178, 0x9191, 0x91aa
  .hword 0x91c3, 0x91dd, 0x91f6, 0x920f, 0x9228, 0x9242, 0x925b, 0x9275
  .hword 0x928e, 0x92a7, 0x92c1, 0x92da, 0x92f4, 0x930d, 0x9327, 0x9340
  .hword 0x935a, 0x9373, 0x938d, 0x93a6, 0x93c0, 0x93da, 0x93f3, 0x940d
  .hword 0x9426, 0x9440, 0x945a, 0x9474, 0x948d, 0x94a7, 0x94c1, 0x94db
  .hword 0x94f4, 0x950e, 0x9528, 0x9542, 0x955c, 0x9576, 0x9590, 0x95aa
  .hword 0x95c3, 0x95dd, 0x95f7, 0x9611, 0x962b, 0x9645, 0x9660, 0x967a
  .hword 0x9694, 0x96ae, 0x96c8, 0x96e2, 0x96fc, 0x9716, 0x9731, 0x974b
  .hword 0x9765, 0x977f, 0x9799, 0x97b4, 0x97ce, 0x97e8, 0x9803, 0x981d
  .hword 0x9837, 0x9852, 0x986c, 0x9887, 0x98a1, 0x98bc, 0x98d6, 0x98f1
  .hword 0x990b, 0x9926, 0x9940, 0x995b, 0x9975, 0x9990, 0x99aa, 0x99c5
  .hword 0x99e0, 0x99fa, 0x9a15, 0x9a30, 0x9a4b, 0x9a65, 0x9a80, 0x9a9b
  .hword 0x9ab6, 0x9ad0, 0x9aeb, 0x9b06, 0x9b21, 0x9b3c, 0x9b57, 0x9b72
  .hword 0x9b8d, 0x9ba8, 0x9bc3, 0x9bde, 0x9bf9, 0x9c14, 0x9c2f, 0x9c4a
  .hword 0x9c65, 0x9c80, 0x9c9b, 0x9cb6, 0x9cd2, 0x9ced, 0x9d08, 0x9d23
  .hword 0x9d3e, 0x9d5a, 0x9d75, 0x9d90, 0x9dab, 0x9dc7, 0x9de2, 0x9dfe
  .hword 0x9e19, 0x9e34, 0x9e50, 0x9e6b, 0x9e87, 0x9ea2, 0x9ebe, 0x9ed9
  .hword 0x9ef5, 0x9f10, 0x9f2c, 0x9f47, 0x9f63, 0x9f7f, 0x9f9a, 0x9fb6
  .hword 0x9fd2, 0x9fed, 0xa009, 0xa025, 0xa041, 0xa05c, 0xa078, 0xa094
  .hword 0xa0b0, 0xa0cc, 0xa0e8, 0xa103, 0xa11f, 0xa13b, 0xa157, 0xa173
  .hword 0xa18f, 0xa1ab, 0xa1c7, 0xa1e3, 0xa1ff, 0xa21b, 0xa238, 0xa254
  .hword 0xa270, 0xa28c, 0xa2a8, 0xa2c4, 0xa2e1, 0xa2fd, 0xa319, 0xa335
  .hword 0xa352, 0xa36e, 0xa38a, 0xa3a7, 0xa3c3, 0xa3df, 0xa3fc, 0xa418
  .hword 0xa435, 0xa451, 0xa46e, 0xa48a, 0xa4a7, 0xa4c3, 0xa4e0, 0xa4fc
  .hword 0xa519, 0xa535, 0xa552, 0xa56f, 0xa58b, 0xa5a8, 0xa5c5, 0xa5e2
  .hword 0xa5fe, 0xa61b, 0xa638, 0xa655, 0xa672, 0xa68e, 0xa6ab, 0xa6c8
  .hword 0xa6e5, 0xa702, 0xa71f, 0xa73c, 0xa759, 0xa776, 0xa793, 0xa7b0
  .hword 0xa7cd, 0xa7ea, 0xa807, 0xa824, 0xa842, 0xa85f, 0xa87c, 0xa899
  .hword 0xa8b6, 0xa8d4, 0xa8f1, 0xa90e, 0xa92b, 0xa949, 0xa966, 0xa983
  .hword 0xa9a1, 0xa9be, 0xa9dc, 0xa9f9, 0xaa17, 0xaa34, 0xaa52, 0xaa6f
  .hword 0xaa8d, 0xaaaa, 0xaac8, 0xaae5, 0xab03, 0xab21, 0xab3e, 0xab5c
  .hword 0xab7a, 0xab97, 0xabb5, 0xabd3, 0xabf1, 0xac0f, 0xac2c, 0xac4a
  .hword 0xac68, 0xac86, 0xaca4, 0xacc2, 0xace0, 0xacfe, 0xad1c, 0xad3a
  .hword 0xad58, 0xad76, 0xad94, 0xadb2, 0xadd0, 0xadee, 0xae0c, 0xae2b
  .hword 0xae49, 0xae67, 0xae85, 0xaea3, 0xaec2, 0xaee0, 0xaefe, 0xaf1d
  .hword 0xaf3b, 0xaf59, 0xaf78, 0xaf96, 0xafb5, 0xafd3, 0xaff2, 0xb010
  .hword 0xb02f, 0xb04d, 0xb06c, 0xb08a, 0xb0a9, 0xb0c7, 0xb0e6, 0xb105
  .hword 0xb123, 0xb142, 0xb161, 0xb180, 0xb19e, 0xb1bd, 0xb1dc, 0xb1fb
  .hword 0xb21a, 0xb239, 0xb257, 0xb276, 0xb295, 0xb2b4, 0xb2d3, 0xb2f2
  .hword 0xb311, 0xb330, 0xb34f, 0xb36e, 0xb38e, 0xb3ad, 0xb3cc, 0xb3eb
  .hword 0xb40a, 0xb429, 0xb449, 0xb468, 0xb487, 0xb4a6, 0xb4c6, 0xb4e5

; table of 27.5Hz * 2(x/34) times 0x8000000/16000000Hz for x between 0 and 255 inclusive.
; These are frequencies in microtone keyboard mode (34-TET).
.global microtoneFrequencies
microtoneFrequencies: ; 0800 - 0a00
  .hword 0x00e6, 0x00eb, 0x00f0, 0x00f5, 0x00fa, 0x00ff, 0x0104, 0x010a
  .hword 0x010f, 0x0115, 0x011a, 0x0120, 0x0126, 0x012c, 0x0132, 0x0139
  .hword 0x013f, 0x0146, 0x014c, 0x0153, 0x015a, 0x0161, 0x0169, 0x0170
  .hword 0x0178, 0x0180, 0x0187, 0x0190, 0x0198, 0x01a0, 0x01a9, 0x01b2
  .hword 0x01ba, 0x01c4, 0x01cd, 0x01d6, 0x01e0, 0x01ea, 0x01f4, 0x01fe
  .hword 0x0209, 0x0214, 0x021f, 0x022a, 0x0235, 0x0241, 0x024d, 0x0259
  .hword 0x0265, 0x0272, 0x027f, 0x028c, 0x0299, 0x02a7, 0x02b5, 0x02c3
  .hword 0x02d2, 0x02e1, 0x02f0, 0x0300, 0x030f, 0x0320, 0x0330, 0x0341
  .hword 0x0352, 0x0364, 0x0375, 0x0388, 0x039a, 0x03ad, 0x03c1, 0x03d4
  .hword 0x03e9, 0x03fd, 0x0412, 0x0428, 0x043e, 0x0454, 0x046b, 0x0482
  .hword 0x049a, 0x04b2, 0x04cb, 0x04e4, 0x04fe, 0x0518, 0x0533, 0x054f
  .hword 0x056b, 0x0587, 0x05a4, 0x05c2, 0x05e1, 0x0600, 0x061f, 0x0640
  .hword 0x0661, 0x0682, 0x06a4, 0x06c8, 0x06eb, 0x0710, 0x0735, 0x075b
  .hword 0x0782, 0x07a9, 0x07d2, 0x07fb, 0x0825, 0x0850, 0x087c, 0x08a9
  .hword 0x08d6, 0x0905, 0x0934, 0x0965, 0x0997, 0x09c9, 0x09fd, 0x0a31
  .hword 0x0a67, 0x0a9e, 0x0ad6, 0x0b0f, 0x0b49, 0x0b85, 0x0bc2, 0x0c00
  .hword 0x0c3f, 0x0c80, 0x0cc2, 0x0d05, 0x0d49, 0x0d90, 0x0dd7, 0x0e20
  .hword 0x0e6a, 0x0eb7, 0x0f04, 0x0f53, 0x0fa4, 0x0ff7, 0x104b, 0x10a1
  .hword 0x10f8, 0x1152, 0x11ad, 0x120a, 0x1269, 0x12cb, 0x132e, 0x1393
  .hword 0x13fa, 0x1463, 0x14cf, 0x153d, 0x15ad, 0x161f, 0x1693, 0x170b
  .hword 0x1784, 0x1800, 0x187f, 0x1900, 0x1984, 0x1a0a, 0x1a93, 0x1b20
  .hword 0x1baf, 0x1c41, 0x1cd5, 0x1d6e, 0x1e09, 0x1ea7, 0x1f49, 0x1fee
  .hword 0x2096, 0x2142, 0x21f1, 0x22a4, 0x235b, 0x2415, 0x24d3, 0x2596
  .hword 0x265c, 0x2726, 0x27f5, 0x28c7, 0x299e, 0x2a7a, 0x2b5a, 0x2c3e
  .hword 0x2d27, 0x2e16, 0x2f09, 0x3001, 0x30fe, 0x3200, 0x3308, 0x3415
  .hword 0x3527, 0x3640, 0x375e, 0x3882, 0x39ab, 0x3adc, 0x3c12, 0x3d4f
  .hword 0x3e92, 0x3fdc, 0x412c, 0x4284, 0x43e3, 0x4549, 0x46b6, 0x482b
  .hword 0x49a7, 0x4b2c, 0x4cb8, 0x4e4d, 0x4fea, 0x518f, 0x533d, 0x54f4
  .hword 0x56b4, 0x587d, 0x5a4f, 0x5c2c, 0x5e12, 0x6002, 0x61fc, 0x6400
  .hword 0x6610, 0x682a, 0x6a4f, 0x6c80, 0x6ebc, 0x7104, 0x7357, 0x75b8
  .hword 0x7824, 0x7a9e, 0x7d24, 0x7fb8, 0x8259, 0x8509, 0x87c6, 0x8a92
  .hword 0x8d6d, 0x9056, 0x934f, 0x9658, 0x9971, 0x9c9a, 0x9fd4, 0xa31e

; table of sines in (360/256) degree increments in saturated signed 1.7 bit format.
.global sineTable
sineTable:            ; 0a00 - 0b00
  .byte 0x00, 0x03, 0x06, 0x09, 0x0c, 0x0f, 0x12, 0x15
  .byte 0x18, 0x1c, 0x1f, 0x22, 0x25, 0x28, 0x2b, 0x2e
  .byte 0x30, 0x33, 0x36, 0x39, 0x3c, 0x3f, 0x41, 0x44
  .byte 0x47, 0x49, 0x4c, 0x4e, 0x51, 0x53, 0x55, 0x58
  .byte 0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68
  .byte 0x6a, 0x6c, 0x6d, 0x6f, 0x70, 0x72, 0x73, 0x75
  .byte 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7c
  .byte 0x7d, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f
  .byte 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7e
  .byte 0x7d, 0x7c, 0x7c, 0x7b, 0x7a, 0x79, 0x78, 0x77
  .byte 0x76, 0x75, 0x73, 0x72, 0x70, 0x6f, 0x6d, 0x6c
  .byte 0x6a, 0x68, 0x66, 0x64, 0x62, 0x60, 0x5e, 0x5c
  .byte 0x5a, 0x58, 0x55, 0x53, 0x51, 0x4e, 0x4c, 0x49
  .byte 0x47, 0x44, 0x41, 0x3f, 0x3c, 0x39, 0x36, 0x33
  .byte 0x30, 0x2e, 0x2b, 0x28, 0x25, 0x22, 0x1f, 0x1c
  .byte 0x18, 0x15, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03
  .byte 0x00, 0xfd, 0xfa, 0xf7, 0xf4, 0xf1, 0xee, 0xeb
  .byte 0xe8, 0xe4, 0xe1, 0xde, 0xdb, 0xd8, 0xd5, 0xd2
  .byte 0xd0, 0xcd, 0xca, 0xc7, 0xc4, 0xc1, 0xbf, 0xbc
  .byte 0xb9, 0xb7, 0xb4, 0xb2, 0xaf, 0xad, 0xab, 0xa8
  .byte 0xa6, 0xa4, 0xa2, 0xa0, 0x9e, 0x9c, 0x9a, 0x98
  .byte 0x96, 0x94, 0x93, 0x91, 0x90, 0x8e, 0x8d, 0x8b
  .byte 0x8a, 0x89, 0x88, 0x87, 0x86, 0x85, 0x84, 0x84
  .byte 0x83, 0x82, 0x82, 0x81, 0x81, 0x81, 0x81, 0x81
  .byte 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x82, 0x82
  .byte 0x83, 0x84, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89
  .byte 0x8a, 0x8b, 0x8d, 0x8e, 0x90, 0x91, 0x93, 0x94
  .byte 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4
  .byte 0xa6, 0xa8, 0xab, 0xad, 0xaf, 0xb2, 0xb4, 0xb7
  .byte 0xb9, 0xbc, 0xbf, 0xc1, 0xc4, 0xc7, 0xca, 0xcd
  .byte 0xd0, 0xd2, 0xd5, 0xd8, 0xdb, 0xde, 0xe1, 0xe4
  .byte 0xe8, 0xeb, 0xee, 0xf1, 0xf4, 0xf7, 0xfa, 0xfd

divisionMultiplier:   ; 0b00 - 0d00
  .hword 0x0000, 0x0000, 0x8000, 0xaaab, 0x4000, 0xcccd, 0xaaab, 0x2493
  .hword 0x2000, 0xe38f, 0xcccd, 0xba2f, 0xaaab, 0x4ec5, 0x2493, 0x8889
  .hword 0x1000, 0xf0f1, 0xe38f, 0xd795, 0xcccd, 0x8619, 0xba2f, 0x642d
  .hword 0xaaab, 0x47af, 0x4ec5, 0x2f69, 0x2493, 0x469f, 0x8889, 0x0843
  .hword 0x0800, 0xf83f, 0xf0f1, 0xea0f, 0xe38f, 0x1bad, 0xd795, 0xa41b
  .hword 0xcccd, 0x8f9d, 0x8619, 0xbe83, 0xba2f, 0x2d83, 0x642d, 0x5c99
  .hword 0xaaab, 0x4e5f, 0x47af, 0xa0a1, 0x4ec5, 0x9a91, 0x2f69, 0x29e5
  .hword 0x2493, 0x1f71, 0x469f, 0x8ad9, 0x8889, 0x2193, 0x0843, 0x0411
  .hword 0x0400, 0x0fc1, 0xf83f, 0x7a45, 0xf0f1, 0xdae7, 0xea0f, 0xe6c3
  .hword 0xe38f, 0xe071, 0x1bad, 0xb4e9, 0xd795, 0x1a99, 0xa41b, 0xcf65
  .hword 0xcccd, 0x6523, 0x8f9d, 0x62b3, 0x8619, 0xc0c1, 0xbe83, 0xbc53
  .hword 0xba2f, 0x702f, 0x2d83, 0x6817, 0x642d, 0x6059, 0x5c99, 0xac77
  .hword 0xaaab, 0x51d1, 0x4e5f, 0xa57f, 0x47af, 0x1447, 0xa0a1, 0x4f89
  .hword 0x4ec5, 0x4e05, 0x9a91, 0x323f, 0x2f69, 0x0965, 0x29e5, 0x939b
  .hword 0x2493, 0x487f, 0x1f71, 0x1cf1, 0x469f, 0x8c09, 0x8ad9, 0x135d
  .hword 0x8889, 0x10ed, 0x2193, 0x0a69, 0x0843, 0x0625, 0x0411, 0x0205
  .hword 0x0200, 0x3f81, 0x0fc1, 0x3e89, 0xf83f, 0xf661, 0x7a45, 0x795d
  .hword 0xf0f1, 0xef2f, 0xdae7, 0x75df, 0xea0f, 0x7433, 0xe6c3, 0x7293
  .hword 0xe38f, 0xe1fd, 0xe071, 0x6f75, 0x1bad, 0x36fb, 0xb4e9, 0x6c81
  .hword 0xd795, 0x358b, 0x1a99, 0xa6d1, 0xa41b, 0xd0b7, 0xcf65, 0xce17
  .hword 0xcccd, 0x970f, 0x6523, 0x1921, 0x8f9d, 0x8d31, 0x62b3, 0x886f
  .hword 0x8619, 0xc1e5, 0xc0c1, 0x7f41, 0xbe83, 0x7ad3, 0xbc53, 0xbb3f
  .hword 0xba2f, 0x7243, 0x702f, 0x0b71, 0x2d83, 0x5a85, 0x6817, 0x0b31
  .hword 0x642d, 0x0589, 0x6059, 0xaf3b, 0x5c99, 0x5ac1, 0xac77, 0x0ab9
  .hword 0xaaab, 0x5391, 0x51d1, 0xa80b, 0x4e5f, 0x532b, 0xa57f, 0x5255
  .hword 0x47af, 0xa307, 0x1447, 0x42d7, 0xa0a1, 0x3fb1, 0x4f89, 0x9e4d
  .hword 0x4ec5, 0x9cc9, 0x4e05, 0x9b4d, 0x9a91, 0x33af, 0x323f, 0x9869
  .hword 0x2f69, 0x2e03, 0x0965, 0x2b41, 0x29e5, 0x4a23, 0x939b, 0x25e3
  .hword 0x2493, 0x91a3, 0x487f, 0x20b5, 0x1f71, 0x11e3, 0x1cf1, 0x1bb5
  .hword 0x469f, 0x8ca3, 0x8c09, 0x16e1, 0x8ad9, 0x8a43, 0x135d, 0x891b
  .hword 0x8889, 0x10ff, 0x10ed, 0x0db3, 0x2193, 0x0b7f, 0x0a69, 0x4255
  .hword 0x0843, 0x0733, 0x0625, 0x828d, 0x0411, 0x8185, 0x0205, 0x8081

divisionHelper:       ; 0d00 - 0f00
  .word gs(0x0000), gs(divAS0), gs(divNS0), gs(divNS1), gs(divNS0), gs(divNS2), gs(divNS2), gs(divAS3)
  .word gs(divNS0), gs(divNS3), gs(divNS3), gs(divNS3), gs(divNS3), gs(divNS2), gs(divAS4), gs(divNS3)
  .word gs(divNS0), gs(divNS4), gs(divNS4), gs(divNS4), gs(divNS4), gs(divAS5), gs(divNS4), gs(divAS5)
  .word gs(divNS4), gs(divAS5), gs(divNS3), gs(divAS5), gs(divAS5), gs(divNS3), gs(divNS4), gs(divAS5)
  .word gs(divNS0), gs(divNS5), gs(divNS5), gs(divNS5), gs(divNS5), gs(divNS2), gs(divNS5), gs(divAS6)
  .word gs(divNS5), gs(divAS6), gs(divAS6), gs(divNS5), gs(divNS5), gs(divNS3), gs(divAS6), gs(divAS6)
  .word gs(divNS5), gs(divAS6), gs(divAS6), gs(divNS5), gs(divNS4), gs(divNS5), gs(divAS6), gs(divAS6)
  .word gs(divAS6), gs(divAS6), gs(divNS4), gs(divNS5), gs(divNS5), gs(divNS3), gs(divAS6), gs(divAS6)
  .word gs(divNS0), gs(divNS2), gs(divNS6), gs(divNS5), gs(divNS6), gs(divAS7), gs(divNS6), gs(divNS6)
  .word gs(divNS6), gs(divNS6), gs(divNS3), gs(divAS7), gs(divNS6), gs(divNS3), gs(divAS7), gs(divNS6)
  .word gs(divNS6), gs(divNS5), gs(divAS7), gs(divNS5), gs(divAS7), gs(divNS6), gs(divNS6), gs(divNS6)
  .word gs(divNS6), gs(divAS7), gs(divNS4), gs(divAS7), gs(divAS7), gs(divAS7), gs(divAS7), gs(divNS6)
  .word gs(divNS6), gs(divAS7), gs(divAS7), gs(divNS6), gs(divAS7), gs(divNS3), gs(divNS6), gs(divNS5)
  .word gs(divNS5), gs(divNS5), gs(divNS6), gs(divAS7), gs(divAS7), gs(divNS2), gs(divAS7), gs(divNS6)
  .word gs(divAS7), gs(divNS5), gs(divAS7), gs(divAS7), gs(divNS5), gs(divNS6), gs(divNS6), gs(divAS7)
  .word gs(divNS6), gs(divNS3), gs(divNS4), gs(divAS7), gs(divAS7), gs(divAS7), gs(divAS7), gs(divAS7)
  .word gs(divNS0), gs(divNS5), gs(divNS3), gs(divNS5), gs(divNS7), gs(divNS7), gs(divNS6), gs(divNS6)
  .word gs(divNS7), gs(divNS7), gs(divAS8), gs(divNS6), gs(divNS7), gs(divNS6), gs(divNS7), gs(divNS6)
  .word gs(divNS7), gs(divNS7), gs(divNS7), gs(divNS6), gs(divNS4), gs(divNS5), gs(divAS8), gs(divNS6)
  .word gs(divNS7), gs(divNS5), gs(divNS4), gs(divAS8), gs(divAS8), gs(divNS7), gs(divNS7), gs(divNS7)
  .word gs(divNS7), gs(divAS8), gs(divNS6), gs(divNS4), gs(divAS8), gs(divAS8), gs(divNS6), gs(divAS8)
  .word gs(divAS8), gs(divNS7), gs(divNS7), gs(divAS8), gs(divNS7), gs(divAS8), gs(divNS7), gs(divNS7)
  .word gs(divNS7), gs(divAS8), gs(divAS8), gs(divNS3), gs(divNS5), gs(divNS6), gs(divAS8), gs(divNS3)
  .word gs(divAS8), gs(divNS2), gs(divAS8), gs(divNS7), gs(divAS8), gs(divAS8), gs(divNS7), gs(divNS3)
  .word gs(divNS7), gs(divAS8), gs(divAS8), gs(divNS7), gs(divAS8), gs(divNS6), gs(divNS7), gs(divNS6)
  .word gs(divAS8), gs(divNS7), gs(divNS4), gs(divAS8), gs(divNS7), gs(divAS8), gs(divNS6), gs(divNS7)
  .word gs(divNS6), gs(divNS7), gs(divNS6), gs(divNS7), gs(divNS7), gs(divAS8), gs(divAS8), gs(divNS7)
  .word gs(divAS8), gs(divAS8), gs(divNS3), gs(divAS8), gs(divAS8), gs(divNS6), gs(divNS7), gs(divAS8)
  .word gs(divAS8), gs(divNS7), gs(divNS6), gs(divAS8), gs(divAS8), gs(divNS4), gs(divAS8), gs(divAS8)
  .word gs(divNS6), gs(divNS7), gs(divNS7), gs(divAS8), gs(divNS7), gs(divNS7), gs(divAS8), gs(divNS7)
  .word gs(divNS7), gs(divNS4), gs(divNS4), gs(divAS8), gs(divNS5), gs(divAS8), gs(divAS8), gs(divNS6)
  .word gs(divAS8), gs(divAS8), gs(divAS8), gs(divNS7), gs(divAS8), gs(divNS7), gs(divAS8), gs(divNS7)

; table of frequencies times 0x8000000/16000000Hz for default (pattern) mode.
defaultFrequencies:   ; 0f00 - 0f20
  .hword 0x39ab, 0x300f, 0x2b40, 0x2672, 0x2070, 0x1cd5, 0x1807, 0x15a0
  .hword 0x1339, 0x1038, 0x0e6a, 0x0c03, 0x0ad0, 0x099c, 0x081c, 0x0735

; table of words to send to the row-addressing shift registers. Rows are strobed one at a time in ascending order.
rowRegisterData:      ; 0f20 - 0f40
  .byte 0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08
  .byte 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80
  .byte 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00
  .byte 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80, 0x00

.global escapeMenu
escapeMenu:           ; 0f40 - 0f60
  .byte 0x72, 0x12  ; .*..***..*..*... sine
  .byte 0x77, 0x37  ; ***.***.***.**..  square
  .byte 0x72, 0x77  ; .*..***.***.***.   triangle
  .byte 0x00, 0x00  ; ................    sawtooth
  .byte 0x45, 0x20  ; *.*...*......*.. noise
  .byte 0x22, 0x57  ; .*...*..***.*.*.  pattern
  .byte 0x15, 0x20  ; *.*.*........*..   coarse
  .byte 0x00, 0x00  ; ................    fine
  .byte 0x75, 0x27  ; *.*.***.***..*.. tuning
  .byte 0x57, 0x22  ; ***.*.*..*...*..  miscellaneous
  .byte 0x72, 0x72  ; .*..***..*..***.   save
  .byte 0x00, 0x00  ; ................    load
  .byte 0x72, 0x05  ; .*..***.*.*..... life
  .byte 0x11, 0x07  ; *...*,..***.....  random
  .byte 0x17, 0x04  ; ***.*.....*.....   microtone
  .byte 0x00, 0x00  ; ................

.global miscellaneousMenu
miscellaneousMenu:    ; 0f60 - 0f80
  .byte 0x3f, 0xe0  ; ******.......*** tempo override
  .byte 0x21, 0xa0  ; *....*.......*.*  patterns per loop for EEPROM cycling mode
  .byte 0x21, 0xa0  ; *....*.......*.*
  .byte 0x21, 0xa0  ; *....*.......*.*
  .byte 0x21, 0xa0  ; *....*.......*.*
  .byte 0x3f, 0xa0  ; ******.......*.*
  .byte 0x00, 0xe0  ; .............***
  .byte 0x3f, 0x00  ; ******.......... envelope decay constant
  .byte 0x21, 0xe0  ; *....*.......***  fixed tuning mode
  .byte 0x21, 0xa0  ; *....*.......*.*
  .byte 0x3f, 0xe0  ; ******.......***
  .byte 0x00, 0x00  ; ................
  .byte 0x0f, 0xe0  ; ****.........*** beats per pattern
  .byte 0x09, 0xa0  ; *..*.........*.*  continuously update noise waveform
  .byte 0x09, 0xe0  ; *..*.........***
  .byte 0x0f, 0x00  ; ****............

.global microtoneScreen
microtoneScreen:      ; 0f80 - 0fa0
  .byte 0x41, 0x82  ; *.....*..*.....* A0    B0 C1    D1
  .byte 0x90, 0x20  ; ....*..*.....*..     E1 F1    G1
  .byte 0x04, 0x09  ; ..*.....*..*....   A1    B1 C2
  .byte 0x42, 0x82  ; .*....*..*.....*  D2   E2 F2    G2
  .byte 0x10, 0x24  ; ....*.....*..*..     A2    B2 C3
  .byte 0x08, 0x09  ; ...*....*..*....    D3   E3 F3
  .byte 0x42, 0x90  ; .*....*.....*..*  G3   A3    B3 C4
  .byte 0x20, 0x24  ; .....*....*..*..      D4   E4 F4
  .byte 0x08, 0x41  ; ...*....*.....*.    G4   A4    B4
  .byte 0x82, 0x90  ; .*.....*....*..*  C5    D5   E5 F5
  .byte 0x20, 0x04  ; .....*....*.....      G5   A5
  .byte 0x09, 0x42  ; *..*.....*....*. B5 C6    D6   E6
  .byte 0x82, 0x10  ; .*.....*....*...  F6    G6   A6
  .byte 0x24, 0x08  ; ..*..*.....*....   B6 C7    D7
  .byte 0x09, 0x42  ; *..*.....*....*. E7 F7    G7   A7
  .byte 0x90, 0x20  ; ....*..*.....*..     B7 C8    D8

.global speakerPositions
speakerPositions:     ; 0fa0 - 0fb0
  .byte 0x80, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7
  .byte 0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f

.global redLED
redLED:               ; 0fb0 - 0fc0
  .byte 0x2d, 0x65, 0xab, 0xe7, 0xff, 0xe7, 0xab, 0x65
  .byte 0x2d, 0x0d, 0x02, 0x00, 0x00, 0x00, 0x02, 0x0d, 0x0d

.global greenLED
greenLED:             ; 0fc0 - 0fd0
  .byte 0xd6, 0x93, 0x50, 0x1f, 0x07, 0x00, 0x00, 0x00
  .byte 0x00, 0x04, 0x15, 0x3d, 0x7c, 0xc2, 0xf4, 0xfc, 0xfc

.global blueLED
blueLED:              ; 0fd0 - 0ff0
  .hword 0x0001, 0x0000, 0x0000, 0x0003, 0x001f, 0x007f, 0x0141, 0x0251
  .hword 0x035c, 0x03f4, 0x03d3, 0x030a, 0x01f2, 0x00f5, 0x0055, 0x0011, 0x0011

columnTable:          ; 0ff0 - 1000
  .word gs(column0), gs(column1), gs(column2), gs(column3), gs(column4), gs(column5), gs(column6), gs(column7)

