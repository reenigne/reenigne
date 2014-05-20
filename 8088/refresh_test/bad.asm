  %include "../defaults_com.asm"

main:
  mov ax,cs
  mov ds,ax

  print "Test starting",10

  cli
  mov cx,256
  rep lodsw
  refreshOff

  mov cx,0x1000
loopTop2a:
  mov bx,cx
  mov cx,0xff
loopTop3a:
  loop loopTop3a
  mov cx,bx
  loop loopTop2a


  refreshOn
  mov cx,256*18
  rep lodsw
  sti


  print "Test complete",10

  mov ax,0x4c00
  int 0x21
