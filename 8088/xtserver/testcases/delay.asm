
org 0
cpu 8086

  mov ax,cs
  mov ds,ax
  mov si,message
  mov cx,messageEnd-message
  int 0x64

  sti
  mov ax,0x40
  mov ds,ax

  mov ax,[0x6c]
delayLoop:
  mov bx,[0x6c]
  sub bx,ax
  cmp bx,546
  jbe delayLoop

  mov ax,cs
  mov ds,ax
  mov si,doneMessage
  mov cx,doneMessageEnd-doneMessage
  int 0x64

  int 0x67

message:
  db "Delaying 30 seconds",10
messageEnd:

doneMessage:
  db "Complete",10
doneMessageEnd:

