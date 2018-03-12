; 1 channel

SCHEME_X EQU 1
SCHEME_Y EQU 0

%macro interruptStart 1
interrupt8_%1:
    push ax
patchPulseWidth_%1:
    mov al,99
  %if SCHEME_X != 0
    test al,al
    jz %%noPort42
  %endif
    out 0x42,al
  %%noPort42:
patchPulseWidthNext_%1:
    mov byte[cs:patchPulseWidth_%1 + 1],99
  %if SCHEME_Y != 0
    push bx
    push cx
  %endif
%endmacro

; ax = nextCount
; bx = phaseA
; bp = phaseB
; si = phaseC
; di = phaseD
; cl = pulseWidthNext
; ch = pulseWidthA
; dl = pulseWidthB
; dh = pulseWidthC

%macro interruptEnd 1
  %if SCHEME_Y != 0
patchPhase_%1:
    mov bx,9999
patchLastCount_%1:
    mov cx,9999
    sub bx,cx
  %endif
patchCurrentCount_%1:
  mov word[cs:patchLastCount_%1 + 1],9999
  mov word[cs:patchCurrentCount_%1 + 4],ax
  out 0x40,al
  mov al,ah
  out 0x40,al
  %if SCHEME_Y != 0
    pop cx
    jnc %%noOldInterrupt
patchInterruptCount_%1:
    add bx,9999
    mov [cs:patchPhase_%1 + 1],bx
    pop bx
patchOldInterrupt_%1:
    jmp far 9999:9999
  %%noOldInterrupt:
    pop bx
  %endif
  mov al,0x20
  out 0x20,al
  pop ax
  iret
%endmacro


interrupt8_1:
  interruptStart 1
patchCountA_1:
  mov ax,9999
patchPulseWidthA_1:
  mov [cs:patchPulseWidthNext+4],99
  interruptEnd 1


interrupt8_2:
  interruptStart 2
patchCountA_2:
  mov bx,



  push ax
patchPulseWidth_1:
  mov al,99
  out 0x42,al

  push bx
patchNextCount_1:
  mov bx,9999
  mov al,bl
  out 0x40,al
  mov al,bh
  out 0x40,al


patchPhase_1:
  mov ax,9999
patchCount_1:
  sub ax,9999
  jnc noCarry_1
patchOuterCount_1:
  add ax,9999
  mov [cs:patchPhase_1+1],ax
  pop ax
patchOldInterrupt8_1:
  jmp far 9999:9999
noCarry_1:
  mov al,0x20
  out 0x20,al
  pop ax
  iret


; 2 channels

interrupt8_2:
  push ax
patchPulseWidth_2:
  mov al,99
  out 0x42,al

  push bx
patchNextCount_2:
  mov bx,9999
  mov al,bl
  out 0x40,al
  mov al,bh
  out 0x40,al

patchPhaseA_2:
  mov ax,9999
  sub ax,bx







patchNextCountLow:
  mov al,99
  out 0x40,al
patchNextCountHigh:
  mov al,99
  out 0x40,al

patchPhaseA:
  mov ax,9999
patchFrequencyA:
  sub ax,9999
  jnc no



  mov al,0x20
  out 0x20,al
  pop ax
  iret
doOldInterrupt:
  pop ax
  jmp far [oldInterrupt8]



oldInterrupt8: dw 0, 0




interrupt8:
  push ax
patchPulseWidth:
  mov al,99
  out 0x42,al
patchNextCountLow:
  mov al,99
  out 0x40,al
patchNextCountHigh:
  mov al,99
  out 0x40,al

patchPhaseA:
  mov ax,9999
patchFrequencyA:
  sub ax,9999
  jnc no



  mov al,0x20
  out 0x20,al
  pop ax
  iret
doOldInterrupt:
  pop ax
  jmp far [oldInterrupt8]

