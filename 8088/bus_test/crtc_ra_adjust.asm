  %include "../defaults_bin.asm"

  mov ax,0xb800
  mov es,ax
  mov di,0
  mov ax,0x0700
  cld
  mov cx,0x2000
looptop:
  stosw
  inc al
  loop looptop

  mov dx,0x3d4
  mov ax,0x4006
  out dx,ax

  captureScreen

  mov ax,0x40
  mov ds,ax
  mov ax,[0x6c]
delayLoop:
  mov bx,[0x6c]
  sub bx,ax
  cmp bx,91
  jbe delayLoop

  complete

