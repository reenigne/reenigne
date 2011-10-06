  in al,0x61
  or al,3
  out 0x61,al
  mov al,0xb6
  out 0x43,al
  mov cx,0xffff
chirpLoopTop:
  mov al,cl
  out 0x42,al
  mov al,ch
  out 0x42,al
  loop chirpLoopTop
  retf
