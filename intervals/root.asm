  list p=12f508
#include "P12F508.INC"

IIII EQU 3fh
IOII EQU 3eh

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

doC
  GOTO doCb

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
  TRIS 5                     ; 1   -3
  INCF FSR, F                ; 1   -2
  MOVF AlowBhigh, W          ; 1   -1
  BTFSC INDF, 0              ; 1    0 <- first read here
  MOVLW IIII                 ; 1
  TRIS 5                     ; 1
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

read
  readBits 0

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

sendRSync
  MOVLW IOII                 ;  1   1    -6
  TRIS GPIO                  ;  1   1    -5
  MOVWF AhighBlow            ;  1   0    -4
  delay3                     ;  3   0    -3
  MOVLW IIII                 ;  1   0     0
  TRIS GPIO                  ;  1   0     1
  MOVF AhighBlow, W          ;  1   1     2
  TRIS GPIO                  ;  1   1     3
  RETLW 0                    ;  2   0     4

receiveSync         ; 9..11 cycles     recieveSyncN should be called 9 cycles (+/-1) after sendSync, 5 cycles (+/-1) after sendRSync
  MOVLW 0x2f
  MOVWF AlowBhigh
  NOP
  NOP
  NOP
  NOP
  RETLW 0


clearPrime
  CALL delay16T
  MOVLW IIII
  TRIS GPIO
  RETLW 0

checkForPrime
  INCF FSR, F
  BTFSC GPIO, 0
  RETLW 0
  INCF INDF, F
waitForPrimeComplete
  BTFSC GPIO, 0
  GOTO waitForPrimeComplete
  RETLW 0

allHigh
  MOVLW IIII
  TRIS GPIO
  RETLW 0

doCb                ; 184       80                 2    loop duration 190 cycles +/- 2
  CALL sendRSync    ; 186       82                14
  CALL receiveSync  ;  10 200   96                10 +/- 1
  CALL write        ;  20      106                67
  CALL sendSync     ;  87      173                18
  CALL receiveSync  ; 105                         10 +/- 1
  CALL read         ; 115                         65
  MOVLW doA         ; 180                          1
  BTFSS GPIO, 0     ; 181                          1
  MOVLW doC         ; 182                          1
  MOVWF PCL         ; 183                          1

doA
  CALL delay16
  CALL receiveSync
  CALL write
  GOTO reset

startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF GPIO

reset
  TRIS 7
  CLRF childBpresent
  CLRF childCpresent
  CLRF childDpresent
  MOVLW IOII
  CALL clearPrime
  MOVLW childBpresent-1
  MOVWF FSR
  CALL checkForPrime
  CALL allHigh

  CALL delay12
  MOVLW doA
  BTFSC childCpresent, 0
  MOVLW doC
  MOVWF afterA
  NOP
  NOP
  NOP
  NOP
  NOP
  NOP
  MOVLW 0 ; parent low
  GOTO initData

initData                     ; 66
                             ; 2
  MOVWF parentLow            ; 1
  MOVLW 1                    ; 1
  CLRF lengthLow
  CLRF lengthMiddle
  CLRF lengthHigh
  MOVWF switch
  NOP
  NOP

  MOVLW 17                   ; 1
  MOVWF count                ; 1
initDelayLoop
  DECFSZ count, F            ; 1*16 + 2
  GOTO initDelayLoop         ; 2*16
  delay2                     ; 2
  MOVF afterA, W             ; 1
  MOVWF PCL                  ; 1

  end

