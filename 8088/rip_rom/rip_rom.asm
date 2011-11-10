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
  int 0x62
  mov al,0
  jmp notEOF
notZero:
  cmp al,26
  jne notEOF
  mov al,0
  int 0x62
  mov al,1
notEOF:
  int 0x62
  loop loopTop

  retf
