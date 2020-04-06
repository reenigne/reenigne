org 0 ; x100
cpu 8086

  mov ax,3
  int 0x10

  cli
  mov ax,0xb800
  mov es,ax
  xor di,di
  mov ax,0x0700
  mov cx,80*25
  mov dx,0x3da
  mov bl,0
loopTop:
  stosw
  inc ax
  loop loopTop
loopTop2:

  waitForDisplayEnable:
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jnz waitForDisplayEnable     ; 2 0 2   jump if -DISPEN, finish if +DISPEN

  waitForVerticalSync:
    in al,dx
    test al,8
    jz waitForVerticalSync       ;         jump if not +VSYNC, finish if +VSYNC

  dec dx
  mov al,bl
  out dx,al
  inc dx
  inc bx
  jmp loopTop2
