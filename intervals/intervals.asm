  list p=12f508
#include "P12F508.INC"

#define length 0

Nbit EQU 4
Ebit EQU 5
Sbit EQU 0
Wbit EQU 1

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

lengthLow                  EQU 07h
lengthMiddle               EQU 08h
lengthHigh                 EQU 09h
parentLow                  EQU 0ah
switch                     EQU 0bh
childBpresent              EQU 0ch
childCpresent              EQU 0dh
childDpresent              EQU 0eh

count                      EQU 0fh
afterInitData              EQU 10h
afterN                     EQU 11h
afterE                     EQU 12h
afterS                     EQU 13h
afterW                     EQU 14h

AhighXlow                  EQU 15h
AlowXhigh                  EQU 16h
AhighXhigh                 EQU 17h

afterReceivePostWriteSync  EQU 18h
afterWrite                 EQU 19h
afterSendPostReadSync      EQU 1ah
bits                       EQU 1bh


  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC

  GOTO startup

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

initRW
  MOVLW 8
  MOVWF count
  MOVLW lengthLow
  MOVWF FSR
  RETLW 0

write
;  CALL delay4
  CALL initRW
writeBitsLoop
  MOVF AlowXhigh, W
  BTFSC INDF, 0
  MOVF AhighXhigh, W
  TRIS GPIO
  INCF FSR, F
  DECFSZ count, F
  GOTO writeBitsLoop
sendPostWriteSync
  MOVF AlowXhigh, W
  TRIS GPIO
;  CALL delay6          ; 6    1        -8
  MOVF AhighXlow, W    ; 1    1        -2
  TRIS GPIO            ; 1    1        -1
  MOVF AhighXhigh, W   ; 1    0         0
  TRIS GPIO            ; 1    0         1
  MOVF AhighXlow, W    ; 1    1         2
  TRIS GPIO            ; 1    1         3
  MOVF afterWrite, W
  MOVWF PCL


readBits macro source        ;      readN should be called 9 cycles after write
  local l                    ; 2  -11
  CALL initRW                ; 8   -9
  TRIS GPIO
l
  CLRF INDF                  ; 1   -1
  BTFSC GPIO, source         ; 1    0
  INCF INDF, F               ; 1
  INCF FSR, F                ; 1
  delay1                     ; 1
  DECFSZ count, F            ; 1
  GOTO l                     ; 2
;  delay1
  endm


sendPostReadSync macro destination  ;  2     total 14      needs to take 16?
  MOVLW destination          ;  1   1    -6
  TRIS GPIO                  ;  1   1    -5
  MOVWF AhighXlow            ;  1   0    -4
;  delay3                     ;  3   0    -3
  MOVF AhighXhigh, W         ;  1   0     0
  TRIS GPIO                  ;  1   0     1
  MOVF AhighXlow, W          ;  1   1     2
  TRIS GPIO                  ;  1   1     3
  MOVF afterSendPostReadSync, W
  MOVWF PCL
  endm

readN
  readBits Nbit
  MOVF afterN, W
  BTFSC GPIO, Nbit
  MOVWF PCL
doN
  MOVLW readN
  MOVWF afterReceivePostWriteSync
sendPostReadSyncN
  sendPostReadSync IIIO

readE
  readBits Ebit
  MOVF afterE, W
  BTFSC GPIO, Ebit
  MOVWF PCL
doE
  MOVLW readE
  MOVWF afterReceivePostWriteSync
sendPostReadSyncE
  sendPostReadSync IIOI

readS
  readBits Sbit
  MOVF afterS, W
  BTFSC GPIO, Sbit
  MOVWF PCL
doS
  MOVLW readS
  MOVWF afterReceivePostWriteSync
sendPostReadSyncS
  sendPostReadSync IOII

readW
  readBits Wbit
  MOVF afterW, W
  BTFSC GPIO, Wbit
  MOVWF PCL
doW
  MOVLW readW
  MOVWF afterReceivePostWriteSync
sendPostReadSyncW
  sendPostReadSync OIII


receivePostReadSync macro source        ; 9..11 cycles     receivePostReadSyncN should be called 9 cycles (+/-1) after sendPostWriteSync, 5 cycles (+/-1) after sendPostReadSync
  local l
  BTFSS GPIO, source        ; 0-2  0-2  2-3  (1)
  GOTO l                    ; 3-5  3-5
l
  BTFSS GPIO, source        ; 3-4  4-5  4-5
  GOTO write                ;      5-6  5-6
  GOTO write                ; 5-6
  endm

receivePostWriteSync macro source        ; 9..11 cycles     receivePostWriteSyncN should be called 9 cycles (+/-1) after sendPostWriteSync, 5 cycles (+/-1) after sendPostReadSync
  local l
  MOVF afterReceivePostWriteSync, W
  BTFSS GPIO, source        ; 0-2  0-2  2-3  (1)
  GOTO l                    ; 3-5  3-5
l
  BTFSS GPIO, source        ; 3-4  4-5  4-5
  MOVWF PCL                 ;      5-6  5-6
  MOVWF PCL                 ; 5-6
  endm

receivePostWriteSyncN
  receivePostWriteSync Nbit
receivePostWriteSyncE
  receivePostWriteSync Ebit
receivePostWriteSyncS
  receivePostWriteSync Sbit
receivePostWriteSyncW
  receivePostWriteSync Wbit

receivePostReadSyncN
  receivePostReadSync Nbit
receivePostReadSyncE
  receivePostReadSync Ebit
receivePostReadSyncS
  receivePostReadSync Sbit
receivePostReadSyncW
  receivePostReadSync Wbit

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
  BTFSC GPIO, Nbit
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteN
  BTFSS GPIO, Nbit
  GOTO waitForPrimeCompleteN
  RETLW 0
checkForPrimeE
  INCF FSR, F
  BTFSC GPIO, Ebit
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteE
  BTFSS GPIO, Ebit
  GOTO waitForPrimeCompleteE
  RETLW 0
checkForPrimeS
  INCF FSR, F
  BTFSC GPIO, Sbit
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteS
  BTFSS GPIO, Sbit
  GOTO waitForPrimeCompleteS
  RETLW 0
checkForPrimeW
  INCF FSR, F
  BTFSC GPIO, Wbit
  RETLW 0
  INCF INDF, F
waitForPrimeCompleteW
  BTFSS GPIO, Wbit
  GOTO waitForPrimeCompleteW
  RETLW 0


doAN
;  CALL delay16
  MOVLW reset
  MOVWF afterWrite
  GOTO receivePostReadSyncN

doAE
;  CALL delay16
  MOVLW reset
  MOVWF afterWrite
  GOTO receivePostReadSyncE

doAS
;  CALL delay16
  MOVLW reset
  MOVWF afterWrite
  GOTO receivePostReadSyncS

doAW
;  CALL delay16
  MOVLW reset
  MOVWF afterWrite
  GOTO receivePostReadSyncW


initData0
  MOVLW 0  ; parent low
  GOTO initData

initData1
  MOVLW 1  ; parent low
  GOTO initData


startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF GPIO

reset
  MOVLW IIII
  TRIS GPIO
  CLRF childBpresent
  CLRF childCpresent
  CLRF childDpresent
  MOVLW childBpresent-1
  MOVWF FSR
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
foundW
  MOVLW OIIO
  CALL clearPrimeW
  CALL checkForPrimeN
  MOVLW OIOI
  CALL clearPrimeW
  CALL checkForPrimeE
  MOVLW OOII
  CALL clearPrimeW
  CALL checkForPrimeS
;  CALL allHigh

  MOVLW doAW
  MOVWF afterS
  BTFSC childDpresent, 0
  MOVLW doS
  MOVWF afterE
  BTFSC childCpresent, 0
  MOVLW doE
  MOVWF afterN
  BTFSC childBpresent, 0
  MOVLW doN
  MOVWF afterInitData

  MOVLW receivePostReadSyncW
  MOVWF afterSendPostReadSync
  MOVLW receivePostWriteSyncW
  MOVWF afterWrite

  MOVLW initData1
  MOVWF afterReceivePostWriteSync

  MOVLW OIII
  MOVWF AlowXhigh

waitForDataRequestW ; parent  child
  BTFSC GPIO, 1
  GOTO waitForDataRequestW
  GOTO receivePostWriteSyncW


foundN
  MOVLW IIOO
  CALL clearPrimeN
  CALL checkForPrimeE
  MOVLW IOIO
  CALL clearPrimeN
  CALL checkForPrimeS
  MOVLW OIIO
  CALL clearPrimeN
  CALL checkForPrimeW
;  CALL allHigh

  MOVLW doAN
  MOVWF afterW
  BTFSC childDpresent, 0
  MOVLW doW
  MOVWF afterS
  BTFSC childCpresent, 0
  MOVLW doS
  MOVWF afterE
  BTFSC childBpresent, 0
  MOVLW doE
  MOVWF afterInitData

  MOVLW receivePostReadSyncN
  MOVWF afterSendPostReadSync
  MOVLW receivePostWriteSyncN
  MOVWF afterWrite

  MOVLW initData0
  MOVWF afterReceivePostWriteSync

  MOVLW IIIO
  MOVWF AlowXhigh

waitForDataRequestN
  BTFSC GPIO, 4
  GOTO waitForDataRequestN
  GOTO receivePostWriteSyncN


foundE
  MOVLW IOOI
  CALL clearPrimeE
  CALL checkForPrimeS
  MOVLW OIOI
  CALL clearPrimeE
  CALL checkForPrimeW
  MOVLW IIOO
  CALL clearPrimeE
  CALL checkForPrimeN
;  CALL allHigh

  MOVLW doAE
  MOVWF afterN
  BTFSC childDpresent, 0
  MOVLW doN
  MOVWF afterW
  BTFSC childCpresent, 0
  MOVLW doW
  MOVWF afterS
  BTFSC childBpresent, 0
  MOVLW doS
  MOVWF afterInitData

  MOVLW receivePostReadSyncE
  MOVWF afterSendPostReadSync
  MOVLW receivePostWriteSyncE
  MOVWF afterWrite

  MOVLW initData1
  MOVWF afterReceivePostWriteSync

  MOVLW IIOI
  MOVWF AlowXhigh

waitForDataRequestE
  BTFSC GPIO, 5
  GOTO waitForDataRequestE
  GOTO receivePostWriteSyncE


foundS
  MOVLW OOII
  CALL clearPrimeS
  CALL checkForPrimeW
  MOVLW IOIO
  CALL clearPrimeS
  CALL checkForPrimeN
  MOVLW IOOI
  CALL clearPrimeS
  CALL checkForPrimeE
;  CALL allHigh

  MOVLW doAS
  MOVWF afterE
  BTFSC childDpresent, 0
  MOVLW doE
  MOVWF afterN
  BTFSC childCpresent, 0
  MOVLW doN
  MOVWF afterW
  BTFSC childBpresent, 0
  MOVLW doW
  MOVWF afterInitData

  MOVLW receivePostReadSyncS
  MOVWF afterSendPostReadSync
  MOVLW receivePostWriteSyncS
  MOVWF afterWrite

  MOVLW initData0
  MOVWF afterReceivePostWriteSync

  MOVLW IOII
  MOVWF AlowXhigh

waitForDataRequestS
  BTFSC GPIO, 0
  GOTO waitForDataRequestS
  GOTO receivePostWriteSyncS


initData                     ; 66
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

  MOVLW 0x12                 ; 1
  MOVWF count                ; 1
initDelayLoop
  DECFSZ count, F            ; 1*17 + 2
  GOTO initDelayLoop         ; 2*17
  delay1                     ; 1
  MOVF afterInitData, W      ; 1
  MOVWF PCL                  ; 1

  end


; TODO:
;   Fix receiveSync comments
;   Move AlowXhigh and AhighXlow init to more suitable places
;   Keep track of bits
;   When writing the last byte we don't want to sendSync (or it will be interpreted as another prime) so set AlowXhigh and AhighXlow to IIII first
;   Fixup timings
;   Check how we're doing for space
;   Can we get rid of any of the indirect GOTOs?
;   read and write loop unroll: 1? 2? 4? 8?
;   combine readX and doX?
;
; initData
; [afterInitData]
; doX
; sendPostReadSyncX          AhighXlow init
; [afterSendPostReadSync]
; receivePostReadSyncA
; write
; sendPostWriteSync
; [afterWrite]
; receivePostWriteSyncA
; [afterReceivePostWriteSync]
; readX
; doXcheck
; doX or [afterX]
;
; Unroll write to 8 costs:
;   7*4 = 28
; saves: 4
;
; Unroll read to 8 costs:
;   7*4*4 = 112
; saves: 21
;
; total cost: 115
;
; words free: 125
;
; => 10 words to get timing and bit tracking correct
;   => will probably need to save some more space to do full unroll
