%include "../../defaults_bin.asm"

  mov ax,3
  int 0x10

  lockstep
  refreshOn
;  initCGA 0x1b,0x0f,2
  initCGA 0x1a,0x0f,2
  sti

  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  xor di,di
  cld
  mov cx,8192
  rep movsw


  xor di,di
  mov cx,100
  mov ax,0xfffe
loopTop:
  mov [es:di],ax
  mov [es:di+78],ax
  mov [es:di+0x2000],ax
  mov [es:di+78+0x2000],ax

  mov [es:di+2],ax
  mov [es:di+76],ax
  mov [es:di+2+0x2000],ax
  mov [es:di+76+0x2000],ax

  rol ax,1
  add di,80
  loop loopTop



  xor bp,bp  ; 1 for switch phase
  mov bh,0
resetVars:
  mov cx,0x0000  ; cl = border colour index (0..15), ch = (hsync width << 1) | phase (0..33)
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
;  out dx,ax
  mov dl,0xda
  waitForDisplayDisable
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x7100
;  out dx,ax
  mov dl,0xda
noSwitchPhase:

  waitForVerticalSync

  mov dl,0xd8
  mov al,8 ;9
  out dx,al
  mov dl,0xda
%rep 25
  waitForDisplayEnable
  waitForDisplayDisable
%endrep

  mov dl,0xd8
  mov al,0x0a ;0x0b
  out dx,al
  mov dl,0xda
%rep 25
  waitForDisplayEnable
  waitForDisplayDisable
%endrep

  mov dl,0xd8
  mov al,0x18 ; 0x19
  out dx,al
  mov dl,0xda
%rep 25
  waitForDisplayEnable
  waitForDisplayDisable
%endrep

  mov dl,0xd8
  mov al,0x1a ;0x1b
  out dx,al
  mov dl,0xd9
  mov al,0x0f
  out dx,al
  mov dl,0xda
%rep 25
  waitForDisplayEnable
  waitForDisplayDisable
%endrep

  mov dl,0xd9
  mov al,0x00
  out dx,al
  mov dl,0xda



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
;  out dx,ax
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
;  mov si,slice_new
;  mov di,39*160
;  mov cx,36*80
;  rep movsw
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
;  mov si,screenData + 39*160
;  mov di,39*160
;  mov cx,36*80
;  rep movsw
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
