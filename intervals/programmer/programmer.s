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

.global __vector_10                      ; 78
__vector_10:  ; TIMER1_CAPT_vect         ; 7
  push r31                               ; 2
  push r30                               ; 2
  push r27                               ; 2
  push r26                               ; 2
  push r25                               ; 2
  push r24                               ; 2
  push r23                               ; 2
  push r22                               ; 2
  push r21                               ; 2
  push r20                               ; 2
  push r19                               ; 2
  push r18                               ; 2
  push r1                                ; 2
  push r0                                ; 2
  in r0, 0x3f                            ; 1
  push r0                                ; 2

  lds r5, 0x86                           ; 1
  lds r6, 0x87                           ; 1
  call sendTimerData                     ; 3

  pop r0                                 ; 2
  out 0x3f, r0                           ; 1
  pop r0                                 ; 2
  pop r1                                 ; 2
  pop r18                                ; 2
  pop r19                                ; 2
  pop r20                                ; 2
  pop r21                                ; 2
  pop r22                                ; 2
  pop r23                                ; 2
  pop r24                                ; 2
  pop r25                                ; 2
  pop r26                                ; 2
  pop r27                                ; 2
  pop r30                                ; 2
  pop r31                                ; 2
  reti                                   ; 4


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
  eor r24, r24
  sbic 0x03, 0
  inc r24
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
wait100ns:  ; 2 cycles        ; 4
  ret                         ; 4

.global wait1us
wait1us:    ; 16 cycles           ; 4
;  ldi r31, 5         ; 1
;wait1usLoop:
;  dec r31            ; 5*1
;  brne wait1usLoop   ; 4*2 + 1
  call wait100ns                  ; 8
  ret                ; 2          ; 4

.global wait5us
wait5us:    ; 80 cycles           ;  4
  ldi r30, 3                      ;  1
wait5usLoop:
  call wait1us                    ; 48 (3*16)
  dec r30                         ;  3 (3*1)
  brne wait5usLoop                ;  5 (2*2 + 1)
  rcall wait100ns                 ;  7
  call wait100ns                  ;  8
  ret                             ;  4

.global wait100us
wait100us:  ; 1600 cycles         ;    4
  ldi r27, 19                     ;    1
wait100usLoop:
  call wait5us                    ; 1520 (19*80)
  dec r27                         ;   19 (19*1)
  brne wait100usLoop              ;   37 (18*2 + 1)
  rcall wait100ns                 ;    7
  call wait100ns                  ;    8
  ret                             ;    4

.global wait2ms
wait2ms:    ; 32000 cycles        ;     4
  ldi r26, 19                     ;     1
wait2msLoop:
  call wait100us                  ; 30400 (19*1600)
  call wait5us                    ;  1520 (19*80)
  dec r26                         ;    10 (19*1)
  brne wait2msLoop                ;    37 (18*2 + 1)
  call wait100ns                  ;     8
  call wait100ns                  ;     8
  call wait100ns                  ;     8
  ret                             ;     4

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

