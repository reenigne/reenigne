  %include "../defaults_com.asm"

main:
  mov dx,0x3da
  waitForVerticalSync
  dec dx
  mov al,15
  out dx,al

  in al,0x61
  or al,3
  out 0x61,al

  mov al,0xb6
  out 0x43,al
  mov al,76
  out 0x42,al
  mov al,0
  out 0x42,al

  inc dx
  waitForNoVerticalSync
  waitForVerticalSync
  dec dx
  mov al,0
  out dx,al

  in al,0x61
  and al,0xfc
  out 0x61,al

  ret
