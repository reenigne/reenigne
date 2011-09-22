  in al,061
  or al,3
  out 061,al
  mov al,0b6
  out 043,al
  mov cx,0ffff
loopTop:
  mov al,cl
  out 042,al
  mov al,ch
  out 042,al
  loop loopTop
  hlt

