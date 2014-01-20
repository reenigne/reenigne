org 0x100
cpu 8086

  jmp loader

newInt10:
  cmp ax,0x13
  jne notMode13

  pushf
  call far[cs:oldInt10]
  mov dx,0x03d4
  mov ax,0x0100
  out dx,ax        ; Reset sequencer
  mov ax,0x0901
  out dx,ax        ; Set double pixel width
  mov dl,0xc2
  mov al,0x27
  out dx,al        ; Set 28.322 MHz clock, polarity for 200 lines
  mov dl,0xc4
  mov ax,0x0300
  out dx,ax        ; Unreset sequencer
  mov dl,0xd4
  mov ax,0x0e11
  out dx,ax        ; Unprotect CRTC registers
  mov ax,0x6b00
  out dx,ax        ; Horizontal Total = character 112
  mov ax,0x4f01
  out dx,ax        ; Horizontal Display Enable End = character 80
  mov ax,0x5002
  out dx,ax        ; Start Horizontal Blanking = character 81
  mov ax,0x8e03
  out dx,ax        ; End Horizontal Blanking = character 110
  mov ax,0x5e04
  out dx,ax        ; Start Horizontal Retrace = character 94
  mov ax,0x8605
  out dx,ax        ; End Horizontal Retrace = character 102
  mov ax,0x0506
  out dx,ax        ; Vertical total = scanline 262
  mov ax,0x0107
  out dx,ax        ; Overflows all 0 except Vertical Total
  mov ax,0x4009
  out dx,ax        ; 1 scanline per vertical pixel
  mov ax,0xe010
  out dx,ax        ; Vertical Retrace Start = scanline 224
  mov ax,0x8311
  out dx,ax        ; Vertical Retrace End = scanline 227, CRTC protected
  mov ax,0xc712
  out dx,ax        ; Vertical Display Enable End = scanline 200
  mov ax,0xcb15
  out dx,ax        ; Start Vertical Blanking = scanline 203
  mov ax,0x0216
  out dx,ax        ; End Vertical Blanking = scanline 258
  iret

notMode13:
  jmp far[cs:oldInt10]

oldInt10:
  dw 0, 0

loader:
  xor ax,ax
  mov ds,ax
  cli
  mov bx,[0x10*4]
  mov [cs:oldInt10],bx
  mov word[0x10*4],newInt10
  mov ax,word[0x10*4+2]
  mov word[cs:oldInt10+2],ax
  mov [0x10*4+2],cs
  sti
  mov dx,loader
  add dx,0x0f
  mov cx,4
  shr dx,cl
  mov ax,0x3100
  int 0x21


