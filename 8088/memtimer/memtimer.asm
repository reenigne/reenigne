org 0

  mov al,0x34
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  mov ax,cs
  mov ds,ax
;  mov ax,0xb800
  mov es,ax

mainLoop:
  mov si,endCode
  mov di,endCode + 8192
  mov cx,2048

  in al,0x40
  mov ah,al
  in al,0x40
  xchg ax,dx

  rep movsw
;%rep 2048
;  movsw
;%endrep

  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  xchg dh,dl
  sub ax,dx
  neg ax
  int 0x63
  mov al,10
  int 0x65

  jmp mainLoop

align 16
endCode:

; Want 16384 IOs
; => 8192 bytes moved
