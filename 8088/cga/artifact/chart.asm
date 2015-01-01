  %include "../../defaults_com.asm"

main:
  cli
  xor ax,ax
  mov ds,ax
  getInterrupt 9, oldInterrupt9
  setInterrupt 9, interrupt9

  mov ax,cs
  mov ds,ax


  ; Set mode. We'll actually use 40-column text mode, so we can display the
  ; mode and palette register values on the first line.
  mov ax,1
  int 0x10

  mov al,0x1e
  mov dx,0x3d8
  out dx,al

  mov al,0x0f
  mov dx,0x3d9
  out dx,al


  ; Draw pattern in graphics memory
  mov ax,0xb800
  mov es,ax
screenLoop:
  mov di,0
  cld

rowLoop:

  mov cx,200
lineLoop:

  push cx
  mov al,0
  mov cx,16
barLoop:
  push cx
  mov cx,5
  rep stosb
  pop cx
  add al,0x11
  loop barLoop

  add di,0x1fb0
  cmp di,0x4000
  jl oddLine
  sub di,0x3fb0
oddLine:

  pop cx
  loop lineLoop

  ; Set up first line
  mov cx,40
  mov di,0
  mov si,topLine
topLineLoop:
  movsb
  mov al,15
  stosb
  loop topLineLoop

  ; Set up mode
  call setMode


  mov dx,0x3da
frameLoop:
  sti
  waitForVerticalSync
  waitForNoVerticalSync
  cli

  ; Set text mode for 8 lines
  mov dl,0xd8
  mov al,[modeRegister]
  and al,0x0c  ; -HRES, -GRPH, -1BPP, -ENABLE BLINK
  out dx,al
  mov dl,0xda

  mov cx,8
lineLoop2t:
  waitForDisplayEnable
  waitForDisplayDisable
  loop lineLoop2t

  ; Set graphics mode
  mov dl,0xd8
  mov al,[modeRegister]
  out dx,al
  inc dx
  mov al,[paletteRegister]
  out dx,al
  mov bl,al
  inc dx


  mov cx,16
rowLoop2:
  push cx

  mov cx,11
  inc bl
lineLoop2:
  waitForDisplayEnable
  waitForDisplayDisable
  loop lineLoop2
  waitForDisplayEnable
  waitForDisplayDisable
  mov al,bl
  dec dx
  out dx,al
  inc dx
  pop cx
  loop rowLoop2

  mov al,[lastScanCode]
  mov byte[lastScanCode],0
  cmp al,1
  je finished
  cmp al,0
  jle noKeyPressed

  call setMode

noKeyPressed:
  jmp frameLoop

finished:
  mov ax,3
  int 0x10
  xor ax,ax
  mov ds,ax
  restoreInterrupt 9, oldInterrupt9
  ret


setMode:
  mov si,[modePointer]
  mov cx,4
  mov di,0
legendLoop:
  movsb
  mov al,15
  stosb
  loop legendLoop

  lodsb
  mov [modeRegister],al
  lodsb
  mov [paletteRegister],al

  cmp si,modePointer
  jne gotNextMode
  mov si,modes
gotNextMode:
  mov [modePointer],si
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


topLine:
  db " 0 1  2 3  4 5  6 7  8 9  A B  C D  E F "

modes:
  db "1a00",0x1a,0x00
  db "1e00",0x1e,0x00
  db "0a00",0x0a,0x00
  db "0a10",0x0a,0x10
  db "0a20",0x0a,0x20
  db "0a30",0x0a,0x30
  db "0e20",0x0e,0x20
  db "0e30",0x0e,0x30
modePointer:
  dw modes

modeRegister:
  db 0
paletteRegister:
  db 0
lastScanCode:
  db 0
