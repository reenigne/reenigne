; Timer 1 interrupt vector.

.global __vector_13
__vector_13:  ; TIMER1_OVF_vect
  push r0
  in r0, 0x3f
  push r0

  inc r4

  pop r0
  out 0x3f, r0
  pop r0
  reti

.global __vector_10
__vector_10:  ; TIMER1_CAPT_vect
  push r31
  push r30
  push r27
  push r26
  push r25
  push r24
  push r23
  push r22
  push r21
  push r20
  push r19
  push r18
  push r1
  push r0
  in r0, 0x3f
  push r0

  lds r5, 0x86
  lds r6, 0x87
  rcall sendTimerData

  pop r0
  out 0x3f, r0
  pop r0
  pop r1
  pop r18
  pop r19
  pop r20
  pop r21
  pop r22
  pop r23
  pop r24
  pop r25
  pop r26
  pop r27
  pop r30
  pop r31
  reti


.global raiseVDD
raiseVDD:
  sbi 0x05, 3
  ret

.global lowerVDD
lowerVDD:
  cbi 0x05, 3
  ret

.global raiseVPP
raiseVPP:
  cbi 0x05, 2
  ret

.global lowerVPP
lowerVPP:
  sbi 0x05, 2
  ret

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

.global getData
getData:
  eor r12, r12
  sbic 0x03, 0
  inc r12
  ret

.global setDataInput
setDataInput:
  cbi 0x04, 0
  ret

.global setDataOutput
setDataOutput:
  sbi 0x04, 0
  ret

.global wait100ns
wait100ns:  ; 2 cycles
  ret

.global wait1us
wait1us:    ; 16 cycles
  ldi r31, 5         ; 1
wait1usLoop:
  dec r31            ; 5*1
  brne wait1usLoop   ; 4*2 + 1
  ret                ; 2

.global wait5us
wait5us:    ; 80 cycles
  ldi r30, 5
wait5usLoop:
  rcall wait1us
  dec r30
  brne wait5usLoop
  ret

.global wait100us
wait100us:  ; 1600 cycles
  ldi r27, 20
wait100usLoop:
  rcall wait5us
  dec r27
  brne wait100usLoop
  ret

.global wait2ms
wait2ms:    ; 32000 cycles
  ldi r26, 20
wait2msLoop:
  rcall wait100us
  dec r26
  brne wait2msLoop
  ret

.global wait10ms
wait10ms:   ; 160000 cycles
  ldi r25, 5
wait10msLoop:
  rcall wait2ms
  dec r25
  brne wait10msLoop
  ret

.global startTimer
startTimer:
  ldi r31, 0x21
  sts 0x6f, r31
  ret

.global stopTimer
stopTimer:
  ldi r31, 0x00
  sts 0x6f, r31
  ret

