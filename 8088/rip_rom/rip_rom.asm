cpu 8086
org 0

  mov ax,0xe000
  mov ds,ax
  mov cx,0
  cld

loopTop:
  lodsb
  cmp al,0
  jne notZero
  printCharacter
  mov al,0
  jmp notEOF
notZero:
  cmp al,26
  jne notEOF
  printCharacter 0
  mov al,1
notEOF:
  printCharacter
  loop loopTop

  retf
