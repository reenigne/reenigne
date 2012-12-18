%include "../../defaults_bin.asm"

  initCGA 9

clear:
  ; Clear screen to white
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov ax,0x0fdb
  mov cx,80*25
  xor di,di
  cld
  rep stosw

loopTop:

  ; Wait for strobe
  mov dx,0x3da
waitLoop:
  in al,dx
  test al,2
  jz waitLoop

  ; Read light pen position:
  mov dl,0xd4
  mov al,0x10
  out dx,al
  inc dx
  in al,dx
  mov bh,al
  dec dx
  mov al,0x11
  out dx,al
  inc dx
  in al,dx
  mov bl,al    ; Light pen character position now in BX
  add bx,bx    ; Multiply by 2 to get memory position (offset from 0xb8000)

  ; Reset strobe
  mov dl,0xdb
  out dx,al

  ; Update screen at light pen position:
  mov byte[bx],0x20

  cmp bx,0
  je clear

;  mov ax,bx
;  mov dx,0
;  mov bx,160
;  div bx
;
;  mov cx,ax  ; quotient = y position
;
;  mov ah,0
;  mov al,dl  ; remainder = x position
;  mov bl,10
;  div bl
;  add ax,0x3030
;  mov byte[0],al  ; quotient  = x 10s
;  mov byte[2],ah  ; remainder = x 1s
;
;  mov ah,0
;  mov al,cl
;  div bl
;  add ax,0x3030
;  mov byte[4],al  ; quotient  = y 10s
;  mov byte[6],ah  ; quotient  = y 1s

  mov dx,0x3da
  waitForVerticalSync
  waitForNoVerticalSync

  jmp loopTop
