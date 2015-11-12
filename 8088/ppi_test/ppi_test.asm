  %include "../defaults_bin.asm"

  mov ax,1
  int 0x10

  mov ax,0xb800
  mov es,ax
  xor di,di

  mov cx,0x100
loopTop:
  mov al,0
  sub al,cl
  out 0x63,al
  mov al,cl
  xor al,0x55
  out 0xe0,al
  in al,0x63
  mov ah,al
  mov al,0x99
  out 0x63,al

  mov al,0x48
  out 0x61,al

%rep 10
  mov al,0x08
  out 0x61,al
  mov al,0x48
  out 0x61,al
%endrep

  mov al,0xc8
  out 0x61,al
  and al,0x48
  out 0x61,al

;  mov al,ah
;  outputHex
;  mov al,0x20
;  outputCharacter



  mov bl,ah

  mov al,bl
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  and al,15
  cmp al,9
  jg af1
  add al,'0'
  jmp done1
af1:
  add al,'A'-10
done1:
  mov ah,7
  stosw

  mov al,bl
  and al,15
  cmp al,9
  jg af2
  add al,'0'
  jmp done2
af2:
  add al,'A'-10
done2:
  mov ah,7
  stosw



  loop loopTop2

stop:
  jmp stop

loopTop2:
  jmp loopTop


; 5c


data:
