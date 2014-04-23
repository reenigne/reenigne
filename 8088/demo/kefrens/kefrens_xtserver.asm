org 0
%include "../../defaults_common.asm"

frames equ 838
positions equ 157
scanlines equ 200
footerSize equ (footerEnd-footer)
headerSize equ (headerEnd-header)

  mov ax,cs
  mov es,ax
  mov ds,ax
  cli
  add ax,((unrolledCode + headerSize + 43*scanlines + footerSize + 15)>>4)
  mov ss,ax
  mov sp,0xfffe
  sti

  mov di,unrolledCode

  ; Copy header

  mov cx,headerSize
  mov si,header
  cld
  rep movsb

  ; Unroll

  mov cx,scanlines
  mov bl,0x80
  mov dx,positions*16
  mov bp,ax
  add bp,0x1000
  mov bh,0x80
unroll:
  mov si,kefrensScanline
  movsw
  movsb
  mov al,bl
  stosb
  movsw
  movsb
  mov ax,dx
  stosw
  times 10 movsw
  movsb
  mov ax,bp
  stosw
  movsw
  movsw
  mov al,bh
  stosb
  movsw
  movsw
  movsw
  add bl,17*2                   ; Increase sineTable pointer
  add dx,positions*2            ; Increase mulTable pointer
  xor bh,1                      ; Alternate raster set
  cmp bh,0x80
  jne noRasterInc
  add bp,(frames*2 + 15)>>4     ; Increase raster segment
  sub bl,(16+17)*2
noRasterInc:
  loop unroll

  ; Copy footer

  mov cx,footerSize
  mov si,footer
  rep movsb

  ; Create pixel table

  mov ax,ss
  mov es,ax
  mov cx,79
  xor di,di
  xor bx,bx
pixelTableLoop:
  mov ax,bx
  stosw
  mov si,pixelTable3
  times 3 movsw
  loop pixelTableLoop1
  jmp pixelTableLoop2
pixelTableLoop1:
  mov ax,bx
  stosw
  times 3 movsw
  inc bx
  jmp pixelTableLoop
pixelTableLoop2:
  mov cx,79
  mov di,80
pixelTableLoop3:
  mov ax,bx
  stosw
  mov si,pixelTable1
  times 3 movsw
  loop pixelTableLoop4
  jmp pixelTableLoop5
pixelTableLoop4:
  mov ax,bx
  stosw
  times 3 movsw
  inc bx
  jmp pixelTableLoop3
pixelTableLoop5:

  ; Create multiplication table

  mov cx,scanlines
mulTableLoopY:
  mov bx,cx

  mov cx,positions
mulTableLoopX3:
  mov ax,positions
  sub ax,cx
  sub ax,78
  imul bx
  mov si,scanlines
  idiv si
  add ax,78
  shl ax,1
  shl ax,1
  shl ax,1
  stosw
  loop mulTableLoopX3

  dec bx
  mov cx,positions
mulTableLoopX1:
  mov ax,positions
  sub ax,cx
  sub ax,78
  imul bx
  mov si,scanlines
  idiv si
  add ax,78
  shl ax,1
  shl ax,1
  shl ax,1
  add ax,positions*8
  stosw
  loop mulTableLoopX1

  mov cx,bx
  loop mulTableLoopY

  ; Find raster bars segment

  mov dx,es
  add dx,0x1000
  mov es,dx

  ; Create raster bars table

  mov cx,scanlines/2
  mov ax,0x1030
rasterLoopY:
  mov bx,cx

  mov cx,frames
  xor di,di
  rep stosw
  add dx,(frames*2 + 15) >> 4
  mov es,dx

  add ax,0x0101
  cmp ax,0x2040
  jne noWrap
  mov ax,0x1030
noWrap:

  mov cx,bx
  loop rasterLoopY

  ; Create music table

  mov [sampleSeg],dx
  mov es,dx
  xor di,di
  mov cx,0x8000
  mov ax,0x0101
  rep stosw

  ; Create song table

  add dx,0x1000
  mov [songSeg],dx
  xor ax,ax
  stosw

  ; Disable DRAM refresh

  cli
  mov cx,256
  rep lodsw
  lockstep


  ; Video layout:
  ;   We don't want to have to use the second 8kB so use Max Scan Line Address = 0
  ; scanlines 260..261 startup - set Vertical Total to 1
  ;             0..199 active video
  ;           200..201 set Vertical Total to 59              0.. 1
  ;           202..223 overscan bottom                       2..23
  ;           224..239 vertical sync                        24..39
  ;           240..259 overscan top                         40..59


  ; Set video mode to 2bpp

  mov dx,0x3d8
  mov al,0x0a
  out dx,al

  ; Palette
  ;      1 +OVERSCAN B
  ;      2 +OVERSCAN G
  ;      4 +OVERSCAN R
  ;      8 +OVERSCAN I
  ;   0x10 +BACKGROUND I
  ;   0x20 +COLOR SEL
  inc dx
  mov al,0x30
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             38
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         28
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               01
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0206
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x1807
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0009
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x070b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  inc ax
  out dx,ax

  ;   0x3f Cursor (H)                                   03  0x3c0 == 40*24 == start of last line
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax



  ; Set up speaker

  in al,0x61
  or al,3
  out 0x61,al

  ; Set up timer 2 for PWM

  mov al,TIMER2 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01
  out 0x42,al  ; Counter 2 count = 1 - terminate count quickly


  ; Setup initial registers and clear screen

  mov ax,0xb800
  mov es,ax
  mov cx,80
  xor ax,ax
  xor di,di
  rep stosw
  mov dx,0x03d4
  mov bp,[sampleSeg]
  mov bx,0x80


  ; Do the actual effect

  mov word[unrollPointer],unrolledCode-sineTable
  mov ax,cs
  add ax,(sineTable >> 4)
  mov [unrollPointer+2],ax
  call far [unrollPointer]


  ; Turn off speaker

  in al,0x61
  and al,0xfc
  out 0x61,al


  ; Restore DRAM refresh

  refreshOn
  mov cx,256*18
  rep lodsw
  sti


  ; End program

  int 0x67


  ; This is the inner loop code. It's not called directory, but instead used
  ; as source data for the unroller, which creates 200 copies with the XXs
  ; filled in.

kefrensScanline:
  db 0x2e, 0x8b, 0x7f  ; mov di,[cs:bx+XX]
  db 0x36, 0x8b, 0xa5  ; mov sp,[ss:di+XXXX]   mulTable is a different 157-element table per scanline

  pop di               ; 1 2
  mov al,[es:di]       ; 3 1 +WS
  pop cx               ; 1 2
  and ax,cx            ; 2 0
  pop cx               ; 1 2
  or ax,cx             ; 2 0
  stosw                ; 1 2 +WS +WS
  mov al,[es:di]       ; 3 1 +WS
  pop cx               ; 1 2
  and al,cl            ; 2 0
  or al,ch             ; 2 0
  stosb                ; 1 1 +WS

  db 0xb8              ; mov ax,XXXX
  mov ds,ax            ; 2 0
  db 0x8a, 0x47        ; mov al,[bx+XX]
  out dx,al            ; 1 1
  mov ds,bp            ; 2 0
  lodsb                ; 1 1
  out 0x42,al          ; 2 1        Total 3 (1) 3 (2) 21 (2) 4 (1) 6 = 43 bytes


pixelTable3:
  dw 0x0000, 0x9b57, 0x00ff, 0x00f0, 0x7905, 0xb00f
pixelTable1:
  dw 0x0000, 0xba5f, 0x00ff, 0x00f0, 0xfb05, 0xa00f


%macro audioOnlyScanline 0
  times 62 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al
%endmacro


header:
  mov [cs:savedSP + (unrolledCode - sineTable) + scanlines*43 - header],sp
frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al

  ; Scanline 260: set Vertical Total to 1
  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  times 50 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al

  ; Scanline 261: audio only
  audioOnlyScanline

headerEnd:

  ; Scanlines 0-199 go in here - actual effect

footer:
  ; Scanline 200: set Vertical Total to 59
  mov dl,0xd4
  mov ax,0x3b04
  out dx,ax
  times 50 nop
  mov ax,[es:di]
  mov ds,cx
  lodsb
  out 0x42,al

  ; Scanline 201: audio only - video still active
  audioOnlyScanline

  ; Scanline 202: clear addresses 0-17
  xor di,di
  xor ax,ax
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al

%rep 8
  ; Scanlines 203-210: clear addresses 18-162
  xor ax,ax
  nop
  nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al
%endrep

  ; Scanlines 211-258: audio only
%rep 48
  audioOnlyScanline
%endrep

  ; Start of scanline 259: check for ending and update

  times 6 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je effectComplete
  jmp doneKey
noKey:
  times 28 nop
doneKey:
  inc bx
  inc bx
  cmp bx,0x80+frames*2
  jne noNewLoop
  mov bx,0x80
noNewLoop:
  jmp frameLoop-43*scanlines
effectComplete:
  mov sp,[cs:savedSP + (unrolledCode - sineTable) + scanlines*43 - header]
  retf

savedSP: dw 0

footerEnd:

sampleSeg: dw 0
songSeg: dw 0
unrollPointer: dw 0, 0

  times 128 dw 0
stackEnd:

