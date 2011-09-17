; Timer 1 interrupt vector.

;.global __vector_13
;__vector_13:  ; TIMER1_OVF_vect
;  push r0
;  in r0, 0x3f
;  push r0
;
;  inc r4
;
;  pop r0
;  out 0x3f, r0
;  pop r0
;  reti
;
;.global __vector_10                      ; 78
;__vector_10:  ; TIMER1_CAPT_vect         ; 7
;  push r31                               ; 2
;  push r30                               ; 2
;  push r27                               ; 2
;  push r26                               ; 2
;  push r25                               ; 2
;  push r24                               ; 2
;  push r23                               ; 2
;  push r22                               ; 2
;  push r21                               ; 2
;  push r20                               ; 2
;  push r19                               ; 2
;  push r18                               ; 2
;  push r1                                ; 2
;  push r0                                ; 2
;  in r0, 0x3f                            ; 1
;  push r0                                ; 2
;
;  lds r5, 0x86                           ; 1
;  lds r6, 0x87                           ; 1
;  rcall sendTimerData                    ; 3
;
;  pop r0                                 ; 2
;  out 0x3f, r0                           ; 1
;  pop r0                                 ; 2
;  pop r1                                 ; 2
;  pop r18                                ; 2
;  pop r19                                ; 2
;  pop r20                                ; 2
;  pop r21                                ; 2
;  pop r22                                ; 2
;  pop r23                                ; 2
;  pop r24                                ; 2
;  pop r25                                ; 2
;  pop r26                                ; 2
;  pop r27                                ; 2
;  pop r30                                ; 2
;  pop r31                                ; 2
;  reti                                   ; 4

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

;.global startTimer
;startTimer:
;  ldi r31, 0x21
;  sts 0x6f, r31
;  ret
;
;.global stopTimer
;stopTimer:
;  ldi r31, 0x00
;  sts 0x6f, r31
;  ret
;
