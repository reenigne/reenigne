org 0 ;x100
cpu 8086

  in al,0x61
  and al,0xfc
  mov bl,al
  or bl,2
  mov bh,0
  mov al,0x90
  out 0x43,al

l:
  inc bh
  mov cl,bh
  and cl,3
  mov al,cl
  or al,bl
  out 0x61,al
  mov al,bh
  shr al,1
  shr al,1
  out 0x42,al
  jmp l

;l:
;  inc bh
;  mov cl,bh
;  rol cl,1
;  rol cl,1
;  and cl,3
;  mov al,cl
;  or al,bl
;  out 0x61,al
;  mov al,bh
;  and al,0x3f
;  out 0x42,al
;  jmp l

