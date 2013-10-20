  %include "../../defaults_bin.asm"

  ; Fill the entire screen area with white

  mov ax,0xb800
  mov es,ax
  mov di,0
  mov ax,0xffdb
  cld
  mov cx,0x2000
looptop:
  stosw
  loop looptop

  cli

  ; Use 80-column mode for best resolution
  ; Snow is not an issue since the CGA data doesn't change from here on

  initCGA 9, 0, 2
  mov ax,0x0004
  out dx,ax   ; Vertical total = 0
  mov ax,0x0005
  out dx,ax   ; Vertical total adjust = 0
  mov ax,0x0106
  out dx,ax   ; Vertical displayed = 1
  mov ax,0x0003
  out dx,ax   ; Horizontal sync width = 16

  ; Wait until CGA has settled down
settle:
  mov dl,0xdc
  in al,dx  ; 0x3dc: Activate light pen
  dec dx
  in al,dx  ; 0x3db: Clean light pen strobe
  mov dl,0xd4
  mov al,16
  out dx,al ; 0x3d4<-16: light pen high
  inc dx
  in al,dx  ; 0x3d5: register value
  mov ah,al
  dec dx
  mov al,17
  out dx,al ; 0x3d4<-17: light pen low
  inc dx
  in al,dx  ; 0x3d5: register value
  cmp ax,80
  jge settle

  ; No interrupts except timer
  in al,0x21
  mov [cs:oldimr],al
  mov al,0xfe
  out 0x21,al

  ; Set up PIT and speaker
  in al,0x61
  or al,3
  out 0x61,al

  mov al,TIMER2 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01
  out 0x42,al  ; Counter 2 count = 1 - terminate count quickly

  mov al,TIMER0 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,77
  out 0x40,al  ; Counter 0 count = 77 - initially use a longer period to get into sync

  ; Set up IRQ0 handler
  xor ax,ax
  mov ds,ax
  mov es,ax
  mov ax,[8*4]
  mov [cs:oldinterrupt8],ax
  mov ax,[8*4+2]
  mov [cs:oldinterrupt8+2],ax
  mov word[8*4],interrupt8first
  mov [8*4+2],cs

  ; Set up initial variables
  mov dx,0x3da
  mov si,0
  mov bx,cs
  add bx,data >> 4
  mov ds,bx
  mov cx,241664 >> 4

;   dec dx
;   mov al,1
;   out dx,al
;   inc dx

  sti
hltloop:
  hlt
  jmp hltloop


interrupt8first:
;   dec dx
;   mov al,2
;   out dx,al
;   inc dx
  in al,dx
  test al,1
  mov al,0x20
  out 0x20,al

  jnz displayDisabled
  mov word[es:8*4],interrupt8second
displayDisabled:
  iret

interrupt8second:
;   dec dx
;   mov al,4
;   out dx,al
;   inc dx
  in al,dx
  test al,1
  mov al,0x20
  out 0x20,al

  jz displayEnabled
  mov word[es:8*4],interrupt8main

  mov al,76
  out 0x40,al  ; Synchronize PIT and CGA horizontal

  mov dl,0xd4
  mov al,1
  out dx,al   ; CRTC Address = horizontal displayed
  inc dx
  lodsb
displayEnabled:
  iret

noAdjust:
  iret

interrupt8main:
 ;  mov dl,0xd9
;   dec dx
;   mov al,5
;   out dx,al
;   mov dl,0xd5

  out dx,al
  out 0x42,al
  mov al,0x20
  out 0x20,al
  lodsb
  cmp si,0x10
  jne noAdjust
  xor si,si
  inc bx
  mov ds,bx
  loop noAdjust
  ; We're out of data - restore everything

  mov al,[cs:oldimr]
  out 0x21,al         ; Restore old interrupt mask

  xor ax,ax
  mov ds,ax
  mov ax,[cs:oldinterrupt8]
  mov [8*4],ax
  mov ax,[cs:oldinterrupt8+2]
  mov [8*4+2],ax

  in al,0x61
  and al,0xfc
  out 0x61,al

  initCGA 9

  writePIT16 0, 2, 0

  pop ax
  pop ax
  popf

  sti

  ret


oldinterrupt8:
  dw 0,0
oldimr:
  db 0

  align 16

data:
