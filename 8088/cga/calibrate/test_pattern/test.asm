org 0x100

  mov ax,2
  int 0x10

  mov dx,0x3d8
  mov al,0x08
  out dx,al

  mov ax,0xb800
  mov es,ax
  mov ax,0x00b0
  xor di,di
  mov cx,16
loopY:
  push cx
  mov cx,16
loopX:
  stosw
  inc ah
  loop loopX
  add di,160-32
  pop cx
  loop loopY

end:
  jmp end

