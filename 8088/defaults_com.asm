org 0x100
%include "defaults_common.asm"

%macro printCharacter 0
  call doPrintCharacter
%endmacro

%macro printHex 0
  call doPrintHex
%endMacro

%macro printString 0
  call doPrintString
%endMacro

%macro complete 0
  xor ax,ax
  int 0x21
%endmacro

  jmp main

doPrintCharacter:
  push ax
  push bx
  push cx
  push dx
  push si
  push di
  push bp
  mov dl,al
  mov ah,2
  int 0x21
  pop bp
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  ret

doPrintString:
  push ax
  push bx
  push dx
  push di
  push bp
.loop:
  lodsb
  mov dl,al
  mov ah,2
  push si
  push cx
  int 0x21
  pop cx
  pop si
  loop .loop
  pop bp
  pop di
  pop dx
  pop bx
  pop ax
  ret

doPrintHex:
  push bx
  push cx
  push dx
  push si
  push di
  push bp
  mov bx,ax
  mov dl,bh
  mov cl,4
  shr dl,cl
  push bx
  call doPrintNybble
  pop bx
  mov dl,bh
  and dl,0xf
  push bx
  call doPrintNybble
  pop bx
  mov dl,bl
  mov cl,4
  shr dl,cl
  push bx
  call doPrintNybble
  pop bx
  mov dl,bl
  and dl,0xf
  call doPrintNybble
  pop bp
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  ret

doPrintNybble:
  cmp dl,10
  jge .alphabetic
  add dl,'0'
  jmp .gotCharacter
.alphabetic:
  add dl,'A' - 10
.gotCharacter:
  mov ah,2
  int 0x21
  ret

