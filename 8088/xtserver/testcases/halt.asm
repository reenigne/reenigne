
org 0
cpu 8086

  mov ax,cs
  mov ds,ax
  mov si,message
  mov cx,messageEnd-message
  int 0x64

crash:
  jmp crash

message:
  db "Crashing",10
messageEnd:

