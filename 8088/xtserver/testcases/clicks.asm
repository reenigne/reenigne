org 0
cpu 8086

  sti
  mov ax,0x40
  mov ds,ax

  in al,0x61
  and al,0xfc
  out 0x61,al


loopTop:
  push ax

  mov ax,[0x6c]
delayLoop:
  mov bx,[0x6c]
  sub bx,ax
  cmp bx,18
  jbe delayLoop

  pop ax
  xor al,2
  out 0x61,al

  jmp loopTop

