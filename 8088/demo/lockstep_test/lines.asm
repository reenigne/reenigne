  %include "../defaults_bin.asm"

  lockstep

  ; Fill video memory with 0s
  mov cx,0x2000
  mov ax,0x0000
  xor di,di
  rep stosw

  initCGA 0x0a

  ; Set DX to palette register port address
  mov dl,0xd9

  ; Fill prefetch queue
  mov al,0
  mov cl,0
  mul cl

  ; Do pattern

;%macro scanLineSegment 0  ; 15 (measured)
;    inc ax
;    out dx,al
;%endmacro
;
;%macro scanLine 0           ;         304
;  %rep 20
;    scanLineSegment         ; 20*15 = 300
;  %endrep
;    sahf                    ;           4
;%endmacro
;
;pattern:                    ;           79648
;%rep 261
;  scanLine                  ; 261*304 = 79344
;%endrep
;%rep 12
;  scanLineSegment           ; 12*15 =     180
;%endrep
;  times 4 sahf
;  mov al,3
;  mov cl,0
;  mul cl
;  jmp pattern               ;              24

  mov cl,1

pattern:
  %rep 261
    inc ax
    out dx,al
    mov bl,al
    mov al,0x01
    mul cl
    mov al,bl
    times 50 nop
  %endrep
  mov al,0x03
  mul cl
  times 52 nop
  jmp pattern
