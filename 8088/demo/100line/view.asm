org 0x100
cpu 8086

%include "../defaults_common.asm"

  ; Copy data
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov cx,8000
  mov si,data
  xor di,di
  cld
  rep movsw

  initCGA 9,0,2

  mov dx,0x03d4

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0003
  out dx,ax

  mov ah,0
  int 0x16
  mov ax,3
  int 0x10
  mov ax,0x4c00
  int 0x21

data:
