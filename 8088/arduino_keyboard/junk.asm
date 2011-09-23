  mov es,0b000
  mov cx,4000
  mov di,0
loopTop:
  mov al,cl
  stosb
  loop loopTop
  hlt
