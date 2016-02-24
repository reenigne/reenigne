  %include "../../defaults_bin.asm"

%macro setNextStartAddress 0
    mov bl,ch                      ; 2 0 2
    mov bh,lineTable >> 8          ; 2 0 2
    mov ah,[bx]                    ; 2 1 3
    mov al,0x0d                    ; 2 0 2
    out dx,ax                      ; 1 2 3
    mov ah,[bx+0x100]              ; 4 1 5
    dec ax                         ; 1 0 1
    out dx,ax                      ; 1 2 3
    add cx,si                      ; 2 0 2
%endMacro

  ; The timing in the inner loop is critical - an interrupt could cause a very
  ; visible glitch.
  cli

  ; Put the image data in video memory
  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  mov cx,80*100
  xor di,di
  mov si,imageData
  cld
  rep movsw

 ; mov dx,0x3d9
;  mov al,1
;  out dx,al

  ; Mode                                                08
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x09
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   0
  ;      4 +OVERSCAN R                                   0
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  mov dx,0x3d9
  mov al,0
  out dx,al

  mov dx,0x3d4

  ;   0xff Horizontal Total                             38
  mov ax,0x7100
  out dx,ax

  ;   0xff Horizontal Displayed                         28
  mov ax,0x5001
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x5a02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0003
  out dx,ax

  ;   0x7f Vertical Total                               3e
;  mov ax,0x3e04
  mov ax,0x0004
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0106
  out dx,ax

  ;   0x7f Vertical Sync Position                       19
  mov ax,0x1907
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0308
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
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

  cli
  hlt

;  mov dx,0x3d9
;  mov al,2
;  out dx,al


; Lines 0..197 = 99x 2-row screens
; Line 198 = line  0 of 63-row screen (visible)
; Line 199 = line  1 of 63-row screen (visible)
;      200         2                   start of overscan
;      224        26                   start of vertical sync
;      240        42                   end of vertical sync
;      261        63                   last line of frame

  mov dl,0xda
  mov si,0  ; Increase per 2 lines in lines/256
  mov di,0  ; Frame number

frameLoop:
    ; Uncomment for no animation
;    mov si,0x200
;    mov cx,0

  ; We now have over 62 rows (3.9ms) to do per-frame changes.
  waitForVerticalSync
  waitForNoVerticalSync

;  dec dx
;  mov al,3
;  out dx,al
;  inc dx

  ; During line 0 we set up the start address for line 1 and change the vertical total to 0x00
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0004 ; 4: Vertical total: 1 row/frame
  out dx,ax
  setNextStartAddress
  mov dl,0xda                    ; 2 0 2
  waitForDisplayDisable

  ; During lines 1..198 we set up the start address for the next line
%rep 198
    waitForDisplayEnable
    mov dl,0xd4
    setNextStartAddress
    mov dl,0xda                    ; 2 0 2
    waitForDisplayDisable
%endrep

  ; During line 199 we set up the start address for line 0 and change the vertical total to 0x3e
  waitForDisplayEnable
  mov dl,0xd4
  mov cx,bp     ; Initial offset in lines/256
  mov ax,0x3e04 ; 4: Vertical total: 63 rows/frame
  out dx,ax
  setNextStartAddress
  mov dl,0xda
  waitForDisplayDisable

;  dec dx
;  mov al,4
;  out dx,al
;  inc dx

  inc si
  mov ax,si
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  add si,ax
  mov ax,si
  mov di,50
  mul di
  neg ax
  mov bp,ax
  mov dx,0x3da
  add bp,0x6400
  jmp frameLoop


;  cmp di,0x200
;  jg otherStyle
;  mov dx,4
;  mov ax,0
;  div di
;  mov si,ax
;  mov dx,0x3da
;  inc di
;
;  jmp frameLoop
;otherStyle:
;  mov si,0x400
;  sub si,di
;  inc di
;  cmp di,0x400
;  jne noReset
;  mov di,1
;noReset:
;  jmp frameLoop


align 0x100
lineTable:

%assign i 0
%rep 200
  db (i*40) & 0xff
%assign i i+1
%endrep
%rep 56
  db (200*40) & 0xff
%endrep

%assign i 0
%rep 200
  db (i*40) >> 8
%assign i i+1
%endrep
%rep 56
  db (200*40) >> 8
%endrep


imageData:

