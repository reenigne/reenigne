org 0x100
%include "../../defaults_common.asm"

  mov ax,cs
  mov es,ax
  mov ds,ax

  mov di,unrolledCode

  ; Copy header

  mov cx,headerEnd-header
  mov si,header
  rep movsb

  ; Unroll

  mov cx,200
  mov bl,0x80
  mov dx,157*16
  mov bp,cs
  add bp,((unrolledCode + (headerEnd-header) + 43*200 + (footerEnd-footer) + 15)>>4) + ((157*16)>>4) + ((157*2*200)>>4)
unroll:
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
  times 5 movsw
  movsb
  inc bx         ; Increase sineTable pointer
  add dx,157*2   ; Increase mulTable pointer
  add bp,0x350   ; Increase raster segment (enough for 838 scanlines)
  loop unroll

  ; Copy footer

  mov cx,footerEnd-footer
  mov si,footer
  rep movsb

  ; Find stack segment

  mov ax,cs
  add di,15
  mov cl,4
  shr di,cl
  add ax,di
  mov [stackSeg],ax
  mov es,ax

  ; Create pixel table

  mov cx,79
  xor di,di
  mov bx,0
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

  mov cx,200
mulTableLoopY:
  mov bx,cx
  mov cx,157
mulTableLoopX:
  mov ax,157
  sub ax,cx
  mul bx
  mov dx,200
  div dx
  shl ax,1
  stosw
  loop mulTableLoopX
  mov cx,bx
  loop mulTableLoopY

  ; Find raster bars segment

  mov ax,es
  add di,15
  mov cl,4
  shr di,cl
  add ax,di
  mov [rasterSeg],ax
  mov es,ax



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


  ; Disable DRAM refresh

  cli
  mov cx,256
  rep lodsw
  refreshOff

  ; Setup screen segment

  mov ax,0xb800
  mov es,ax


  ; Do the actual effect

  mov si,transition
  call unrolledCode


  ; Restore DRAM refresh

  refreshOn
  mov cx,256*18
  rep lodsw
  sti


  ; End program

  mov ax,3
  int 0x10
  mov ax,0x4c00
  int 0x21


  ; This is the inner loop code. It's not called directory, but instead used
  ; as source data for the unroller, which creates 1000 copies with the xx
  ; filled in and the "add" instructions at the end of each line of 20.

kefrensScanline:
  db 0x2e, 0x8b, 0x5e  ; mov bx,[cs:bp+XX]
  db 0x36, 0x8b, 0xa7  ; mov sp,[ss:bx+XXXX]   mulTable is a different 157-element table per scanline

  pop di               ; 1 2
  mov al,[es:di]       ; 3 1 +WS
  pop bx               ; 1 2
  and ax,bx            ; 2 0
  pop bx               ; 1 2
  or ax,bx             ; 2 0
  stosw                ; 1 2 +WS +WS
  mov al,[es:di]       ; 3 1 +WS
  pop bx               ; 1 2
  and al,bl            ; 2 0
  or al,bh             ; 2 0
  stosb                ; 1 1 +WS

  db 0xb8              ; mov ax,XXXX
  mov ds,ax            ; 2 0
  mov al,[bp-128]      ; 3 0
  out dx,al            ; 1 1
  mov ds,cx            ; 2 0
  lodsb                ; 1 1
  out 0x42,al          ; 2 1        Total 3 (1) 3 (2) 21 (2) 11 = 43 bytes
kefrensScanline:


pixelTable3:
  dw 0x0000, 0xb975, 0x00ff, 0x000f, 0x9750, 0x0bf0
pixelTable1:
  dw 0x0000, 0xabf5, 0x00ff, 0x000f, 0xbf50, 0x0af0


header:
  mov [savedSP],sp
  mov bx,motion
frameLoop:
  mov di,1
  mov sp,[bx]
  mov bp,[bx+2]
  add bx,4

  mov al,0
  out dx,al
  inc dx
  waitForDisplayEnable
  dec dx
  mov al,15
  out dx,al
headerEnd:


footer:
  cmp byte[ending],1
  jne noTransition
  cmp si,picture
  je effectComplete

  ; Make the transition modifications
  lodsw                  ; ax = p
  xchg ax,di             ; di = p
  mov ax,0xb8            ; ax = 0x00b8
  mov word[di],ax
  mov byte[di+2],ah
  mov word[di+206],ax
  mov byte[di+208],ah
  xchg ax,di             ; di = 0x00b8
  lodsw                  ; ax = p
  xchg ax,di             ; ax = 0x00b8, di = p
  mov word[di],ax
  mov byte[di+2],ah
  mov word[di+206],ax
  mov byte[di+208],ah

noTransition:
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
  jne noKey
;  cmp byte[ending],1
;  je effectComplete
  mov byte[ending],ah
noKey:
  cmp bx,motion+4*frames
  jne noNewLoop
  mov bx,motion
noNewLoop:
  jmp frameLoop-(10*20*50+49*6)
effectComplete:
  mov sp,[savedSP]
  ret
footerEnd:

savedSP: dw 0
ending:  db 0
stackSeg: dw 0
