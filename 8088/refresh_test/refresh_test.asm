  %include "../defaults_bin.asm"

  mov ax,cs
  mov ds,ax

;  stopScreen

  print "Test starting",10
  mov cx,0x7000
outerTest:
  push cx

  cli

  mov al,TIMER1 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,cl
  out 0x41,al  ; Timer 1 rate
  mov al,ch
  out 0x41,al  ; Timer 1 rate

  mov ax,cx
  printHex
  printCharacter 10


  mov cx,0x1000
loopTop2a:
  mov bx,cx
  mov cx,0xff
loopTop3a:
  loop loopTop3a
  mov cx,bx
  loop loopTop2a

  print "Loop complete",10

  mov cx,0
segLoop:
  push cx
  mov ds,cx
  xor si,si
  cld
  mov cx,0x8000
  rep lodsw
  printCharacter '.'
  pop cx
  add cx,0x1000
  cmp cx,0xa000
  jne segLoop

  mov ax,cs
  mov ds,ax

  print "Scan complete",10

  pop cx
  add cx,0x10
  jmp outerTest

  print "Test complete",10
  complete

