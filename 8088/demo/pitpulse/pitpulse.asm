; 1 channel

%macro interruptStart 1
interrupt8_%1:
  push ax
patchPulseWidth_%1:
  mov al,99
  test al,al
  jz %%noPort42
  out 0x42,al
%%noPort42:



interrupt8_1:
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

