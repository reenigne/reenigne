  list p=12f508
#include "P12F508.INC"

#define length 0

; 4210  old
; WSEN
; 1054
IIII EQU 3fh

IIIO EQU 2fh
IIOI EQU 1fh
IOII EQU 3eh
OIII EQU 3dh

IIOO EQU 0fh
IOOI EQU 1eh
OOII EQU 3ch
OIIO EQU 2dh

IOIO EQU 2eh
OIOI EQU 1dh

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

doBN
  GOTO doBNb
doCN
  GOTO doCNb
doDN
  GOTO doDNb
doBE
  GOTO doBEb
doCE
  gOTO doCEb
doDE
  GOTO doDEb
doBS
  GOTO doBSb
doCS
  GOTO doCSb
doDS
  GOTO doDSb
doBW
  GOTO doBWb
doCW
  GOTO doCWb
doDW
  GOTO doDWb

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

readN
  readBits 4
readE
  readBits 5
readS
  readBits 0
readW
  readBits 1

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

sendRSyncN
  sendRSync IIIO
sendRSyncE
  sendRSync IIOI
sendRSyncS
  sendRSync IOII
sendRSyncW
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

receiveSyncN
  receiveSync 4
receiveSyncE
  receiveSync 5
receiveSyncS
  receiveSync 0
receiveSyncW
  receiveSync 1

clearPrimeN
  CALL delay16T
  MOVLW IIIO
  TRIS GPIO
  RETLW 0
clearPrimeE
  CALL delay16T
  MOVLW IIOI
  TRIS GPIO
  RETLW 0
clearPrimeS
  CALL delay16T
  MOVLW IOII
  TRIS GPIO
  RETLW 0
clearPrimeW
  CALL delay16T
  MOVLW OIII
  TRIS GPIO
  RETLW 0

checkForPrimeN
  INCF FSR, F
  BTFSC GPIO, 4
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteN
  BTFSC GPIO, 4
  GOTO waitForPrimeCompleteN
  RETLW 0
checkForPrimeE
  INCF FSR, F
  BTFSC GPIO, 5
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteE
  BTFSC GPIO, 5
  GOTO waitForPrimeCompleteE
  RETLW 0
checkForPrimeS
  INCF FSR, F
  BTFSC GPIO, 0
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteS
  BTFSC GPIO, 0
  GOTO waitForPrimeCompleteS
  RETLW 0
checkForPrimeW
  INCF FSR, F
  BTFSC GPIO, 1
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteW
  BTFSC GPIO, 1
  GOTO waitForPrimeCompleteW
  RETLW 0

allHigh
  MOVLW IIII
  TRIS GPIO
  RETLW 0

doAN
  CALL delay16
  CALL receiveSyncN
  CALL write
  GOTO reset

doAE
  CALL delay16
  CALL receiveSyncE
  CALL write
  GOTO reset

doAS
  CALL delay16
  CALL receiveSyncS
  CALL write
  GOTO reset

doAW
  CALL delay16
  CALL receiveSyncW
  CALL write
  GOTO reset

doBNb               ; 184       80                 2    loop duration 190 cycles +/- 2
  CALL sendRSyncE   ; 186       82                14
  CALL receiveSyncN ;  10 200   96                10 +/- 1
  CALL write        ;  20      106                67
  CALL sendSync     ;  87      173                18
  CALL receiveSyncN ; 105                         10 +/- 1
  CALL readE        ; 115                         65
  MOVF afterB, W    ; 180                          1
  BTFSS GPIO, 5     ; 181                          1
  MOVLW doBN        ; 182                          1
  MOVWF PCL         ; 183                          1

doCNb
  CALL sendRSyncS
  CALL receiveSyncN
  CALL write
  CALL sendSync
  CALL receiveSyncN
  CALL readS
  MOVF afterC, W
  BTFSS GPIO, 0
  MOVLW doCN
  MOVWF PCL

doDNb
  CALL sendRSyncW
  CALL receiveSyncN
  CALL write
  CALL sendSync
  CALL receiveSyncN
  CALL readW
  MOVLW doAN
  BTFSS GPIO, 1
  MOVLW doDN
  MOVWF PCL

doBEb
  CALL sendRSyncS
  CALL receiveSyncE
  CALL write
  CALL sendSync
  CALL receiveSyncE
  CALL readS
  MOVF afterB, W
  BTFSS GPIO, 0
  MOVLW doBE
  MOVWF PCL

doCEb
  CALL sendRSyncW
  CALL receiveSyncE
  CALL write
  CALL sendSync
  CALL receiveSyncE
  CALL readW
  MOVF afterC, W
  BTFSS GPIO, 1
  MOVLW doCE
  MOVWF PCL

doDEb
  CALL sendRSyncN
  CALL receiveSyncE
  CALL write
  CALL sendSync
  CALL receiveSyncE
  CALL readN
  MOVLW doAE
  BTFSS GPIO, 4
  MOVLW doDE
  MOVWF PCL

doBSb
  CALL sendRSyncW
  CALL receiveSyncS
  CALL write
  CALL sendSync
  CALL receiveSyncS
  CALL readW
  MOVF afterB, W
  BTFSS GPIO, 1
  MOVLW doBS
  MOVWF PCL

doCSb
  CALL sendRSyncN
  CALL receiveSyncS
  CALL write
  CALL sendSync
  CALL receiveSyncS
  CALL readN
  MOVF afterC, W
  BTFSS GPIO, 4
  MOVLW doCS
  MOVWF PCL

doDSb
  CALL sendRSyncE
  CALL receiveSyncS
  CALL write
  CALL sendSync
  CALL receiveSyncS
  CALL readE
  MOVLW doAS
  BTFSS GPIO, 5
  MOVLW doDS
  MOVWF PCL

doBWb
  CALL sendRSyncN
  CALL receiveSyncW
  CALL write
  CALL sendSync
  CALL receiveSyncW
  CALL readN
  MOVF afterB, W
  BTFSS GPIO, 4
  MOVLW doBW
  MOVWF PCL

doCWb
  CALL sendRSyncE
  CALL receiveSyncW
  CALL write
  CALL sendSync
  CALL receiveSyncW
  CALL readE
  MOVF afterC, W
  BTFSS GPIO, 5
  MOVLW doCW
  MOVWF PCL

doDWb
  CALL sendRSyncS
  CALL receiveSyncW
  CALL write
  CALL sendSync
  CALL receiveSyncW
  CALL readS
  MOVLW doAW
  BTFSS GPIO, 0
  MOVLW doDW
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
  BTFSS GPIO, 4      ; 1 2 2 2 2
  GOTO foundN        ; 2 0 0 0 0
  BTFSS GPIO, 5      ; 0 1 2 2 2
  GOTO foundE        ; 0 2 0 0 0
  BTFSS GPIO, 0      ; 0 0 1 2 2
  GOTO foundS        ; 0 0 2 0 0
  BTFSC GPIO, 1      ; 0 0 0 2 1
  GOTO waitForPrime  ; 0 0 0 0 2
;foundW
  MOVLW OIIO
  CALL clearPrimeW
  MOVLW childBpresent-1
  MOVWF FSR
  CALL checkForPrimeN
  MOVLW OIOI
  CALL clearPrimeW
  CALL checkForPrimeE
  MOVLW OOII
  CALL clearPrimeW
  CALL checkForPrimeS
  CALL allHigh

  MOVLW doAW
  BTFSC childDpresent, 0
  MOVLW doDW
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doCW
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doBW
  MOVWF afterA
waitForDataRequestW ; parent  child
  BTFSC GPIO, 1             ;   1
  GOTO waitForDataRequestW
  CALL receiveSyncW         ;   3

  MOVLW 1 ; parent low      ;  13
  GOTO initData             ;  14


foundN
  MOVLW IIOO
  CALL clearPrimeN
  MOVLW childBpresent-1
  MOVWF FSR
  CALL checkForPrimeE
  MOVLW IOIO
  CALL clearPrimeN
  CALL checkForPrimeS
  MOVLW OIIO
  CALL clearPrimeN
  CALL checkForPrimeW
  CALL allHigh

  MOVLW doAN
  BTFSC childDpresent, 0
  MOVLW doDN
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doCN
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doBN
  MOVWF afterA
waitForDataRequestN
  BTFSC GPIO, 4
  GOTO waitForDataRequestN
  CALL receiveSyncN
  MOVLW 0 ; parent low
  GOTO initData


foundE
  MOVLW IOOI
  CALL clearPrimeE
  MOVLW childBpresent-1
  MOVWF FSR
  CALL checkForPrimeS
  MOVLW OIOI
  CALL clearPrimeE
  CALL checkForPrimeW
  MOVLW IIOO
  CALL clearPrimeE
  CALL checkForPrimeN
  CALL allHigh

  MOVLW doAE
  BTFSC childDpresent, 0
  MOVLW doDE
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doCE
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doBE
  MOVWF afterA
waitForDataRequestE
  BTFSC GPIO, 5
  GOTO waitForDataRequestE
  CALL receiveSyncE
  MOVLW 1 ; parent low
  GOTO initData


foundS
  MOVLW OOII
  CALL clearPrimeS
  MOVLW childBpresent-1
  MOVWF FSR
  CALL checkForPrimeW
  MOVLW IOIO
  CALL clearPrimeS
  CALL checkForPrimeN
  MOVLW IOOI
  CALL clearPrimeS
  CALL checkForPrimeE
  CALL allHigh

  MOVLW doAS
  BTFSC childDpresent, 0
  MOVLW doDS
  MOVWF afterC
  BTFSC childCpresent, 0
  MOVLW doCS
  MOVWF afterB
  BTFSC childBpresent, 0
  MOVLW doBS
  MOVWF afterA
waitForDataRequestS
  BTFSC GPIO, 0
  GOTO waitForDataRequestS
  CALL receiveSyncS

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
  BTFSC GPIO, 2              ; 1
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


; 8 instructions free total, 8 in low page


; TODO:
;   Write PIC12 simulator to test/debug code

