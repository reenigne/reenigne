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
  push bx
patchPhase0_%1:
  mov bx,9999
  %if %1 > 1
    push bp
patchPhase1_%1:
    mov bp,9999
    %if %1 > 2
      push si
patchPhase2_%1:
      mov si,9999
      %if %1 > 3
        push di
patchPhase3_%1:
        mov di,9999
      %endif
    %endif
  %endif
%endmacro

; ax = nextCount
; bx = phase0
; bp = phase1
; si = phase2
; di = phase3
; cl = pulseWidthNext

%macro interruptEnd 1
  mov [cs:patchPhase0_%1 + 1],bx
  %if %1 > 1
    mov [cs:patchPhase1_%1 + 1],bp
    %if %1 > 2
      mov [cs:patchPhase2_%1 + 1],si
      %if %1 > 3
        mov [cs:patchPhase3_%1 + 1],di
        pop di
      %endif
      pop si
    %endif
    pop bp
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
    sub register%[%1],ax                         ; 2
    cmp register%[%1],consolidateCycles          ; 3
    jae %%noConsolidate                          ; 2
    add register%[%1],[cs:patchCount%2_%1 + 1]   ; 7
    add cl,[cs:patchPulseWidth%2_%1 + 1]         ; 6
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
patchPulseWidth0_1:
  mov cl,99
  interruptEnd 1


interrupt8_2:
  interruptStart 2
  cmp bp,bx
  jl next1_2
  mov ax,bx
patchCount0_2:
  mov bx,9999
patchPulseWidth0_2:
  mov cl,99
  consolidates 2 0
  jmp done_2

next1_2:
  mov ax,bp
patchCount1_2:
  mov bp,9999
patchPulseWidth1_2:
  mov cl,99
  consolidates 2 1

done_2:
  interruptEnd 2


interrupt8_3:
  interruptStart 3
  cmp bp,bx
  jl next12_3
  cmp si,bx
  jl next2_3
  mov ax,bx
patchCount0_3:
  mov bx,9999
patchPulseWidth0_3:
  mov cl,99
  consolidates 3 0
  jmp done_3

next12_3:
  cmp si,bp
  jl next2_3
  mov ax,bp
patchCount1_3:
  mov bp,9999
patchPulseWidth1_3:
  mov cl,99
  consolidates 3 1
  jmp done_3

next2_3:
  mov ax,si
patchCount2_3:
  mov si,9999
patchPulseWidth2_3:
  mov cl,99
  consolidates 3 2

done_3:
  interruptEnd 3


interrupt8_4
  interruptStart 4
  cmp bp,bx
  jl next123_4
  cmp si,bx
  jl next23_4
  cmp di,bx
  jl next3_4
  mov ax,bx
patchCount0_4:
  mov bx,9999
patchPulseWidth0_4:
  mov cl,99
  consolidates 4 0
  jmp done_4

next123_4:
  cmp si,bp
  jl next23_4
  cmp di,bp
  jl next3_4
  mov ax,bp
patchCount1_4:
  mov bp,9999
patchPulseWidth1_4:
  mov cl,99
  consolidates 4 1
  jmp done_4

next23_4:
  cmp di,si
  jl next3_4
  mov ax,si
patchCount2_4:
  mov si,9999
patchPulseWidth2_4:
  mov cl,99
  consolidates 4 2
  jmp done_4

next3_4:
  mov ax,di
patchCount3_4:
  mov di,9999
patchPulseWidth3_4:
  mov cl,99
  consolidates 4 3

done_4:
  interruptEnd 4




