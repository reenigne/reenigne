%ifdef bin
%include "../../defaults_bin.asm"
%else
%include "../../defaults_com.asm"

main:
  mov ax,0x40
  mov ds,ax
checkMotorShutoff:
  cmp byte[0x40],0
  je noMotorShutoff
  mov byte[0x40],1
  jmp checkMotorShutoff
noMotorShutoff:

%endif

  mov ax,4
  int 0x10
;  mov dx,0x3d8
;  mov al,0x0a
;  out dx,al

  cli
  xor ax,ax
  mov ds,ax
  mov ax,[0x20]
  mov [cs:oldInterrupt8],ax
  mov ax,[0x22]
  mov [cs:oldInterrupt8+2],ax
  mov word[0x20],interrupt8temp
  mov ax,cs
  mov [0x22],ax
  mov ds,ax





  in al,0x61
  or al,0x80
  mov [port61high+1],al
  and al,0x7f
  mov [port61low+1],al

  in al,0x21
  mov [imr],al
  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
  out 0x21,al




  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate

  writePIT16 0, 2, 19912     ; Now counting down with the frame, one IRQ0 pending

  sti
  hlt

interrupt8temp:
  mov al,0x20
  out 0x20,al

  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8

  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate

  sti
  hlt

oldInterrupt8: dw 0, 0
frameCount: dw 0, 0
imr: db 0

interrupt8:
  mov al,0x20
  out 0x20,al

  mov dx,0x3d9
  %rep 3800
    out dx,al
    inc ax
  %endrep

  inc word[frameCount]
  jnz noFrameCountCarry
  inc word[frameCount+2]
noFrameCountCarry:

  in al,0x60
  xchg ax,bx
  ; Acknowledge the previous byte
port61high:
  mov al,0xcf
  out 0x61,al
port61low:
  mov al,0x4f
  out 0x61,al
  cmp bl,1
  je tearDown

  mov sp,stackTop
  sti
  hlt

tearDown:
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  xor ax,ax
  mov ds,ax
  mov ax,[cs:oldInterrupt8]
  mov [0x20],ax
  mov ax,[cs:oldInterrupt8+2]
  mov [0x22],ax

  mov ax,cs
  mov ds,ax
  mov al,[imr]
  out 0x21,al

  writePIT16 0, 2, 0

  mov ax,3
  int 0x10

  sti

  mov ax,19912
  mul word[frameCount]
  mov cx,dx
  mov ax,19912
  mul word[frameCount+2]
  add ax,cx
  adc dx,0
  mov cx,0x40
  mov ds,cx
  add [0x6c],ax
  adc [0x6e],dx
dateLoop:
  cmp word[0x6c],0x18
  jb doneDateLoop
  cmp word[0x6e],0xb0
  jb doneDateLoop
  mov byte[0x70],1
  sub word[0x6c],0xb0
  sbb word[0x6e],0x18
  jmp dateLoop
doneDateLoop:
exit:
  mov ax,0x4c00
  int 0x21


section .bss
data:
  resw 128
stackTop:

