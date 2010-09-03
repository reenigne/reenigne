  list p=12f508
#include "P12F508.INC"

#define length 0

; 4210
IIII EQU 3fh

IIIO EQU 3eh
IIOI EQU 3dh
IOII EQU 3bh
OIII EQU 2fh

IIOO EQU 3ch
IOOI EQU 39h
OOII EQU 2bh
OIIO EQU 2eh

IOIO EQU 3ah
OIOI EQU 2dh

data0         EQU 07h
data1         EQU 08h
data2         EQU 09h
data3         EQU 0ah
data4         EQU 0bh
data5         EQU 0ch
data6         EQU 0dh
data7         EQU 0eh

count         EQU 0fh
afterA        EQU 10h
afterB        EQU 11h
afterC        EQU 12h

AhighBlow     EQU 13h
AlowBhigh     EQU 14h

lengthLow     EQU 07h
lengthMiddle  EQU 08h
lengthHigh    EQU 09h
parentLow     EQU 0ah
switch        EQU 0bh
childBpresent EQU 0ch
childCpresent EQU 0dh
childDpresent EQU 0eh


  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC

  GOTO startup

doB0
  GOTO doB0b
doC0
  GOTO doC0b
doD0
  GOTO doD0b
doB1
  GOTO doB1b
doC1
  gOTO doC1b
doD1
  GOTO doD1b
doB2
  GOTO doB2b
doC2
  GOTO doC2b
doD2
  GOTO doD2b
doB4
  GOTO doB4b
doC4
  GOTO doC4b
doD4
  GOTO doD4b

delay16T
  TRIS GPIO
delay16
  GOTO delay14
delay14
  GOTO delay12
delay12
  GOTO delay10
delay10
  GOTO delay8
delay8
  GOTO delay6
delay6
  GOTO delay4
delay4
  RETLW 0

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

initRW                       ; 8
  MOVLW 4                    ; 1
  MOVWF count                ; 1
  MOVLW data0                ; 1
  MOVWF FSR                  ; 1
  RETLW 0                    ; 2

write                        ; 67 -20
  CALL delay4                ; 4  -18
  CALL initRW                ; 8  -14
writeBitsLoop
  MOVF AlowBhigh, W          ; 1   -6
  BTFSC INDF, 0              ; 1   -5
  MOVLW IIII                 ; 1   -4
  TRIS GPIO                  ; 1   -3
  INCF FSR, F                ; 1   -2
  MOVF AlowBhigh, W          ; 1   -1
  BTFSC INDF, 0              ; 1    0 <- first read here
  MOVLW IIII                 ; 1
  TRIS GPIO                  ; 1
  INCF FSR, F                ; 1
  DECFSZ count, F            ; 1
  GOTO writeBitsLoop         ; 2
  RETLW 0                    ; 2

readBits macro source        ;      readN should be called 9 cycles after write
  local l                    ; 2  -11
  CALL initRW                ; 8   -9
l
  CLRF INDF                  ; 1   -1
  BTFSC GPIO, source         ; 1    0
  INCF INDF, F               ; 1
  INCF FSR, F                ; 1
  NOP                        ; 1
  CLRF INDF                  ; 1
  BTFSC GPIO, source         ; 1
  INCF INDF, F               ; 1
  INCF FSR, F                ; 1
  NOP                        ; 1
  DECFSZ count, F            ; 1
  GOTO l                     ; 2
  delay2                     ; 2
  RETLW 0                    ; 2
  endm

read0
  readBits 0
read1
  readBits 1
read2
  readBits 2
read4
  readBits 4

sendRSync macro destination  ;  2     total 14      needs to take 16?
  MOVLW destination          ;  1   1    -6
  TRIS GPIO                  ;  1   1    -5
  MOVWF AhighBlow            ;  1   0    -4
  delay3                     ;  3   0    -3
  MOVLW IIII                 ;  1   0     0
  TRIS GPIO                  ;  1   0     1
  MOVF AhighBlow, W          ;  1   1     2
  TRIS GPIO                  ;  1   1     3
  RETLW 0                    ;  2   0     4
  endm

sendSync               ; 2    1     total 18
  MOVF AlowBhigh, W
  TRIS GPIO
  CALL delay6          ; 6    1        -8
  MOVF AhighBlow, W    ; 1    1        -2
  TRIS GPIO            ; 1    1        -1
  MOVLW IIII           ; 1    0         0
  TRIS GPIO            ; 1    0         1
  MOVF AhighBlow, W    ; 1    1         2
  TRIS GPIO            ; 1    1         3
  RETLW 0              ; 2    0         4

sendRSync0
  sendRSync IIIO
sendRSync1
  sendRSync IIOI
sendRSync2
  sendRSync IOII
sendRSync4
  sendRSync OIII


receiveSync macro source        ; 9..11 cycles     recieveSyncN should be called 9 cycles (+/-1) after sendSync, 5 cycles (+/-1) after sendRSync
  local l
  MOVLW 0x3f - (1 << source) ; -1
  MOVWF AlowBhigh           ; 0
  BTFSS GPIO, source        ; 0-2  0-2  2-3  (1)
  GOTO l                    ; 3-5  3-5
l
  BTFSS GPIO, source        ; 3-4  4-5  4-5
  RETLW 0                   ;      5-6  5-6
  RETLW 0                   ; 5-6
  endm

receiveSync0
  receiveSync 0
receiveSync1
  receiveSync 1
receiveSync2
  receiveSync 2
receiveSync4
  receiveSync 4

clearPrime0
  CALL delay16T
  MOVLW IIIO
  TRIS GPIO
  RETLW 0
clearPrime1
  CALL delay16T
  MOVLW IIOI
  TRIS GPIO
  RETLW 0
clearPrime2
  CALL delay16T
  MOVLW IOII
  TRIS GPIO
  RETLW 0
clearPrime4
  CALL delay16T
  MOVLW OIII
  TRIS GPIO
  RETLW 0

checkForPrime0
  INCF FSR, F
  BTFSC GPIO, 0
  RETLW 0
  INCF INDF, F
waitForPrimeComplete0
  BTFSC GPIO, 0
  GOTO waitForPrimeComplete0
  RETLW 0
checkForPrime1
  INCF FSR, F
  BTFSC GPIO, 1
  RETLW 0
  INCF INDF, F
waitForPrimeComplete1
  BTFSC GPIO, 1
  GOTO waitForPrimeComplete1
  RETLW 0
checkForPrime2
  INCF FSR, F
  BTFSC GPIO, 2
  RETLW 0
  INCF INDF, F
waitForPrimeComplete2
  BTFSC GPIO, 2
  GOTO waitForPrimeComplete2
  RETLW 0
checkForPrime4
  INCF FSR, F
  BTFSC GPIO, 4
  RETLW 0
  INCF INDF, F
waitForPrimeComplete4
  BTFSC GPIO, 4
  GOTO waitForPrimeComplete4
  RETLW 0

allHigh
  MOVLW IIII
  TRIS GPIO
  RETLW 0

doA0
  CALL delay16
  CALL receiveSync0
  CALL write
  GOTO reset

doA1
  CALL delay16
  CALL receiveSync1
  CALL write
  GOTO reset

doA2
  CALL delay16
  CALL receiveSync2
  CALL write
  GOTO reset

doA4
  CALL delay16
  CALL receiveSync4
  CALL write
  GOTO reset

doB0b               ; 184       80                 2    loop duration 190 cycles +/- 2
  CALL sendRSync1   ; 186       82                14
  CALL receiveSync0 ;  10 200   96                10 +/- 1
  CALL write        ;  20      106                67
  CALL sendSync     ;  87      173                18
  CALL receiveSync0 ; 105                         10 +/- 1
  CALL read1        ; 115                         65
  MOVF afterB, W    ; 180                          1
  BTFSS GPIO, 1     ; 181                          1
  MOVLW doB0        ; 182                          1
  MOVWF PCL         ; 183                          1

doC0b
  CALL sendRSync2
  CALL receiveSync0
  CALL write
  CALL sendSync
  CALL receiveSync0
  CALL read2
  MOVF afterC, W
  BTFSS GPIO, 2
  MOVLW doC0
  MOVWF PCL

doD0b
  CALL sendRSync4
  CALL receiveSync0
  CALL write
  CALL sendSync
  CALL receiveSync0
  CALL read4
  MOVLW doA0
  BTFSS GPIO, 4
  MOVLW doD0
  MOVWF PCL

doB1b
  CALL sendRSync2
  CALL receiveSync1
  CALL write
  CALL sendSync
  CALL receiveSync1
  CALL read2
  MOVF afterB, W
  BTFSS GPIO, 2
  MOVLW doB1
  MOVWF PCL

doC1b
  CALL sendRSync4
  CALL receiveSync1
  CALL write
  CALL sendSync
  CALL receiveSync1
  CALL read4
  MOVF afterC, W
  BTFSS GPIO, 4
  MOVLW doC1
  MOVWF PCL

doD1b
  CALL sendRSync0
  CALL receiveSync1
  CALL write
  CALL sendSync
  CALL receiveSync1
  CALL read0
  MOVLW doA1
  BTFSS GPIO, 0
  MOVLW doD1
  MOVWF PCL

doB2b
  CALL sendRSync4
  CALL receiveSync2
  CALL write
  CALL sendSync
  CALL receiveSync2
  CALL read4
  MOVF afterB, W
  BTFSS GPIO, 4
  MOVLW doB2
  MOVWF PCL

doC2b
  CALL sendRSync0
  CALL receiveSync2
  CALL write
  CALL sendSync
  CALL receiveSync2
  CALL read0
  MOVF afterC, W
  BTFSS GPIO, 0
  MOVLW doC2
  MOVWF PCL

doD2b
  CALL sendRSync1
  CALL receiveSync2
  CALL write
  CALL sendSync
  CALL receiveSync2
  CALL read1
  MOVLW doA2
  BTFSS GPIO, 1
  MOVLW doD2
  MOVWF PCL

doB4b
  CALL sendRSync0
  CALL receiveSync4
  CALL write
  CALL sendSync
  CALL receiveSync4
  CALL read0
  MOVF afterB, W
  BTFSS GPIO, 0
  MOVLW doB4
  MOVWF PCL

doC4b
  CALL sendRSync1
  CALL receiveSync4
  CALL write
  CALL sendSync
  CALL receiveSync4
  CALL read1
  MOVF afterC, W
  BTFSS GPIO, 1
  MOVLW doC4
  MOVWF PCL

doD4b
  CALL sendRSync2
  CALL receiveSync4
  CALL write
  CALL sendSync
  CALL receiveSync4
  CALL read2
  MOVLW doA4
  BTFSS GPIO, 2
  MOVLW doD4
  MOVWF PCL


startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF GPIO

reset
  CLRF childBpresent
  CLRF childCpresent
  CLRF childDpresent
  MOVLW childBpresent-1
  MOVWF FSR
  CALL allHigh
; Time from input to output: 5-15 cycles
waitForPrime
  BTFSS GPIO, 0      ; 1 2 2 2 2
  GOTO found0        ; 2 0 0 0 0
  BTFSS GPIO, 1      ; 0 1 2 2 2
  GOTO found1        ; 0 2 0 0 0
  BTFSS GPIO, 2      ; 0 0 1 2 2
  GOTO found2        ; 0 0 2 0 0
  BTFSC GPIO, 4      ; 0 0 0 2 1
  GOTO waitForPrime  ; 0 0 0 0 2
;found4
  MOVLW OIIO
  CALL clearPrime4
  MOVLW childBpresent-1
  MOVWF FSR
  CALL checkForPrime0
  MOVLW OIOI
  CALL clearPrime4
  CALL checkForPrime1
  MOVLW OOII
  CALL clearPrime4
  CALL checkForPrime2
  CALL allHigh

  MOVLW doA4
  BTFSC childDpresent, 0
  MOVLW doD4
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doC4
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doB4
  MOVWF afterA
waitForDataRequest4 ; parent  child
  BTFSC GPIO, 4             ;   1
  GOTO waitForDataRequest4
  CALL receiveSync4         ;   3

  MOVLW 1 ; parent low      ;  13
  GOTO initData             ;  14


found0
  MOVLW IIOO
  CALL clearPrime0
  CALL checkForPrime1
  MOVLW IOIO
  CALL clearPrime0
  CALL checkForPrime2
  MOVLW OIIO
  CALL clearPrime0
  CALL checkForPrime4
  CALL allHigh

  MOVLW doA0
  BTFSC childDpresent, 0
  MOVLW doD0
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doC0
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doB0
  MOVWF afterA
waitForDataRequest0
  BTFSC GPIO, 0
  GOTO waitForDataRequest0
  CALL receiveSync0
  MOVLW 0 ; parent low
  GOTO initData


found1
  MOVLW IOOI
  CALL clearPrime1
  CALL checkForPrime2
  MOVLW OIOI
  CALL clearPrime1
  CALL checkForPrime4
  MOVLW IIOO
  CALL clearPrime1
  CALL checkForPrime0
  CALL allHigh

  MOVLW doA1
  BTFSC childDpresent, 0
  MOVLW doD1
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doC1
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doB1
  MOVWF afterA
waitForDataRequest1
  BTFSC GPIO, 1
  GOTO waitForDataRequest1
  CALL receiveSync1
  MOVLW 1 ; parent low
  GOTO initData


found2
  MOVLW OOII
  CALL clearPrime2
  CALL checkForPrime4
  MOVLW IOIO
  CALL clearPrime2
  CALL checkForPrime0
  MOVLW IOOI
  CALL clearPrime2
  CALL checkForPrime1
  CALL allHigh

  MOVLW doA2
  BTFSC childDpresent, 0
  MOVLW doD2
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doC2
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doB2
  MOVWF afterA
waitForDataRequest2
  BTFSC GPIO, 2
  GOTO waitForDataRequest2
  CALL receiveSync2

  MOVLW 0 ; parent low
  GOTO initData


initData                     ; 66
                             ; 2
  MOVWF parentLow            ; 1
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
  BTFSC GPIO, 3              ; 1
  INCF switch, F             ; 1

  MOVLW 17                   ; 1
  MOVWF count                ; 1
initDelayLoop
  DECFSZ count, F            ; 1*16 + 2
  GOTO initDelayLoop         ; 2*16
  delay2                     ; 2
  MOVF afterA, W             ; 1
  MOVWF PCL                  ; 1

  end


; 14 instructions free total, 14 in low page


; TODO:
;   Write PIC12 simulator to test/debug code

