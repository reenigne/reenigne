  mov dx,03b8
  mov al,029
  out dx,al

  ; Palette
  ;      1 +OVERSCAN B
  ;      2 +OVERSCAN G
  ;      4 +OVERSCAN R
  ;      8 +OVERSCAN I
  ;   0x10 +BACKGROUND I
  ;   0x20 +COLOR SEL
  mov dx,03b9
  mov al,0
  out dx,al

  mov dx,03b4

  ;   0xff Horizontal Total                             38  38  71  71  38  38  38  61
  mov ax,07000
  out dx,ax

  ;   0xff Horizontal Displayed                         28  28  50  50  28  28  28  50
  mov ax,05001
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d  2d  5a  5a  2d  2d  2d  52
  mov ax,05a02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a  0a  0a  0a  0a  0a  0a  0f
  mov ax,00a03
  out dx,ax

  ;   0x7f Vertical Total                               1f  1f  1f  1f  7f  7f  7f  19
  mov ax,01104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06  06  06  06  06  06  06  06
  mov ax,00a05
  out dx,ax

  ;   0x7f Vertical Displayed                           19  19  19  19  64  64  64  19
  mov ax,00e06
  out dx,ax

  ;   0x7f Vertical Sync Position                       1c  1c  1c  1c  70  70  70  19
  mov ax,01007
  out dx,ax

  ;   0x03 Interlace Mode                               02  02  02  02  02  02  02  02
  mov ax,00208
  out dx,ax

  ;   0x1f Max Scan Line Address                        07  07  07  07  01  01  01  0d
  mov ax,00d09
  out dx,ax

  ; Cursor Start                                        06  06  06  06  06  06  06  0b
  ;   0x1f Cursor Start                                  6   6   6   6   6   6   6  0b
  ;   0x60 Cursor Mode                                   0   0   0   0   0   0   0   0
  mov ax,00b0a
  out dx,ax

  ;   0x1f Cursor End                                   07  07  07  07  07  07  07  0c
  mov ax,00c0b
  out dx,ax

  ;   0x3f Start Address (H)                            00  00  00  00  00  00  00  00
  mov ax,0000c
  out dx,ax

  ;   0xff Start Address (L)                            00  00  00  00  00  00  00  00
  mov ax,0000d
  out dx,ax

  ;   0x3f Cursor (H)                                   00  00  00  00  00  00  00  00
  mov ax,0000e
  out dx,ax

  ;   0xff Cursor (L)                                   00  00  00  00  00  00  00  00
  mov ax,0000f
  out dx,ax

  ; Write some characters to the screen
  mov es,0b000
  mov cx,2000
  mov di,0
  mov ax,0700
loopTop:
  stosw
  inc al
  loop loopTop
  hlt

;  jmp 0f000:0e329

