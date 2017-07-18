org 0x100
cpu 8086

xAcceleration        equ 0x10
yAcceleration        equ 0x10
xMaxVelocity         equ 0x100
yMaxVelocity         equ 0x100
xVelocity: dw 0
xSubTile: dw 0
yVelocity: dw 0
ySubTile: dw 0


  mov ax,cs
  mov ds,ax

  in al,0x61
  or al,3
  or al,0x80
  mov [port61high+1],al
  and al,0x7f
  mov [port61low+1],al

  in al,0x21
  mov [imr],al
  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
  out 0x21,al

  mov al,0x0a  ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 0x20,al  ;  read Interrupt Request Register


checkKey:
  in al,0x20
  and al,2    ; Check for IRR bit 1 (IRQ 1) high
  jz noKey
  ; Read the keyboard byte and store it
  in al,0x60
  xchg ax,bx
  ; Acknowledge the previous byte
port61high:
  mov al,0xcf
  out 0x61,al
port61low:
  mov al,0x4f
  out 0x61,al

;  push bx
;  mov ah,0xe
;  mov al,bl
;  xor bx,bx
;  int 0x10
;  pop bx


  mov dx,0x3da
waitForNoVerticalSync:
  in al,dx
  test al,8
  jnz waitForNoVerticalSync
waitForVerticalSync:
  in al,dx
  test al,8
  jz waitForVerticalSync


  mov al,bl
  and bx,7
  mov cl,[shifts+bx]
  mov bl,al
  shr bl,1
  shr bl,1
  shr bl,1
  and bl,0x0f
  test al,0x80
  jz keyPressed
  not cl
  and [keyboardFlags+bx],cl
  jmp noKey
keyPressed:
  or [keyboardFlags+bx],cl


noKey:
  mov ax,[xVelocity]
  test byte[keyboardFlags+9],8
  jz leftNotPressed
  test byte[keyboardFlags+9],0x20
  jnz noHorizontalAcceleration
  ; Speed up leftwards
  sub ax,xAcceleration
  cmp ax,-xMaxVelocity
  jge doneHorizontalAcceleration
  mov ax,-xMaxVelocity
  jmp doneHorizontalAcceleration
leftNotPressed:
  test byte[keyboardFlags+9],0x20
  jz rightNotPressed
  ; Speed up rightwards
  add ax,xAcceleration
  cmp ax,xMaxVelocity
  jle doneHorizontalAcceleration
  mov ax,xMaxVelocity
  jmp doneHorizontalAcceleration
rightNotPressed:
  ; Slow down
  cmp ax,0
  jl slowDownLeftwards
  sub ax,xAcceleration
  jge doneHorizontalAcceleration
stopHorizontal:
  xor ax,ax
  jmp noHorizontalAcceleration
slowDownLeftwards:
  add ax,xAcceleration
  jg stopHorizontal
doneHorizontalAcceleration:
  mov [xVelocity],ax
noHorizontalAcceleration:
  xchg ax,si
  mov dx,[xSubTile]
  add dx,si
  mov [xSubTile],dx

  mov ax,[yVelocity]
  test byte[keyboardFlags+9],1
  jz upNotPressed
  test byte[keyboardFlags+10],1
  jnz noVerticalAcceleration
  ; Speed up upwards
  sub ax,yAcceleration
  cmp ax,-yMaxVelocity
  jge doneVerticalAcceleration
  mov ax,-yMaxVelocity
  jmp doneVerticalAcceleration
upNotPressed:
  test byte[keyboardFlags+10],1
  jz downNotPressed
  ; Speed up downwards
  add ax,yAcceleration
  cmp ax,yMaxVelocity
  jle doneVerticalAcceleration
  mov ax,yMaxVelocity
  jmp doneVerticalAcceleration
downNotPressed:
  ; Slow down
  cmp ax,0
  jl slowDownUpwards
  sub ax,yAcceleration
  jge doneVerticalAcceleration
stopVertical:
  xor ax,ax
  jmp noVerticalAcceleration
slowDownUpwards:
  add ax,yAcceleration
  jg stopVertical
doneVerticalAcceleration:
  mov [yVelocity],ax
noVerticalAcceleration:
  xchg ax,di
  mov bx,[ySubTile]
  add bx,di
  mov [ySubTile],bx


  mov ax,dx
  call printHex
  mov ax,bx
  call printHex
  mov ax,si
  call printHex
  mov ax,di
  call printHex

  mov ah,0xe
  mov al,13
  xor bx,bx
  int 0x10

  jmp checkKey


printHex:
  push bx
  push ax

  mov al,ah
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  mov bl,al
  mov bh,0

  mov ah,0xe
  mov al,[bx+hex]
  xor bx,bx
  int 0x10

  pop ax
  push ax
  mov al,ah
  and al,0xf
  mov bl,al
  mov bh,0

  mov ah,0xe
  mov al,[bx+hex]
  xor bx,bx
  int 0x10

  pop ax
  push ax
  shr al,1
  shr al,1
  shr al,1
  shr al,1
  mov bl,al
  mov bh,0

  mov ah,0xe
  mov al,[bx+hex]
  xor bx,bx
  int 0x10

  pop ax
  push ax
  and al,0xf
  mov bl,al
  mov bh,0

  mov ah,0xe
  mov al,[bx+hex]
  xor bx,bx
  int 0x10

  mov ah,0xe
  mov al,0
  xor bx,bx
  int 0x10

  pop ax
  pop bx
  ret


shifts: db 1,2,4,8,0x10,0x20,0x40,0x80
keyboardFlags: times 16 db 0
imr: db 0
hex: db '0123456789ABCDEF'

