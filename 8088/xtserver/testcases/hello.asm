org 0
cpu 8086

  mov ax,cs
  mov ds,ax
  mov si,message
  mov cx,messageEnd-message
  int 0x64
  int 0x67

message:
  db "Hello, World!",10
messageEnd:

