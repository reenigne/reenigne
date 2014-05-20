  %include "../defaults_com.asm"

main:
  mov ax,cs
  mov ds,ax

  print "Test starting",10

  cli
  mov cx,512
  cld
  xor si,si
loopA:
  mov al,[si]
  inc si
  loop loopA
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
  mov cx,512*18
  cld
  xor si,si
loopB:
  mov al,[si]
  inc si
  loop loopB
  sti

  mov cx,400
loopTop2:

  in al,0x00
  mov ah,al
  in al,0x00
  xchg ah,al
  printHex
  printCharacter 32

  loop loopTop2

  print "Test complete",10

  mov ax,0x4c00
  int 0x21
