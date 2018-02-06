  %include "defaults_bin.asm"

  mov ax,cs
  mov ds,ax
  mov al,0
  mul al
  call dump
  mov ax,0xffff
  mov cl,10
  shr cl,cl
  call dump
  complete


dump:
  pushf
  push ds
  push ss
  push cs
  push es
  push di
  push si
  push bp
  push sp
  push bx
  push dx
  push cx
  push ax
  xor si,si
  mov cx,13
dumpLoop:
  call dumpRegister
  inc si
  loop dumpLoop
  outputCharacter 10
  ret

dumpRegister:
  push si
  push cx
  add si,si
  add si,registerNames
  mov cx,2
  outputString
  pop cx
  pop si
  outputCharacter '='
  pop bx
  pop ax
  outputHex
  outputCharacter ' '
  push bx
  ret


registerNames:
  DB 'AXCXDXBXSPBPSIDIESCSSSDS F'
