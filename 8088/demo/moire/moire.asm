cpu 8086
org 0x100

  ; Create copy of picture shifted over by 1 pixel

  ; Fill character is 0xdd which has foreground on left
  ; Low nybble is foreground => low nybble on left

  ; picture at "picture":
  ;   bytes 0xNM 0xQP 0xVU ...
  ;   pixels M N P Q U V ...

  ; picture at "pictureEnd":
  ;   bytes 0xPN 0xUQ ...
  ;   pixels N P Q U V ...

  mov ax,cs
  mov es,ax
  mov ds,ax
  mov di,pictureEnd
  mov si,picture
  lodsb       ; al = 0xNM
  mov cl,4
  mov bl,al   ; bl = 0xNM
shiftLoop:
  lodsb       ; al = 0xQP
  mov bh,al   ; bh = 0xQP
  mov ax,bx   ; ax = 0xQPNM
  shr ax,cl   ; ax = 0x0QPN
  stosb
  cmp si,pictureEnd
  jne shiftLoop
  mov [picture2End],di


  ; Compute location for first unroll

  add di,15
  shr di,cl   ; 4 from above
  mov bx,cs
  add di,bx
  mov es,di
  mov [unrolled1Seg],di
  xor di,di
  mov bx,picture


  ; First unroll

  mov cx,pictureEnd - picture
copyUnroll1:
  mov si,moire4
  movsw
  movsw
  mov ax,[bx]
  inc bx
  inc bx
  stosw
  movsw
  movsw
  movsb
  loop copyUnroll1


  ; Compute location for second unroll

  add di,15
  mov cl,4
  shr di,cl
  mov bx,es
  add di,bx
  mov es,di
  mov [unrolled2Seg],di
  xor di,di
  mov bx,pictureEnd


  ; Second unroll

  mov cx,pictureEnd - picture
copyUnroll2:
  mov si,moire4
  movsw
  movsw
  mov ax,[bx]
  inc bx
  inc bx
  stosw
  movsw
  movsw
  movsb
  loop copyUnroll2


  ; Set video mode

  mov dx,0x3d8
  mov al,0x08     ; 40-column text mode, colour
  out dx,al
  inc dx
  mov al,0        ; Black border
  out dx,al
  mov dl,0xd4
  mov ax,0x3800
  out dx,ax
  mov ax,0x2801
  out dx,ax
  mov ax,0x2d02
  out dx,ax
  mov ax,0x0a03
  out dx,ax
  mov ax,0x4004
  out dx,ax
  mov ax,0x0205
  out dx,ax
  mov ax,0x3206
  out dx,ax
  mov ax,0x3807
  out dx,ax
  mov ax,0x0208
  out dx,ax
  mov ax,0x0309
  out dx,ax
  mov ax,0x060a
  out dx,ax
  mov ax,0x070b
  out dx,ax
  mov ax,0x000c
  out dx,ax
  inc ax
  out dx,ax


  ; Fill screen

  mov ax,0xb800
  mov es,ax
  mov ax,0x00dd
  mov cx,40*50
  rep stosw


  ; Frame loop

  mov dx,0x3da
  mov cx,[frames]
  mov bx,motion
frameLoop:

  ; Sync with raster beam vertically

waitForVerticalSync:
  in al,dx
  test al,8
  jz waitForVerticalSync
waitForNoVerticalSync:
  in al,dx
  test al,8
  jnz waitForNoVerticalSync

  ;  Set up code segment for unrolled loop

  mov si,[bx]
  mov ax,[unrolled1Seg]
  test si,0x8000
  jz noUnroll2
  mov ax,[unrolled2Seg]
noUnroll2:

  ; Patch CALL instruction with destination

  mov [callInstruction+3],ax
  mov es,ax
  mov di,[bx+2]
  add bx,4
  mov [callInstruction+1],di

  ; Patch in RETF

  add di,11*20*50
  mov byte[es:di],0xcb
  mov [patchOff],di
  mov [patchSeg],es

  ; Setup initial DI and clear flag bits from SI

  xor di,di
  test si,0x4000
  jz noOddDI
  mov di,0x3fff
noOddDI:
  and si,0x3fff

  ; Setup screen segment

  mov ax,0xb800
  mov es,ax

  ; Do the actual call into the unrolled code

callInstruction:
  call 0:0

  ; Patch LODSW back in

  les di,[patchOff]
  mov byte[es:di],0xad

  ; End of frame

  loop frameLoop

  ; Effect complete

  mov ax,3
  int 0x10
  ret


moire4:
  lodsw
  xor ax,9999
  stosb
  mov al,ah
  inc di
  stosb
  inc di
moire4End:

picture2End:  dw 0
unrolled1Seg: dw 0
unrolled2Seg: dw 0
patchOff:     dw 0
patchSeg:     dw 0

