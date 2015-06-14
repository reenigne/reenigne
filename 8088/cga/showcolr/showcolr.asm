org 0
cpu 8086

  mov ax,6
  int 0x10
  mov al,0x1a
  mov dx,0x3d8
  out dx,al
  mov al,15
  mov dx,0x03d9
  out dx,al

  mov dx,0x3d4

  ;   0x7f Vertical Total                                        1f 7f
  mov ax,0x3f04
  out dx,ax

  ;   0x1f Vertical Total Adjust                              06
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                                    19 64
  mov ax,0x3206
  out dx,ax

  ;   0x7f Vertical Sync Position                                1c 70
  mov ax,0x3807
  out dx,ax



  mov ax,0xb800
  mov es,ax

  xor di,di
  mov cx,16
  mov ax,0

loopTop:
  push cx
  mov cx,40*6
  rep stosw

  add di,0x2000 - 80*6
  mov cx,40*6
  rep stosw

  sub di,0x2000

  pop cx

  add ax,0x1111

  loop loopTop

  hlt

