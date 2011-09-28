  mov dx,0x3b8
  mov al,0x29
  out dx,al

  ; Palette
  ;      1 +OVERSCAN B
  ;      2 +OVERSCAN G
  ;      4 +OVERSCAN R
  ;      8 +OVERSCAN I
  ;   0x10 +BACKGROUND I
  ;   0x20 +COLOR SEL
  mov dx,0x3b9
  mov al,0
  out dx,al

  mov dx,0x3b4

  ;   0xff Horizontal Total                             38  38  71  71  38  38  38  61
  mov ax,0x7000
  out dx,ax

  ;   0xff Horizontal Displayed                         28  28  50  50  28  28  28  50
  mov ax,0x5001
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d  2d  5a  5a  2d  2d  2d  52
  mov ax,0x5a02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a  0a  0a  0a  0a  0a  0a  0f
  mov ax,0x3a03
  out dx,ax

  ;   0x7f Vertical Total                               1f  1f  1f  1f  7f  7f  7f  19
  mov ax,0x1104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06  06  06  06  06  06  06  06
  mov ax,0x0a05
  out dx,ax

  ;   0x7f Vertical Displayed                           19  19  19  19  64  64  64  19
  mov ax,0x0e06
  out dx,ax

  ;   0x7f Vertical Sync Position                       1c  1c  1c  1c  70  70  70  19
  mov ax,0x1007
  out dx,ax

  ;   0x03 Interlace Mode                               02  02  02  02  02  02  02  02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        07  07  07  07  01  01  01  0d
  mov ax,0x0d09
  out dx,ax

  ; Cursor Start                                        06  06  06  06  06  06  06  0b
  ;   0x1f Cursor Start                                  6   6   6   6   6   6   6  0b
  ;   0x60 Cursor Mode                                   0   0   0   0   0   0   0   0
  mov ax,0x0b0a
  out dx,ax

  ;   0x1f Cursor End                                   07  07  07  07  07  07  07  0c
  mov ax,0x0c0b
  out dx,ax

  ;   0x3f Start Address (H)                            00  00  00  00  00  00  00  00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00  00  00  00  00  00  00  00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   00  00  00  00  00  00  00  00
  mov ax,0x000e
  out dx,ax

  ;   0xff Cursor (L)                                   00  00  00  00  00  00  00  00
  mov ax,0x000f
  out dx,ax

  ; Write some characters to the screen
  mov ax,0xb000
  mov es,ax
  mov cx,2000
  mov di,0
  mov ax,0x700
loopTop:
  stosw
  inc al
  loop loopTop

  ; Make some noise on the speaker
  in al,0x61
  or al,3
  out 0x61,al
  mov al,0xb6
  out 0x43,al
  mov cx,0xffff
chirpLoopTop:
  mov al,cl
  out 0x42,al
  mov al,ch
  out 0x42,al
  loop chirpLoopTop
  hlt

;  jmp 0xf000:0xe329

