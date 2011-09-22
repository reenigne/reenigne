; Send CX bytes pointed to by DS:SI
sendLoop:
  ; Interrupts off - the timing here is critical
  cli
  ; Lower clock line to tell the Arduino we want to send data
  mov dx,061
  in al,dx
  and al,0bf
  out dx,al
  ; Wait for 1ms
  mov bx,cx
  mov cx,281   ; 4.77 c/us / (1000us * 17 c/loop) =
waitLoop:
  loop waitLoop
  ; Raise clock line again
  or al,040
  out dx,al
  ;
