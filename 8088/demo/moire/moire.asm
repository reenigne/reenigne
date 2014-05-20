org 0x100
%include "../../defaults_common.asm"

  mov ax,cs
  mov es,ax
  mov ds,ax

  ; Create copy of picture shifted over by 1 pixel

  ; Fill character is 0xdd which has foreground on left
  ; Low nybble is foreground => low nybble on left

  ; picture at "picture":
  ;   bytes 0xNM 0xQP 0xVU ...
  ;   pixels M N P Q U V ...

  ; picture at "pictureEnd":
  ;   bytes 0xPN 0xUQ ...
  ;   pixels N P Q U V ...

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
  mov bl,bh   ; bl = 0xQP
  cmp si,pictureEnd
  jne shiftLoop


  ; Copy header

  mov cx,headerEnd-header
  mov si,header
  rep movsb

  ; Unroll

  mov bx,47
unrollY:
  mov cx,20
  mov al,0
unrollX:
  mov si,moire4
  movsw
  movsb
  stosb
  add al,2
  movsw
  movsw
  movsw
  loop unrollX
  dec bx
  jz doneUnroll
  movsw
  movsw
  movsw
  jmp unrollY
doneUnroll:

  ; Copy footer

  mov cx,footerEnd-footer
  mov si,footer
  rep movsb



  ; Set video mode to 40-column text mode, black border, 50 rows

  initCGA 8, 0, 4


  ; Fill screen with left half-block character

  mov ax,0xb800
  mov es,ax
  mov ax,0x00dd
  mov cx,40*50
  xor di,di
  rep stosw


  ; Sync with raster beam vertically

  cli
  mov dx,0x3da
  waitForVerticalSync
  waitForNoVerticalSync
  dec dx


  ; Send read interrupt request command to PIC

  mov al,0x0a
  out 0x20,al


  ; Setup screen segment

  mov ax,0xb800
  mov es,ax


  ; Do the actual effect

  mov si,transition
  call pictureEnd+pictureEnd-picture-1


  ; Finish up

  sti
  mov ax,3
  int 0x10
  mov ax,0x4c00
  int 0x21


  ; This is the inner loop code. It's not called directory, but instead used
  ; as source data for the unroller, which creates 1000 copies with the xx
  ; filled in and the "add" instructions at the end of each line of 20.

moire4:
  pop ax
  db 0x33, 0x46  ; xor ax,[bp+xx]
;  db 0x8b, 0x46  ; mov ax,[bp+xx]
  stosb
  inc di
  mov al,ah
  stosb
  inc di
  add sp,stride-40
  add bp,stride
moire4End:


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
  cmp di,pictureEnd+(pictureEnd-picture)+(headerEnd-header)+(10*20+6)*46
  jge over1
  mov word[di+206],ax
  mov byte[di+208],ah
over1:
  xchg ax,di             ; di = 0x00b8
  lodsw                  ; ax = p
  xchg ax,di             ; ax = 0x00b8, di = p
  mov word[di],ax
  mov byte[di+2],ah
  cmp di,pictureEnd+(pictureEnd-picture)+(headerEnd-header)+(10*20+6)*46
  jge over2
  mov word[di+206],ax
  mov byte[di+208],ah
over2:

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
  cmp byte[ending],1
  je effectComplete
  mov byte[ending],ah
noKey:
  cmp bx,motion+4*frames
  jne noNewLoop
  mov bx,motion
noNewLoop:
  jmp frameLoop-((10*20+6)*47-6)
effectComplete:
  mov sp,[savedSP]
  ret
footerEnd:

savedSP: dw 0
ending:  db 0
