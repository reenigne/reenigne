org 0
  mov ax,cs
  mov ds,ax
  mov si,testMessage
  mov cx,testMessageEnd - testMessage
  int 0x61
  retf

testMessage:
  db "Hello World!",10
testMessageEnd:

