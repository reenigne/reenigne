  %include "../defaults_com.asm"

  cli
  xor ax,ax
  mov ds,ax
  getInterrupt 9, oldInterrupt9
  setInterrupt 9, interrupt9

  mov ax,cs
  mov ds,ax


  ; Set mode.
  mov ax,4
  int 0x10

  ; Mode                                                0a
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         2
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x0a
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   0
  ;      4 +OVERSCAN R                                   0
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                           0x71  == 912*2/16 - 1
  mov ax,0x7100
  out dx,ax

  ;   0xff Horizontal Displayed                       0x28  == 40
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                   0x2d  == 45
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                      0x0a  == 10
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                             0x3f  == 64 - 1  (CRTC total scanlines == 64*2 + 3 == 131 == 262/2)
  mov ax,0x3f04
  out dx,ax

  ;   0x1f Vertical Total Adjust                      0x03
  mov ax,0x0305
  out dx,ax

  ;   0x7f Vertical Displayed                         0x32  == 50  (CRTC displayed scanlines == 50*2 == 100)
  mov ax,0x3206
  out dx,ax

  ;   0x7f Vertical Sync Position                     0x38  == 56
  mov ax,0x3807
  out dx,ax

  ;   0x03 Interlace Mode                             0x02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                      0x01  == 2 - 1
  mov ax,0x0109
  out dx,ax

  ; Cursor Start                                      0x1f
  ;   0x1f Cursor Start                               0x1f
  ;   0x60 Cursor Mode                                0x00
  mov ax,0x1f0a
  out dx,ax

  ;   0x1f Cursor End                                 0x1f
  inc ax
  out dx,ax

  ;   0x3f Start Address (H)                          0x00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                          0x00
  inc ax
  out dx,ax

  ;   0x3f Cursor (H)                                 0x00
  inc ax
  out dx,ax

  ;   0xff Cursor (L)                                 0x00
  inc ax
  out dx,ax


  ; Copy data to video memory
  mov ax,0xb800
  mov es,ax
  mov si,videoData
  xor di,di
  mov cx,0x2000
  rep movsw


  mov dx,0x3da
  sti
frameLoop:
  waitForVerticalSync
  cli

  mov dl,0xd4

  ;   0x3f Start Address (H)                          0x00
  mov ax,0x000c
  out dx,ax
  ;   0xff Horizontal Sync Position                   0x2d  == 45
  mov ax,0x2d02
  out dx,ax

  mov dl,0xd8
  mov al,0x0a  ; INSERT MODE REGISTER VALUE FOR FIELD 0 HERE
  out dx,al
  inc dx
  mov al,0x00  ; INSERT PALETTE REGISTER VALUE FOR FIELD 0 HERE
  out dx,al

  mov dl,0xda
  sti
  waitForNoVerticalSync
  waitForVerticalSync
  cli

  mov dl,0xd4

  ;   0x3f Start Address (H)                          0x08  (start address for second page == CRTC address 0x800 == CPU address 0x1000)
  mov ax,0x080c
  out dx,ax
  ;   0xff Horizontal Sync Position                   0x66  == 45 + 912/16
  mov ax,0x2d02
  out dx,ax

  mov dl,0xd8
  mov al,0x0a  ; INSERT MODE REGISTER VALUE FOR FIELD 1 HERE
  out dx,al
  inc dx
  mov al,0x00  ; INSERT PALETTE REGISTER VALUE FOR FIELD 1 HERE
  out dx,al

  mov al,[lastScanCode]
  mov byte[lastScanCode],0
  cmp al,1                   ; Press ESC to end program
  je finished

  mov dl,0xda
  sti
  waitForNoVerticalSync
  jmp frameLoop

finished:
  mov ax,3
  int 0x10
  xor ax,ax
  mov ds,ax
  restoreInterrupt 9, oldInterrupt9
  sti
  ret


interrupt9:
  push ax
  in al,0x60
  mov [lastScanCode],al
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  mov al,0x20
  out 0x20,al
  pop ax
  iret



oldInterrupt9:
  dw 0,0

lastScanCode:
  db 0


videoData:
  // Append video memory data here.
  // 0x4000 bytes, 80 bytes per line, left to right, 2bpp, MSB on left
  // 0x0000-0x0f9f: even lines on first field
  // 0x0fa0-0x0fff: unused
  // 0x1000-0x1f9f: even lines on second field
  // 0x1fa0-0x1fff: unused
  // 0x2000-0x2f9f: odd lines on first field
  // 0x2fa0-0x2fff: unused
  // 0x3000-0x3f9f: odd lines on second field
  // 0x3fa0-0x3fff: unused
