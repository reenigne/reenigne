org 0 ;x100
cpu 8086

iters equ 50

  cli
  mov ax,cs
  mov ss,ax
  mov sp,0xfffe

  mov ax,3
  int 0x10


  cli
  cld

  mov al,0xff
  out 0x21,al

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  xor bx,bx

  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop1:
  mov al,0x04    ; latch counter value, channel 0, mode 2
  out 0x43,al
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  stosw
  loop loopTop1

  call dumpOutput

  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop2:
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  stosw
  loop loopTop2

  call dumpOutput

  mov al,0x14
  out 0x43,al
  mov al,0
  out 0x40,al


  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop3:
;  mov al,0x14    ; LSB only, channel 0, mode 2
;  out 0x43,al
  in al,0x40
  mov ah,0
  stosw
  loop loopTop3

  call dumpOutput

  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop4:
  in al,0x40
  mov ah,0
  stosw
  loop loopTop4

  call dumpOutput

  mov al,0x24
  out 0x43,al
  mov al,0
  out 0x40,al

  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop5:
;  mov al,0x24    ; MSB only, channel 0, mode 2
;  out 0x43,al
  in al,0x40
  mov ah,al
  mov al,0
  stosw
  loop loopTop5

  call dumpOutput

  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop6:
  in al,0x40
  mov ah,al
  mov al,0
  stosw
  loop loopTop6

  call dumpOutput

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  mov al,0
  out 0x40,al

  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop7:
;  mov al,0x34    ; LSB then MSB, channel 0, mode 2
;  out 0x43,al
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  stosw
  loop loopTop7

  call dumpOutput

  mov al,0xb4
  out 0x43,al
  mov al,0
  out 0x42,al
  mov al,0
  out 0x42,al

  mov cx,iters
  mov di,scratch
  mov ax,cs
  mov es,ax
loopTop8:
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  stosw
  loop loopTop8

  call dumpOutput

  ;ret
  cli
  hlt

dumpOutput:

  mov cx,iters
  mov ax,cs
  mov ds,ax
  mov si,scratch
  mov di,bx
  mov ax,0xb800
  mov es,ax
  xor dx,dx

outputLoop:
  lodsw
  xchg ax,dx
  sub ax,dx

  push ax
  mov al,ah
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  call outputDigit
  pop ax

  push ax
  mov al,ah
  call outputDigit
  pop ax

  push ax
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  call outputDigit
  pop ax

  call outputDigit

  mov ax,0x0720
  stosw

;  int 0x63
  loop outputLoop
  mov bx,di
  ret


outputDigit:
  mov ah,7
  and al,0x0f
  cmp al,10
  jge .alpha
  add al,'0'
  stosw
  jmp .done
.alpha:
  add al,'A'-10
  stosw
.done:
  ret


scratch:



; PIT (8253) reading experiment results:
;   If PIT is counting:
;      Latch read works fine (must read both bytes)
;      Can repeatedly read properly - don't need to output latch command each time
;      At what point in the cycle is the count latched?
;   Sending a (non-latch) mode command stops counting, so read always returns the same value
;   If last (non-latch) mode command was completed, reading works as expected.
;     However, returns current value not latched value, so the bytes of a two byte read are not synchronised.



