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
loopTop:
  times 512 nop
  loop loopTop1
  jmp loopDone
loopTop1:
  jmp loopTop
loopDone:

  refreshOn
  mov cx,256*18
  rep lodsw
  sti

  print "Test complete",10

  mov ax,0x4c00
  int 0x21
