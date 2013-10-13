  %include "../defaults_bin.asm"

  mov dx,0x3d0
  mov cx,16
loopTop:
  in al,dx
  inc dx
  printHex
  loop loopTop

  printNewLine

  mov dx,0x3d4
  mov cx,64
  mov bl,0
loopTop2:
  mov al,bl
  out dx,al
  inc dx
  in al,dx
  dec dx
  inc bl
  printHex
  loop loopTop2

  printNewLine

  mov ax,0xb800
  mov es,ax
  mov al,[es:0x3fff]
  printHex
  mov byte[es:0x3fff],0x42

  printNewLine

  mov ax,0x8000
  mov es,ax
  mov al,[es:0]
  printHex
  mov byte[es:0],0x59

  printNewLine

  complete

