  %include "../defaults_com.asm"

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

;  printCharacter '-'
;  in al,0x00
;  mov ah,al
;  in al,0x00
;  xchg ah,al
;  printHex
;  printCharacter 10

  loop loopTop1
  jmp loopDone
loopTop1:
  jmp loopTop
loopDone:



;  mov cx,0x1000
;loopTop2a:
;  mov bx,cx
;  mov cx,0xff
;loopTop3a:
;  loop loopTop3a
;  mov cx,bx
;
;  printCharacter '-'
;  in al,0x00
;  mov ah,al
;  in al,0x00
;  xchg ah,al
;  printHex
;  printCharacter 10
;
;  loop loopTop2a


;  print "Loop complete",10

  refreshOn
  mov cx,256*18
  rep lodsw
  sti

;  mov cx,0x1000
;loopTop2:
;  mov bx,cx
;  mov cx,0xff
;loopTop3:
;  loop loopTop3
;
;  printCharacter '+'
;  in al,0x00
;  mov ah,al
;  in al,0x00
;  xchg ah,al
;  printHex
;  printCharacter 10

  mov cx,bx
  loop loopTop2

  print "Test complete",10

  mov ax,0x4c00
  int 0x21
