  org 0x100

  mov ax,4
  int 0x10
  xor ax,ax
  mov ds,ax
  mov si,8*4
  cli
  mov word[si],interrupt8
  mov [si+2],cs

  mov dx,0x3da
waitForDisplayEnable:
  in al,dx
  test al,1
  jnz waitForDisplayEnable

  mov al,0x34
  out 0x43,al
  mov al,19912 & 0xff
  out 0x40,al
  mov al,19912 >> 8
  out 0x40,al
  sti

loopTop:
  hlt
  jmp loopTop

interrupt8:
  push ax
  push dx
  mov dx,0x3d9
  mov al,1
  out dx,al
  times 76 nop
  mov al,0
  out dx,al

  mov ax,0x20
  out 0x20,al
  pop dx
  pop ax
  iret

