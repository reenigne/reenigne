.global raiseClock
raiseClock:
  sbi 0x05, 1
  ret

.global lowerClock
lowerClock:
  cbi 0x05, 1
  ret

.global raiseData
raiseData:
  sbi 0x05, 0
  ret

.global lowerData
lowerData:
  cbi 0x05, 0
  ret

.global getClock
getData:
  eor r24, r24
  sbic 0x03, 1
  inc r24
  ret

.global getData
getData:
  eor r24, r24
  sbic 0x03, 0
  inc r24
  ret

.global setClockInput
setClockInput:
  cbi 0x04, 1
  ret

.global setClockOutput
setClockOutput:
  sbi 0x04, 1
  ret

.global setDataInput
setDataInput:
  cbi 0x04, 0
  ret

.global setDataOutput
setDataOutput:
  sbi 0x04, 0
  ret

.global wait2us       ; 32 cycles
wait2us:              ; 4
  ldi r31,8           ; 1          ; (cycles to delay - 8)/3
wait2usLoop:
  dec r31             ; n*1
  brne wait2usLoop    ; n*2 - 1
  ret                 ; 4

.global wait50us      ; 800 cycles
wait50us:             ; 4
  ldi r31,200         ; 1          ; (cycles to delay - 8)/4
wait50usLoop:
  nop                 ; n*1
  dec r31             ; n*1
  brne wait2usLoop    ; n*2 - 1
  ret                 ; 4

.global wait250ms     ; 4000000 cycles
wait250ms:            ; 4
  ldi r31,250         ; 1          ; (cycles to delay - 8)/16003
wait250msLoop:
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  call wait50us       ; n*800
  dec r31             ; n*1
  brne wait2usLoop    ; n*2 - 1
  ret                 ; 4

.section .progmem.data,"a",@progbits

.align 8

; Table for converting ASCII characters to scancodes.
; Low 7 bits are the scancode, high bit is set for shift (need to send 0x2a if not shifted)
.global asciiToScancodes
asciiToScancodes:
  .byte 0x0e  ; '\b' == 0x08
  .byte 0x0f  ; '\t' == 0x09
  .byte 0x1c  ; '\n' == 0x0a/0x0d
  .byte 0x01  ; 0x1b (escape)
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

