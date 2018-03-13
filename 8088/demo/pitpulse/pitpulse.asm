; 1 channel

SCHEME_X EQU 1
SCHEME_Y EQU 0
consolidateCycles equ 100

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
  push cx
  %if SCHEME_Y != 0
    push bx
  %endif
%endmacro

; ax = nextCount
; bx = phase0
; bp = phase1
; si = phase2
; di = phase3
; cl = pulseWidthNext

%macro interruptEnd 1
  mov [cs:patchPhase0_%1],bx
  %if %1 > 1
    mov [cs:patchPhase1_%1],bp
    %if %1 > 2
      mov [cs:patchPhase2_%1],si
      %if %1 > 3
        mov [cs:patchPhase3_%1],di]
      %endif
    %endif
  %endif
  mov byte[cs:patchPulseWidthNext_%1 + 4],cl
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
    jnc %%noOldInterrupt
patchInterruptCount_%1:
    add bx,9999
    mov [cs:patchPhase_%1 + 1],bx
    pop bx
    pop cx
    pop ax
patchOldInterrupt_%1:
    jmp far 9999:9999
  %%noOldInterrupt:
    pop bx
  %endif
  pop cx
  mov al,0x20
  out 0x20,al
  pop ax
  iret
%endmacro

%define register0 bx
%define register1 bp
%define register2 si
%define register3 di

%macro consolidate 2  ; channels channel
    sub register%[%1],ax
    cmp register%[%1],consolidateCycles
    jae %%noConsolidate
    add register%[%1],[cs:patchCount%2_%1 + 1]
    add cl,[cs:patchPulseWidth%2_%1 + 1]
  %%noConsolidate:
%endmacro

%macro consolidates 2  ; channels channel
  %assign i 0
  %rep %1
    %if i != %2
       consolidate %1 i
    %endif
    %assign i i+1
  %endrep
%endmacro


interrupt8_1:
  interruptStart 1
patchCount0_1:
  mov ax,9999
patchPulseWidth0_1:
  mov cl,99
  interruptEnd 1


interrupt8_2:
  interruptStart 2
patchPhase0_2:
  mov bx,9999
patchPhase1_2:
  mov bp,9999
  cmp bp,bx
  jl next1
  mov ax,bx


