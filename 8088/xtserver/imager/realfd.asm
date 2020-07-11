  org 0x100
  %include "../../defaults_common.asm"

  xor ax,ax
  mov es,ax
  mov di,0x13*4
  mov ax,0xec59
  stosw
  mov ax,0xf000
  stosw

;  mov di,0x0e*4
;  mov ax,0xef57
;  stosw
;  mov ax,0xf000
;  stosw
;
;
;  in al,0x21
;  and al,0xfb
;  out 0x21,al

  mov ah,0
  mov dl,0
  int 0x13

;  and byte[0x410],

  ret

