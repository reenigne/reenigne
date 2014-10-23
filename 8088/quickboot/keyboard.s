;.global __vector_4
;__vector_4:  ; PCINT1_vect - reset button pressed
;  push r31
;  push r30
;  push r27
;  push r0
;  in r0, 0x3f
;  push r0
;
;  sbis 0x06, 1  ; Port C
;  call reset
;
;  pop r0
;  out 0x3f, r0
;  pop r0
;  pop r27
;  pop r30
;  pop r31
;  reti
;
;.global __vector_5
;__vector_5:  ; PCINT2_vect - keyboard clock input
;  push r31
;  push r30
;  push r27
;  push r0
;  in r0, 0x3f
;  push r0
;
;  sbis 0x09, 3  ; Port D
;  rjmp fallingEdge  ; We only care about the rising edge
;
;
;
;fallingEdge:
;  pop r0
;  out 0x3f, r0
;  pop r0
;  pop r27
;  pop r30
;  pop r31
;  reti


.global getInputClock
getInputClock:
  eor r24, r24
  sbic 0x09, 3  ; Port D
  inc r24
  ret

.global getInputData
getInputData:
  eor r24, r24
  sbic 0x09, 2  ; Port D
  inc r24
  ret

.global getResetButton
getResetButton:
  eor r24, r24
  sbic 0x06, 1  ; Port C
  inc r24
  ret

.global getResetLine
getResetLine:
  eor r24, r24
  sbic 0x03, 3  ; Port B
  inc r24
  ret

; When going high, switch to input first to avoid output going high with
; low-impedence (even momentarily).
; Similarly, when going low switch to low/pull-up-disabled first.

.global raiseInputClock
raiseInputClock:
  cbi 0x0a, 3  ; Port D - input
  sbi 0x0b, 3  ; Port D - high/pull-up enabled
  ret

.global lowerInputClock
lowerInputClock:
  cbi 0x0b, 3  ; Port D - low/pull-up disabled
  sbi 0x0a, 3  ; Port D - output
  ret

.global raiseInputData
raiseInputData:
  cbi 0x0a, 2  ; Port D - input
  sbi 0x0b, 2  ; Port D - high/pull-up enabled
  ret

.global lowerInputData
lowerInputData:
  cbi 0x0b, 2  ; Port D - low/pull-up disabled
  sbi 0x0a, 2  ; Port D - output
  ret

.global raiseClock
raiseClock:
  cbi 0x04, 1  ; Port B - input
;  sbi 0x05, 1  ; Port B - high/pull-up enabled - external pull-up in XT
  ret

.global lowerClock
lowerClock:
;  cbi 0x05, 1  ; Port B - low/pull-up disabled - external pull-up in XT
  sbi 0x04, 1  ; Port B - output
  ret

.global raiseData
raiseData:
  cbi 0x04, 0  ; Port B - input
;  sbi 0x05, 0  ; Port B - high/pull-up enabled - external pull-up in XT
  ret

.global lowerData
lowerData:
;  cbi 0x05, 0  ; Port B - low/pull-up disabled - external pull-up in XT
  sbi 0x04, 0  ; Port B - output
  ret

.global getClock
getClock:
  eor r24, r24
  sbic 0x03, 1  ; Port B
  inc r24
  ret

.global getData
getData:
  eor r24, r24
  sbic 0x03, 0  ; Port B
  inc r24
  ret

.global getQuickBoot
getQuickBoot:
  eor r24, r24
  sbic 0x06, 2  ; Port C
  inc r24
  ret

.global reset
reset:
  sbi 0x07, 0  ; Port C
  call wait250ms
  cbi 0x07, 0  ; Port C
  ret

.global waitCycles
waitCycles:
;     (0x84) TCNT1L                                                                   139                   Timer/Counter 1 Register Low
;     (0x85) TCNT1H                                                                   139                   Timer/Counter 1 Register High
  lds r26, 0x84
  lds r27, 0x85
waitCyclesLoop:
  lds r30, 0x84
  lds r31, 0x85

  sub r30, r26
  sbc r31, r27   ; r31:r30 is now cycles elapsed

  cp r30, r24
  cpc r31, r25   ; r25:r24 is input value

  ; If the carry flag is set now, r31:r30 is smaller than r25:r24
  brcs waitCyclesLoop
  ret

.global waitCycles32
waitCycles32:
  nop
.global waitCycles31
waitCycles31:
  nop
.global waitCycles30
waitCycles30:
  dec r24             ; n*1
  brne waitCycles30   ; n*2 - 1
  ret                 ; 4

.global wait2us       ; 32 cycles
wait2us:              ; 4
  ldi r31,8           ; 1          ; (cycles to delay - 8)/3
wait2usLoop:
  dec r31             ; n*1
  brne wait2usLoop    ; n*2 - 1
  ret                 ; 4

wait151cycles:        ; 4
  ldi r31,47          ; 1          ; (cycles to delay - 10)/3
wait151cyclesLoop:
  dec r31             ; n*1
  brne wait151cyclesLoop  ; n*2 - 1
  rjmp wait151cyclesNop   ; 2
wait151cyclesNop:
  ret                 ; 4

.global wait50us      ; 800 cycles
wait50us:             ; 4
;  ldi r31,198         ; 1          ; (cycles to delay - 8)/4
;wait50usLoop:
;  nop                 ; n*1
;  dec r31             ; n*1
;  brne wait50usLoop   ; n*2 - 1
;  ret                 ; 4
  ldi r31,104         ; 1          ; (cycles to delay - 8)/3           - this is actually now 20us!
wait50usLoop:
  dec r31             ; n*1
  brne wait50usLoop    ; n*2 - 1
  ret                 ; 4

.global wait1ms       ; 16000 cycles
wait1ms:              ; 4
  ldi r30,19          ; 1          ; (cycles to delay - 8)/803
wait1msLoop1:
  call wait50us       ; n*800
  dec r30             ; n*1
  brne wait1msLoop1   ; n*2 - 1
  ldi r30,245         ; 1          ; (cycles to delay)/3
wait1msLoop2:
  dec r30             ; n*1
  brne wait1msLoop2   ; n*2 - 1
  ret                 ; 4

.global wait250ms     ; 4000000 cycles
wait250ms:            ; 4
  ldi r27,250         ; 1          ; (cycles to delay - 8)/16003
wait250msLoop:
  call wait1ms        ; n*16000
  dec r27             ; n*1
  brne wait250msLoop  ; n*2 - 1
  ret                 ; 4

; The data line isn't controllable from software (except for setting it high
; after it went low due to a byte being received). So we have to do one-line
; communications, which means we need to do precise timing. So we turn off
; interrupts.
.global receiveKeyboardByte
receiveKeyboardByte:
  eor r24, r24
  cli
  ; Wait for clock line to go low
clockWaitLoop1:
  sbic 0x03, 1                      ; 2 1  ; Port B
  rjmp clockWaitLoop1               ; 0 2
  ; Wait for clock line to go high
clockWaitLoop2:
  sbic 0x03, 1                      ; 2 1  ; Port B
  rjmp clockWaitLoop2               ; 0 2

  ldi r31,23                        ; 1          ; (cycles to delay)/3
wait69cyclesLoop:
  dec r31                           ; n*1
  brne wait69cyclesLoop             ; n*2 - 1

  call wait151cycles
  ; Read bit 0
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 1                        ; 0 1
  call wait151cycles
  ; Read bit 1
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 2                        ; 0 1
  call wait151cycles
  ; Read bit 2
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 4                        ; 0 1
  call wait151cycles
  ; Read bit 3
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 8                        ; 0 1
  call wait151cycles
  ; Read bit 4
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 0x10                     ; 0 1
  call wait151cycles
  ; Read bit 5
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 0x20                     ; 0 1
  call wait151cycles
  ; Read bit 6
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 0x40                     ; 0 1
  call wait151cycles
  ; Read bit 7
  sbic 0x03, 1                      ; 2 1  ; Port B
  ori r24, 0x80                     ; 0 1
  sei
  ret

.section .progmem.data,"a",@progbits

.align 8

; Table for converting ASCII characters to scancodes.
; Low 7 bits are the scancode, high bit is set for shift
.global asciiToScancodes
asciiToScancodes:
  .byte 0x39, 0x82, 0xa8, 0x84, 0x85, 0x86, 0x88, 0x28   ;  !"#$%&'
  .byte 0x8a, 0x8b, 0x89, 0x8d, 0x33, 0x0c, 0x34, 0x35   ; ()*+,-./
  .byte 0x0b, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08   ; 01234567
  .byte 0x09, 0x0a, 0xa7, 0x27, 0xb3, 0x0d, 0xb4, 0xb5   ; 89:;<=>?
  .byte 0x83, 0x9e, 0xb0, 0xae, 0xa0, 0x92, 0xa1, 0xa2   ; @ABCDEFG
  .byte 0xa3, 0x97, 0xa4, 0xa5, 0xa6, 0xb2, 0xb1, 0x98   ; HIJKLMNO
  .byte 0x99, 0x90, 0x93, 0x9f, 0x94, 0x96, 0xaf, 0x91   ; PQRSTUVW
  .byte 0xad, 0x95, 0xac, 0x1a, 0x2b, 0x1b, 0x87, 0x8c   ; XYZ[\]^_
  .byte 0x29, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22   ; `abcdefg
  .byte 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18   ; hijklmno
  .byte 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11   ; pqrstuvw
  .byte 0x2d, 0x15, 0x2c, 0x9a, 0xab, 0x9b, 0xa9         ; xyz{|}~

; Table for converting scancodes to remote codes
.global remoteCodes
remoteCodes:
  .word 0x0000, 0x906f, 0x6897, 0x58a7, 0x7887, 0xd827, 0xe817, 0xc837
  .word 0xd02f, 0xe01f, 0xc03f, 0x22dd, 0x2ad5, 0x1ae5, 0x0000, 0x0000
  .word 0x0000, 0x0000, 0x0000, 0x0000, 0xf00f, 0x0000, 0x28d7, 0xa857
  .word 0x40bf, 0x00ff, 0x609f, 0x807f, 0x0000, 0x0000, 0x48b7, 0x10ef
  .word 0x18e7, 0x08f7, 0x38c7, 0x0000, 0x0000, 0x0000, 0x20df, 0x0000
  .word 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
  .word 0x0000, 0x0000, 0x708f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
  .word 0x0000, 0x0000, 0x0000, 0x30cf, 0xb847, 0x12ed, 0xf807, 0x0af5
  .word 0x02fd, 0x3ac5, 0x8877, 0x32cd, 0x9867, 0x0000, 0x0000, 0x0000
  .word 0xa05f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
  .word 0x50af

; The default tester program. First two bytes are length, remaining bytes are
; the program code which is to be loaded at 0000:0500.
.global defaultProgram
defaultProgram:


