org 0

  ; Disable NMI
  xor al,al
  out 0xa0,al

  mov bx,0x4000
segmentLoop:
  mov es,bx
  xor si,si
offsetLoop:
  mov ah,0xff
  mov byte[es:si],ah
  mov al,byte[es:si]
  cmp al,ah
  jz okHigh

  printHex
  mov ax,es
  printHex
  mov ax,si
  printHex
  printNewLine

okHigh:
  mov ah,0
  mov byte[es:si],ah
  mov al,byte[es:si]
  cmp al,ah
  jz okLow

  printHex
  mov ax,es
  printHex
  mov ax,si
  printHex
  printNewLine

okLow:
  add si,1
  jnc offsetLoop

  mov ax,bx
  printHex
  printNewLine

  add bx,0x1000
  cmp bx,0xa000
  jne segmentLoop
  retf

