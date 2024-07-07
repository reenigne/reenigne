  org 0x100
;  %include "../../defaults_common.asm"
  cpu 8086

  jmp loader

oldInterrupt10:
  dw 0, 0

int10Routine:
  sti
  cmp ah,0x0e
  jne .noIntercept
  out 0xe9,al
.noIntercept:
  jmp far [cs:oldInterrupt10]


  ; Non-resident portion
loader:
  xor ax,ax
  mov ds,ax

%macro setResidentInterrupt 2
  mov word [%1*4], %2
  mov [%1*4 + 2], cs
%endmacro

  cli

  mov ax,[0x10*4]
  mov cx,[0x10*4 + 2]

  mov [cs:oldInterrupt10],ax
  mov [cs:oldInterrupt10+2],cx

  setResidentInterrupt 0x10, int10Routine

  sti

  mov dx,(loader + 15) >> 4
  mov ax,0x3100
  int 0x21

