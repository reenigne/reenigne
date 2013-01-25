
org 0
cpu 8086

  mov ax,cs
  mov ds,ax
  mov si,message
  mov cx,messageEnd-message
  int 0x64

  mov al,'A'
  int 0x65

  sti

  mov al,'B'
  int 0x65

  mov ax,0x40
  mov ds,ax

  mov ax,[0x6c]
delayLoop:
  mov bx,[0x6c]
  sub bx,ax
  cmp bx,91
  jbe delayLoop

  mov al,'C'
  int 0x65

  int 0x60

  mov al,'D'
  int 0x65

  mov ax,[0x6c]
delayLoop2:
  mov bx,[0x6c]
  sub bx,ax
  cmp bx,91
  jbe delayLoop2

  mov al,'E'
  int 0x65

  mov ax,cs
  mov ds,ax
  mov si,doneMessage
  mov cx,doneMessageEnd-doneMessage
  int 0x64

  mov al,'F'
  int 0x65

  int 0x67

message:
  db "Delaying 5 seconds",10
messageEnd:

doneMessage:
  db "Complete",10
doneMessageEnd:

