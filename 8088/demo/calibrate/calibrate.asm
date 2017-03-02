org 0x100
%include "../defaults_common.asm"

demoAPI EQU 0F8h

  ; Determine if loader is present, and abort if not

  ; First, check to see if the API is even present.
  ; If not, don't try to call anything since it will hang the system.
  xor bx,bx
  mov es,bx
  mov di,(demoAPI * 4)+2      ;check to see if our INT is empty
  cmp [word es:di],bx         ;int. vector empty?
  je  exitShort               ;abort if so
  mov ax,0700h                ;check int. vector to see if it's ours
  int demoAPI
  cmp ax,0C0DEh               ;magic cookie received?
  jne exitShort               ;abort if not
  jmp mstart
exitShort:
  jmp exitEffect
mstart:


  mov ax,1
  int 0x10

  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov si,which_cga
  xor di,di
  mov cx,40*192
  cld
  rep movsw
  mov cx,8192-40*192
  xor ax,ax
  rep stosw

  ; Mode                                                08
  ;      1 +HRES                                         0
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x08
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

  ;   0x7f Vertical Total                               3e
  mov ax,0x3e04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0206
  out dx,ax

  ;   0x7f Vertical Sync Position                       19
  mov ax,0x1a07
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
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

  mov dl,0xda

  mov cx,5*60
whichFrameLoop:

  waitForVerticalSync
  waitForNoVerticalSync

  cli
  ; During line 0-1 we set up the start address for line 2 and change the vertical total to 0x01
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0104 ; 4: Vertical total: 2 rows/frame
  out dx,ax
  mov dl,0xda                    ; 2 0 2
  waitForDisplayDisable
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x000c
  out dx,ax
  mov ax,0x500d
  out dx,ax
  mov dl,0xda                    ; 2 0 2
  waitForDisplayDisable

  ; During lines 2..197 we set up the start address for the next line
  push cx
  mov cx,98
  mov bx,0x00a0
whichRowLoop:
  waitForDisplayEnable
  waitForDisplayDisable
  waitForDisplayEnable
  mov dl,0xd4
  mov ah,bh
  mov al,0x0c
  out dx,ax
  mov ah,bl
  inc ax
  out dx,ax
  mov dl,0xda                    ; 2 0 2
  waitForDisplayDisable
  add bx,0x50
  loop whichRowLoop
  pop cx

  ; During line 198 we set up the start address for line 0 and change the vertical total to 0x3e
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x3e04 ; 4: Vertical total: 63 rows/frame
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x000c
  out dx,ax
  inc ax
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable
  sti

  loop whichFrameLoop1
  jmp whichDone
whichFrameLoop1:
  jmp whichFrameLoop
whichDone:


  mov ax,3
  int 0x10

  lockstep
  refreshOn
  initCGA 9,0,2
  sti

  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax

  mov si,screenData
  xor di,di
  mov cx,80*100
  rep movsw


  xor bp,bp  ; 1 for switch phase
  mov bh,0
resetVars:
  mov cx,0x1400  ; cl = border colour index (0..15), ch = (hsync width << 1) | phase (0..33)
  mov byte[model],0
  call setModel

  xor si,si  ; si = frames since last keypress

  mov dx,0x3da
  jmp noSwitchPhase2
rasterLoop:
  waitForDisplayEnable
  cmp bp,1
  jne noSwitchPhase
  xor bp,bp
  mov dl,0xd4
  mov ax,0x7200
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x7100
  out dx,ax
  mov dl,0xda
noSwitchPhase:

  waitForVerticalSync

  mov dl,ch

  mov ah,1
  int 0x16
  jnz gotKeypress
  jmp doneKey

gotKeypress:
  xor si,si
  mov ah,0
  int 0x16
  push ax
  xor di,di
  mov ah,2
  int 0x16
  test al,3
  jz notShifted
  mov di,shiftUpTable - upTable
notShifted:
  pop ax
  mov bh,0
  cmp al,0x0d
  jne notEnter
  jmp doneCalibration
notEnter:
  cmp al,0x20
  jne notSpace
  xor byte[model],1
  call setModel

notSpace:
  cmp ah,0x48
  jne notUp
  mov bl,ch
  mov ch,[bx+di+upTable]
  jmp doneKey
notUp:
  cmp ah,0x50
  jne notDown
  mov bl,ch
  mov ch,[bx+di+downTable]
  jmp doneKey
notDown:
  cmp ah,0x4b
  jne notLeft
  mov bl,cl
  mov cl,[bx+di+leftTable]
 jmp doneKey
notLeft:
  cmp ah,0x4d
  jne doneKey
  mov bl,cl
  mov cl,[bx+di+rightTable]
doneKey:

  xor dl,ch
  test dl,1
  jz noSwitchPhase2
  mov bp,1
noSwitchPhase2:

  push si
  mov bl,ch
  add bl,bl
  mov si,[bx+hsyncTable]
  mov di,138+14*160
  call plotCharacter

  mov bl,ch
  add bl,bl
  mov si,[bx+phaseTable]
  mov di,146+14*160
  call plotCharacter

  mov bl,cl
  add bl,bl
  mov si,[bx+borderTable]
  mov di,138+20*160
  call plotCharacter
  pop si

  mov dl,0xd9
  mov al,cl
  out dx,al
  mov dl,0xd4
  mov al,3
  mov ah,ch
  shr ah,1
  out dx,ax
  mov dl,0xda

  inc si
  cmp si,15*60
  jl noResetVars
  xor bp,bp
  xor si,si
  test ch,1
  jz resetVars2
  mov bp,1
resetVars2:
  jmp resetVars
noResetVars:
  jmp rasterLoop

doneCalibration:
  initCGA 1

  ; Set background color and hsync width values to loader API
  mov ah,3
  int demoAPI
  mov [si+25],cl  ; background color
  mov al,ch
  shr al,1
  mov [si+24],al  ; hsync width
  and ch,1
  mov [si+26],ch  ; phase
  mov al,[cs:model]
  mov [si+27],al  ; model

exitEffect:
  ret

plotCharacter:
  movsw
  movsw
  movsw
  add si, 160-6
  add di, 160-6
  movsw
  movsw
  movsw
  add si, 160-6
  add di, 160-6
  movsw
  movsw
  movsw
  add si, 160-6
  add di, 160-6
  movsw
  movsw
  movsw
  ret

setModel:
  push si
  push cx
  cmp byte[model],0
  je setOldModel
  mov si,slice_new
  mov di,39*160
  mov cx,36*80
  rep movsw
  mov si,slice_newtext
  mov di,8*160+138
  mov cx,9
  rep movsw
  add di,160-9*2
  mov cx,9
  rep movsw
  add di,160-9*2
  mov cx,9
  rep movsw
  add di,160-9*2
  mov cx,9
  rep movsw

  jmp doneSetModel
setOldModel:
  mov si,screenData + 39*160
  mov di,39*160
  mov cx,36*80
  rep movsw
  mov si,screenData + 8*160+138
  mov di,8*160+138
  mov cx,9
  rep movsw
  add si,160-9*2
  add di,160-9*2
  mov cx,9
  rep movsw
  add si,160-9*2
  add di,160-9*2
  mov cx,9
  rep movsw
  add si,160-9*2
  add di,160-9*2
  mov cx,9
  rep movsw

doneSetModel:
  pop cx
  pop si
  ret

model: db 0  ; 0 for old, 1 for new

; hsync position = 138, 8
; phase position = 146, 8
; border position= 138, 13

c0 equ screenData + 160*113
ca equ screenData + 160*101
slice_new equ screenData + 160*118
slice_newtext equ slice_new + 160*36
which_cga equ slice_newtext + 72

hsyncTable:
  dw c0, c0, c0+6, c0+6, c0+12, c0+12, c0+18, c0+18, c0+24, c0+24, c0+30, c0+30, c0+36, c0+36, c0+42, c0+42, c0+48, c0+48, c0+54, c0+54, ca, ca, ca+6, ca+6, ca+12, ca+12, ca+18, ca+18, ca+24, ca+24, ca+30, ca+30

phaseTable:
  dw c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6, c0, c0+6

borderTable:
  dw c0, c0+6, c0+12, c0+18, c0+24, c0+30, c0+36, c0+42, c0+48, c0+54, ca, ca+6, ca+12, ca+18, ca+24, ca+30

;     0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31
upTable:
  db  1, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,  0
downTable:
  db 31,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30
leftTable:
  db 15,  0,  0,  0,  0,  0,  0,  6,  7,  8,  8,  8,  8,  8,  8, 14
rightTable:
  db  6,  6,  6,  6,  6,  6,  7,  8, 14, 14, 14, 14, 14, 14, 15,  0
shiftUpTable:
  db  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 34, 25, 26, 27, 28, 29, 30, 31,  0
shiftDownTable:
  db 31,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30
shiftLeftTable:
  db 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14
shiftRightTable:
  db  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0



screenData:
