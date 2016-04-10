  %include "../defaults_bin.asm"

  mov ax,cs
  mov ds,ax
  mov es,ax
  mov di,storage

  in al,0x61
  or al,3
  out 0x61,al

  mov al,0xb4
  out 0x43,al
  mov al,0x34
  out 0x42,al
  mov al,0x12
  out 0x42,al

  mov al,0x84
  out 0x43,al

  mov al,0xb4
  out 0x43,al
  mov al,0x78
  out 0x42,al
  mov al,0x56
  out 0x42,al

  in al,0x42
  mov ah,al
  in al,0x42
  xchg ah,al
  outputHex
  complete

;  mov al,0xa4
;  out 0x43,al
;  mov al,0
;  out 0x42,al

;  mov cx,100
;captureLoopTop:
;  mov al,0x84
;  out 0x43,al
;  in al,0x42
;  mov ah,al
;  in al,0x42
;  xchg ah,al
;  stosw
;  loop captureLoopTop
;
;  mov cx,100
;  mov si,storage
;outputLoopTop:
;  lodsw
;  outputHex
;  loop outputLoopTop
;
;  hlt
;  complete
;
storage:


