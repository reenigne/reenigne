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

data0                      EQU 07h
data1                      EQU 08h
data2                      EQU 09h
data3                      EQU 0ah
data4                      EQU 0bh
data5                      EQU 0ch
data6                      EQU 0dh
data7                      EQU 0eh

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

afterReceivePostWriteSync  EQU 15h
afterWrite                 EQU 16h
afterSendPostReadSync      EQU 17h
bits                       EQU 18h
Alow                       EQU 19h
Xlow                       EQU 1ah
more                       EQU 1bh


  __config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC

  GOTO startup

reset
  GOTO reset1
primeN
  GOTO primeN1
primeE
  GOTO primeE1
primeS
  GOTO primeS1
primeW
  GOTO primeW1

initData0
  MOVLW 0  ; parent low
  GOTO initData

initData1
  MOVLW 1  ; parent low
  GOTO initData

trisAndDelay16
  TRIS GPIO
delay16
  GOTO delay14
delay14
  GOTO delay12
delay12
  GOTO delay10
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

readBit macro address, source  ; 4
  CLRF address               ; 1   -1
  BTFSC GPIO, source         ; 1    0
  INCF address, F            ; 1
  delay1                     ; 1
  endm

readBits macro source        ; 32
  readBit data0, source      ; 4
  readBit data1, source      ; 4
  readBit data2, source      ; 4
  readBit data3, source      ; 4
  readBit data4, source      ; 4
  readBit data5, source      ; 4
  readBit data6, source      ; 4
  readBit data7, source      ; 4
;  delay1
  endm

readN                              ; 35 41
  readBits Nbit                    ; 32
  MOVF afterN, W                   ;  1
  BTFSC GPIO, Nbit                 ;  1  2
  MOVWF PCL                        ;  1  0
doN
  BSF bits, Nbit                   ;     1
  MOVLW readN                      ;     1
  MOVWF afterReceivePostWriteSync  ;     1
  MOVLW IIIO                       ;     1
  GOTO sendPostReadSync            ;     2

readE
  readBits Ebit
  MOVF afterE, W
  BTFSC GPIO, Ebit
  MOVWF PCL
doE
  BSF bits, Ebit
  MOVLW readE
  MOVWF afterReceivePostWriteSync
  MOVLW IIOI
  GOTO sendPostReadSync

readS
  readBits Sbit
  MOVF afterS, W
  BTFSC GPIO, Sbit
  MOVWF PCL
doS
  BSF bits, Sbit
  MOVLW readS
  MOVWF afterReceivePostWriteSync
  MOVLW IOII
  GOTO sendPostReadSync

readW
  readBits Wbit
  MOVF afterW, W
  BTFSC GPIO, Wbit
  MOVWF PCL
doW
  BSF bits, Wbit
  MOVLW readW
  MOVWF afterReceivePostWriteSync
  MOVLW OIII
  GOTO sendPostReadSync


receivePostReadSync macro source ; 5-7
  local l
  BTFSC GPIO, source        ; 0-2  0-2  2-3  (1)
  GOTO l                    ; 3-5  3-5
l
  BTFSC GPIO, source        ; 3-4  4-5  4-5
  GOTO write                ;      5-6  5-6
  GOTO write                ; 5-6
  endm

receivePostReadSyncN
  receivePostReadSync Nbit
receivePostReadSyncE
  receivePostReadSync Ebit
receivePostReadSyncS
  receivePostReadSync Sbit
receivePostReadSyncW
  receivePostReadSync Wbit

receivePostWriteSync macro source ; 6-8
  local l
  MOVF afterReceivePostWriteSync, W
  BTFSC GPIO, source        ; 0-2  0-2  2-3  (1)
  GOTO l                    ; 3-5  3-5
l
  BTFSC GPIO, source        ; 3-4  4-5  4-5
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

doFinalWrite                     ; 15
  CALL delay9                    ; 9
  MOVLW reset                    ; 1
  MOVWF afterWrite               ; 1
  MOVLW IIII                     ; 1
  MOVWF more                     ; 1
  MOVF afterSendPostReadSync, W  ; 1
  MOVWF PCL                      ; 1

writeBit macro address
  MOVF bits, W                   ; 1
  BTFSS address, 0               ; 1
  ANDWF Alow, W                  ; 1
  TRIS GPIO                      ; 1
  endm

write                            ; 43
;  CALL delay4
  writeBit data0                 ;  4
  writeBit data1                 ;  4
  writeBit data2                 ;  4
  writeBit data3                 ;  4
  writeBit data4                 ;  4
  writeBit data5                 ;  4
  writeBit data6                 ;  4
  writeBit data7                 ;  4
  MOVF bits, W                   ;  1
  ANDWF more, W                  ;  1
  TRIS GPIO                      ;  1             ; more
;  CALL delay6
  MOVF bits, W                   ;  1
  TRIS GPIO                      ;  1             ; sync init
  ANDWF more, W                  ;  1
  TRIS GPIO                      ;  1             ; sync first transition
  MOVF bits, W                   ;  1
  TRIS GPIO                      ;  1             ; sync second transition
  MOVF afterWrite, W             ;  1
  MOVWF PCL                      ;  1

primeN1                          ; >= 27
                                 ;  2  2
  MOVF bits, W                   ;  1  1
  ANDLW IIIO                     ;  1  1
  CALL trisAndDelay16            ; 17 17
  MOVF bits, W                   ;  1  1
  TRIS GPIO                      ;  1  1
  INCF FSR, F                    ;  1  1
  BTFSC GPIO, Nbit               ;  1  2
  RETLW 0                        ;  2  0
  INCF INDF, F                   ;     1
  BCF bits, Nbit                 ;     1
waitForPrimeCompleteN
  BTFSS GPIO, Nbit               ;  1 2
  GOTO waitForPrimeCompleteN     ;  2 0
  RETLW 0                        ;    2

primeE1
  MOVF bits, W
  ANDLW IIOI
  CALL trisAndDelay16
  MOVF bits, W
  TRIS GPIO
  INCF FSR, F
  BTFSC GPIO, Ebit
  RETLW 0
  INCF INDF, F
  BCF bits, Ebit
waitForPrimeCompleteE
  BTFSS GPIO, Ebit
  GOTO waitForPrimeCompleteE
  RETLW 0

primeS1
  MOVF bits, W
  ANDLW IOII
  CALL trisAndDelay16
  MOVF bits, W
  TRIS GPIO
  INCF FSR, F
  BTFSC GPIO, Sbit
  RETLW 0
  INCF INDF, F
  BCF bits, Sbit
waitForPrimeCompleteS
  BTFSS GPIO, Sbit
  GOTO waitForPrimeCompleteS
  RETLW 0

primeW1
  MOVF bits, W
  ANDLW OIII
  CALL trisAndDelay16
  MOVF bits, W
  TRIS GPIO
  INCF FSR, F
  BTFSC GPIO, Wbit
  RETLW 0
  INCF INDF, F
  BCF bits, Wbit
waitForPrimeCompleteW
  BTFSS GPIO, Wbit
  GOTO waitForPrimeCompleteW
  RETLW 0


startup
  MOVWF OSCCAL
  MOVLW 80h                  ; wake up on pin change disabled (80h) | weak pull-ups enabled (00h) | timer 0 clock source on instruction cycle (00h) | timer 0 source
  OPTION
  CLRF GPIO
  MOVLW IIII
  MOVWF bits

reset1
  CLRF childBpresent     ; 1
  CLRF childCpresent     ; 1
  CLRF childDpresent     ; 1
  MOVLW childBpresent-1  ; 1
  MOVWF FSR              ; 1
; Time from input to output: 5-15 cycles
waitForPrime
  BTFSS GPIO, Nbit   ; 1 2 2 2 2
  GOTO foundN        ; 2 0 0 0 0
  BTFSS GPIO, Ebit   ; 0 1 2 2 2
  GOTO foundE        ; 0 2 0 0 0
  BTFSS GPIO, Sbit   ; 0 0 1 2 2
  GOTO foundS        ; 0 0 2 0 0
  BTFSC GPIO, Wbit   ; 0 0 0 2 1
  GOTO waitForPrime  ; 0 0 0 0 2
foundW
  BCF bits, Wbit          ; 1
  CALL primeN             ; >=27
  CALL primeE             ; >=27
  CALL primeS             ; >=27
  BSF bits, Wbit          ; 1
  MOVF bits, W            ; 1
  TRIS GPIO               ; 1

  MOVLW doFinalWrite      ; 1
  MOVWF afterS            ; 1
  BTFSC childDpresent, 0  ; 1 2
  MOVLW doS               ; 1 0
  MOVWF afterE            ; 1
  BTFSC childCpresent, 0  ; 1 2
  MOVLW doE               ; 1 0
  MOVWF afterN            ; 1
  BTFSC childBpresent, 0  ; 1 2
  MOVLW doN               ; 1 0
  MOVWF afterInitData     ; 1

  MOVLW initData1                  ; 1
  MOVWF afterReceivePostWriteSync  ; 1

  MOVLW OIII                       ; 1
  MOVWF Alow                       ; 1
  MOVWF more                       ; 1

  MOVLW receivePostReadSyncW       ; 1
  MOVWF afterSendPostReadSync      ; 1
  MOVLW receivePostWriteSyncW      ; 1
  MOVWF afterWrite                 ; 1

waitForDataRequestW
  BTFSS GPIO, Wbit                 ; 1 2          1
  GOTO waitForDataRequestW         ; 2 0
  MOVWF PCL                        ;   1          3


foundN
  BCF bits, Nbit
  CALL primeE
  CALL primeS
  CALL primeW
  BSF bits, Nbit
  MOVF bits, W
  TRIS GPIO

  MOVLW doFinalWrite
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

  MOVLW initData0
  MOVWF afterReceivePostWriteSync

  MOVLW IIIO
  MOVWF Alow
  MOVWF more

  MOVLW receivePostReadSyncN
  MOVWF afterSendPostReadSync
  MOVLW receivePostWriteSyncN
  MOVWF afterWrite

waitForDataRequestN
  BTFSS GPIO, Nbit
  GOTO waitForDataRequestN
  MOVWF PCL


foundE
  BCF bits, Ebit
  CALL primeS
  CALL primeW
  CALL primeN
  BSF bits, Ebit
  MOVF bits, W
  TRIS GPIO

  MOVLW doFinalWrite
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

  MOVLW initData1
  MOVWF afterReceivePostWriteSync

  MOVLW IIOI
  MOVWF Alow
  MOVWF more

  MOVLW receivePostReadSyncE
  MOVWF afterSendPostReadSync
  MOVLW receivePostWriteSyncE
  MOVWF afterWrite

waitForDataRequestE
  BTFSS GPIO, Ebit
  GOTO waitForDataRequestE
  MOVWF PCL


foundS
  BCF bits, Sbit
  CALL primeW
  CALL primeN
  CALL primeE
  BSF bits, Sbit
  MOVF bits, W
  TRIS GPIO

  MOVLW doFinalWrite
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

  MOVLW initData0
  MOVWF afterReceivePostWriteSync

  MOVLW IOII
  MOVWF Alow
  MOVWF more

  MOVLW receivePostReadSyncS
  MOVWF afterSendPostReadSync
  MOVLW receivePostWriteSyncS
  MOVWF afterWrite

waitForDataRequestS
  BTFSS GPIO, Sbit
  GOTO waitForDataRequestS
  MOVWF PCL


initData
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

;  MOVLW 0x12                 ; 1
;  MOVWF count                ; 1
;initDelayLoop
;  DECFSZ count, F            ; 1*17 + 2
;  GOTO initDelayLoop         ; 2*17
;  delay1                     ; 1
  MOVF afterInitData, W      ; 1
  MOVWF PCL                  ; 1


sendPostReadSync
  MOVWF Xlow                     ; 1
  MOVF bits, W                   ; 1
  TRIS GPIO                      ; 1              data request and sync init
;  delay3
  ANDWF Xlow, W                  ; 1
  TRIS GPIO                      ; 1              first transition
  MOVF bits, W                   ; 1
  TRIS GPIO                      ; 1              second transition
  MOVF afterSendPostReadSync, W  ; 1
  MOVWF PCL                      ; 1

  end


; TODO:
;   Change NESW to 0123 and use macros to combine foundX, primeX1, readX/doX
;   Can we get rid of a pair of syncs?
;   Fixup timings
;     initData1 needs to be same length as readX (add delay of 23 cycles + read delay)
;   Fix receiveSync comments
;   Use an afterWrite (and change afterWrite to afterSendPostWriteSync) instead of more?
;     Takes an extra cycle in write, but saves 2 instructions
;
; receivePostWriteSyncA         7 +/- 1      4
; initData                      2           11
; initData1                    10           13
;
; doX                           6
; sendPostReadSync              9
; receivePostReadSyncA          6 +/- 1
; write                        43
; receivePostWriteSyncA         7 +/- 1
; readX                        35
;
; doFinalWrite                 15                    +sendPostReadSync delay
; receivePostReadSyncA          6
; write                        43
; reset                         7

;
; 23 instructions free, 27 in first half
