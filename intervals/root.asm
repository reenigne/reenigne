  list p=12f508
#include "P12F508.INC"

#define length 0

bit0 EQU 4
bit1 EQU 5
bit2 EQU 0
bit3 EQU 1

highAll EQU 3fh

low0 EQU 2fh
low1 EQU 1fh
low2 EQU 3eh
low3 EQU 3dh

lengthLow                  EQU 07h
lengthMiddle               EQU 08h
lengthHigh                 EQU 09h
parentAxis                 EQU 0ah
switch                     EQU 0bh
childBpresent              EQU 0ch
childCpresent              EQU 0dh
childDpresent              EQU 0eh
count                      EQU 0fh
afterInitData              EQU 10h
after0                     EQU 11h
after1                     EQU 12h
after2                     EQU 13h
after3                     EQU 14h
afterReceiveUSync          EQU 15h
afterWrite                 EQU 16h
afterSendUSync             EQU 17h
bits                       EQU 18h
lowParent                  EQU 19h
lowChild                   EQU 1ah
more                       EQU 1bh


unroll macro m
  m 0
  m 1
  m 2
  m 3
  endm


delay1 macro
  NOP
  endm

delay2 macro
  local l
  GOTO l
l
  endm

delay3 macro
  delay2
  delay1
  endm


readBit macro i, b       ; 4     read needs to be started 5 cycles after write
  MOVLW 0                    ; 1   -1
  BTFSC GPIO, bit#v(b)       ; 1    0
  MOVLW 1                    ; 1
  TRIS 5                     ; 1
  endm

writeBit macro i
  NOP
  NOP
  NOP
  NOP
  endm


lowPageCode macro b
  local lU, lV

prime#v(b)
  GOTO prime#v(b)b

read#v(b)
  readBit 0, b         ; 4
  readBit 1, b         ; 4
  readBit 2, b         ; 4
  readBit 3, b         ; 4
  readBit 4, b         ; 4
  readBit 5, b         ; 4
  readBit 6, b         ; 4
  readBit 7, b         ; 4
  MOVF after#v(b), W               ;  1
  BTFSC GPIO, bit#v(b)             ;  1  2
  MOVWF PCL                        ;  1  0
do#v(b)
  BSF bits, bit#v(b)               ;     1
  MOVLW read#v(b)                  ;     1
  MOVWF afterReceiveUSync          ;     1
  MOVLW low#v(b)                   ;     1
  GOTO sendUSync                   ;     2

receiveVSync#v(b)         ; 5-7     receiveVSync needs to be started 4 cycles after sendVSync (+/- 1), 36 cycles after write
  BTFSC GPIO, bit#v(b)      ; 0-2  0-2  2-3  (1)
  GOTO lU                   ; 3-5  3-5
lU
  BTFSC GPIO, bit#v(b)      ; 3-4  4-5  4-5
  GOTO write                ;      5-6  5-6
  GOTO write                ; 5-6

receiveUSync#v(b)
  MOVF afterReceiveUSync, W
  BTFSC GPIO, bit#v(b)      ; 0-2  0-2  2-3  (1)
  GOTO lV                   ; 3-5  3-5
lV
  BTFSC GPIO, bit#v(b)      ; 3-4  4-5  4-5
  MOVWF PCL                 ;      5-6  5-6
  MOVWF PCL                 ; 5-6

  endm


highPageCode macro b
  local waitForPrimeComplete, waitForDataRequest

found#v(b)
  BCF bits, bit#v(b)
  CALL prime#v((b+1)&3)
  CALL prime#v((b+2)&3)
  CALL prime#v((b+3)&3)
  BSF bits, bit#v(b)
  MOVF bits, W
  TRIS GPIO

  MOVLW doFinalWrite
  MOVWF after#v((b+3)&3)
  BTFSC childDpresent, 0
  MOVLW do#v((b+3)&3)
  MOVWF after#v((b+2)&3)
  BTFSC childCpresent, 0
  MOVLW do#v((b+2)&3)
  MOVWF after#v((b+1)&3)
  BTFSC childBpresent, 0
  MOVLW do#v((b+1)&3)
  MOVWF afterInitData

  MOVLW initData#v(b&1)
  MOVWF afterReceiveUSync

  MOVLW low#v(b)
  MOVWF lowParent
  MOVWF more

  MOVLW receiveVSync#v(b)
  MOVWF afterSendUSync
  MOVLW receiveUSync#v(b)
  MOVWF afterWrite

  GOTO dataRequested#v(b)
  NOP
dataRequested#v(b)
  MOVWF PCL

prime#v(b)b
  MOVF bits, W
  ANDLW low#v(b)
  CALL trisAndDelay21
  MOVF bits, W
  TRIS GPIO
  INCF FSR, F
  BTFSC GPIO, bit#v(b)
  RETLW 0
  INCF INDF, F
  BCF bits, bit#v(b)
waitForPrimeComplete
  BTFSS GPIO, bit#v(b)
  GOTO waitForPrimeComplete
  RETLW 0

  endm


  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC


; --- low page ---

  GOTO startup

  unroll lowPageCode

reset
  GOTO resetb

initData0
  MOVLW 0  ; parent axis
  GOTO initData

initData1
  MOVLW 1  ; parent axis
  GOTO initData

trisAndDelay21
  TRIS GPIO
delay21
  NOP
delay20
  GOTO delay18
delay18
  GOTO delay16
delay16
  GOTO delay14
delay14
  GOTO delay12
delay12
  NOP
delay11
  NOP
delay10
  NOP
delay9
  NOP
delay8
  GOTO delay6
delay6
  GOTO delay4
delay4
  RETLW 0


doFinalWrite                     ; 18
  CALL delay11                   ; 11
  MOVLW reset                    ; 1
  MOVWF afterWrite               ; 1
  MOVLW highAll                  ; 1
  MOVWF more                     ; 1
  MOVWF lowChild                 ; 1
  MOVF afterSendUSync, W         ; 1
  MOVWF PCL                      ; 1

write                            ; 43     Write needs to be called 5 cycles before read
  writeBit 0                     ;  4
  writeBit 1                     ;  4
  writeBit 2                     ;  4
  writeBit 3                     ;  4
  writeBit 4                     ;  4
  writeBit 5                     ;  4
  writeBit 6                     ;  4
  writeBit 7                     ;  4
; sendVSync
  delay1                         ;  1
  MOVF bits, W                   ;  1
  ANDWF more, W                  ;  1
  TRIS GPIO                      ;  1             ; more/sync init
  CALL delay9                    ;  9
  MOVF bits, W
  ANDWF lowChild, W              ;  1
  TRIS GPIO                      ;  1             ; sync first transition
  MOVF bits, W                   ;  1
  TRIS GPIO                      ;  1             ; sync second transition
  MOVF afterWrite, W             ;  1
  MOVWF PCL                      ;  1

; --- high page (actually starts somewhere after write) ---

sendUSync
  MOVWF lowChild                 ; 1
  MOVF bits, W                   ; 1
  TRIS GPIO                      ; 1              data request and sync init
  delay3                         ; 3
  ANDWF lowChild, W              ; 1
  TRIS GPIO                      ; 1              first transition
  MOVF bits, W                   ; 1
  TRIS GPIO                      ; 1              second transition
  MOVF afterSendUSync, W         ; 1
  MOVWF PCL                      ; 1

startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF GPIO
  MOVLW highAll
  MOVWF bits

resetb
  CLRF childBpresent     ; 1
  CLRF childCpresent     ; 1
  CLRF childDpresent     ; 1
  MOVLW childBpresent-1  ; 1
  MOVWF FSR              ; 1
; Time from input to output: 5-15 cycles
  TRIS 7
  NOP
  delay2
  delay2
  GOTO found0
  NOP
  NOP
  NOP
found0

  unroll highPageCode

initData                     ; 32
  MOVWF parentAxis           ; 1
  MOVLW 1                    ; 1
  if (length & 1)
    MOVWF lengthLow          ; 1
  else
    CLRF lengthLow
  endif
  if ((length >> 1) & 1)
    MOVWF lengthMiddle       ; 1
  else
    CLRF lengthMiddle
  endif
  if ((length >> 2) & 1)
    MOVWF lengthHigh         ; 1
  else
    CLRF lengthHigh
  endif
  CLRF switch                ; 1
  BTFSC GPIO, 2              ; 1
  INCF switch, F             ; 1

  MOVLW 7                    ; 1
  MOVWF count                ; 1
initDelayLoop
  DECFSZ count, F            ; 1*6 + 2
  GOTO initDelayLoop         ; 2*6
  MOVF afterInitData, W      ; 1
  MOVWF PCL                  ; 1

  end


; TODO:
;   Does the sendUSync delay (and hence the doFinalWrite delay) need to be longer?

; 20 instructions free, 23 in low page
