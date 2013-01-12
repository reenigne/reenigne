  %include "../defaults_bin.asm"

  cli

  mov dx,0x03d8
  mov al,1
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total
  mov ax,0x0000
  out dx,ax

  ;   0xff Horizontal Displayed                         28
  mov ax,0x0101
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               7f
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           64
  mov ax,0x0106
  out dx,ax

  ;   0x7f Vertical Sync Position                       70
  mov ax,0x0007
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        01
  mov ax,0x0009
  out dx,ax

  hlt
