cpu 8086

pitCyclesPerScanline equ 76     ; Fixed by CGA hardware
scanlinesPerFrame    equ 262    ; Fixed by NTSC standard
activeScanlines      equ 200
overscanPitCycles    equ pitCyclesPerScanline*(scanlinesPerFrame - activeScanlines)
onScreenPitCycles    equ pitCyclesPerScanline*activeScanlines
screenWidth          equ 80
charactersPerTile    equ 8
rowsPerTile          equ 16
plotBufferSize       equ 100
updateBufferSize     equ 100
updateBufferStart    equ endCode + plotBufferSize*8 + 12
updateBufferEnd      equ updateBufferStart + updateBufferSize*10 + 14
preBufferParagraphs  equ (updateBufferEnd & 15) >> 4
%assign bufferWidthTiles  screenWidth/charactersPerTile + 3
%assign bufferHeightTiles (activeScanlines/2)/rowsPerTile + 3



%assign i 1
%rep screenWidth
  %assign plotterHeights%[i] 0
  %assign updaterHeights%[i] 0
  %assign i i+1
%endrep

%define plotter(x,y) (plotter %+ x - y*(x + 2) + 2)

%macro makePlotter 2  ;  width height
  %if %2 > plotterHeights%[%1]
    %assign plotterHeights%[%1] %2
  %endif
%endmacro

%macro makeUpdater 2  ;  width height
  %if %2 > updaterHeights%[%1]
    %assign updaterHeights%[%1] %2
  %endif
%endmacro

makePlotter 2,3
mov ax,plotter(2,3)

startup:
  mov ax,0x40
  mov ds,ax
checkMotorShutoff:
  cmp byte[0x40],0
  je noMotorShutoff
  mov byte[0x40],1
  jmp checkMotorShutoff
noMotorShutoff:

  mov ax,cs
  mov ds,ax
  mov [soundPointer+2],ax
  mov [musicPointer+2],ax
  mov bx,endPreBuffer
  add bx,15
  mov cl,4
  shr bx,cl
  add ax,bx
  mov [bufferSegment],ax
  add ax,0x1000
  mov [mapSegment],ax
  add ax,0x1000
  mov [tilesSegment],ax

  in al,0x61
  or al,3
  out 0x61,al
  or al,0x80
  mov [port61high+1],al
  and al,0x7f
  mov [port61low+1],al

  mov al,0xb6
  out 0x43,al
  mov al,0x02
  out 0x42,al
  mov al,0x00
  out 0x42,al

  in al,0x21
  mov [imr],al
  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
  out 0x21,al

  mov ax,3
  int 0x10
  mov dx,0x3d8
  mov al,9
  out dx,al
  mov dl,0xd4
  mov ax,0x0f03
  out dx,ax
  mov ax,0x7f04
  out dx,ax
  mov ax,0x6406
  out dx,ax
  mov ax,0x7007
  out dx,ax
  mov ax,0x0109
  out dx,ax
  mov dl,0xda
waitForVSync:
  in al,dx
  test al,8
  jz waitForVSync
  cli
  xor ax,ax
  mov ds,ax

waitForDisplayEnable:
  in al,dx
  test al,1
  jnz waitForDisplayEnable

  mov al,0x34
  out 0x43,al
  mov ax,2
  out 0x40,al
  mov al,ah
  out 0x40,al

  mov ax,onScreenPitCycles/2
  out 0x40,al
  mov al,ah
  out 0x40,al

  times 5 nop
  sti
  times 5 nop
  cli

  mov ax,[0x20]
  mov [cs:oldInterrupt8],ax
  mov ax,[0x22]
  mov [cs:oldInterrupt8+2],ax
  mov word[0x20],offScreenHandler
  mov [0x22],cs

  mov al,0x0a  ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 0x20,al  ;  read Interrupt Request Register
  sti

idle:
  hlt
  jmp idle


teardown:
  xor ax,ax
  mov ds,ax
  cli
  mov ax,[cs:oldInterrupt8]
  mov [0x20],ax
  mov ax,[cs:oldInterrupt8+2]
  mov [0x22],ax
  sti

  in al,0x61
  and al,0xfc
  out 0x61,al

  mov ax,cs
  mov ds,ax
  mov al,[imr]
  out 0x21,al

  mov ax,3
  int 0x10

  mov ax,19912
  mul word[frameCount]
  mov cx,dx
  mov ax,19912
  mul word[frameCount+2]
  add ax,cx
  adc dx,0
  mov cx,0x40
  mov ds,cx
  add [0x6c],ax
  adc [0x6e],dx
dateLoop:
  cmp word[0x6c],0x18
  jb doneDateLoop
  cmp word[0x6e],0xb0
  jb doneDateLoop
  mov byte[0x70],1
  sub word[0x6c],0xb0
  sbb word[0x6e],0x18
  jmp dateLoop
doneDateLoop:

  mov ax,0x4c00
  int 0x21


oldInterrupt8: dw 0, 0
frameCount: dw 0, 0
soundPointer: dw 0, 0
musicPointer: dw 0, 0
startAddress: dw 0
soundEnd: dw 0
soundStart: dw 0
musicEnd: dw 0
musicStart: dw 0
bufferSegment: dw 0
mapSegment: dw 0
tilesSegment: dw 0
xVelocity: dw 0  ; In characters per 0x100 frames
yVelocity: dw 0  ; In rows per 0x100 frames
xTilePosition: db 0  ; In tiles
yTilePosition: db 0  ; In tiles
xCharPosition: dw 0  ; In characters /0x100
yRowPosition: dw 0  ; In rows /0x100
imr: db 0
upDown: db 0
downDown: db 0
leftDown: db 0
rightDown: db 0
spaceDown: db 0
xAcceleration: dw accelerateNone ; Pointer to acceleration table
topTiles: dw 0
bottomTiles: dw 0
leftTiles: dw 0
rightTiles: dw 0


offScreenHandler:
  push bx
  push di
  push si
  mov bp,sp

  mov al,0x20
  out 0x20,al

  xor ax,ax
  mov ds,ax
  mov word[0x20],onScreenHandler

  mov al,(onScreenPitCycles & 0xff)
  out 0x40,al
  mov al,(onScreenPitCycles >> 8)
  out 0x40,al

  lds si,[cs:musicPointer]
  lodsw
  out 0x42,al
  mov al,ah
  out 0x42,al
  cmp si,[musicEnd]
  jne noRestartMusic
  mov si,[musicStart]
noRestartMusic:
  mov [musicPointer],si

  mov sp,updateBufferStart + 6
  pop si
  pop di
  pop bx
  pop cx
  sti
  ret

offScreenHandlerEnd:
  mov sp,bp
  pop si
  pop di
  pop bx
  pop ax
  pop bp
  pop bp
  jmp ax


onScreenHandler:
  push cx
  push bx
  push di
  push si
  mov al,0x20
  out 0x20,al

  xor ax,ax
  mov ds,ax
  mov word[0x20],offScreenHandler

  mov al,(overscanPitCycles & 0xff)
  out 0x40,al
  mov al,(overscanPitCycles >> 8)
  out 0x40,al

  lds si,[cs:soundPointer]
  lodsw
  out 0x42,al
  mov al,ah
  out 0x42,al
  cmp si,[soundEnd]
  jne noRestartSound
  mov si,[soundStart]
noRestartSound:
  mov [soundPointer],si

  mov dx,0x3d4
  mov al,0x0c
  mov ah,[startAddress+1]
  out dx,ax
  inc ax
  mov ah,[startAddress]
  out dx,ax

  inc word[frameCount]
  jz noFrameCountCarry
  inc word[frameCount+2]
noFrameCountCarry:

  in al,0x20
  and al,2    ; Check for IRR bit 1 (IRQ 1) high
  jz noKey
readKey
  ; Read the keyboard byte and store it
  in al,0x60
  cbw
  xchg ax,bx
  ; Acknowledge the previous byte
port61high:
  mov al,0xcf
  out 0x61,al
port61low:
  mov al,0x4f
  out 0x61,al

  add bx,bx
  jmp word[bx+keyboardTable]
noKey:
  mov bx,[xVelocity]
  mov si,[xAcceleration]
  mov bx,[bx+si]
  mov [xVelocity],bx

  add bx,[xCharPosition]
  cmp bh,
  mov [xCharPosition],bx


  ; TODO: create update buffer
  ;   TODO: figure out where we got to
  ;   TODO: end with offScreenHandlerEnd
  ; TODO: create plotter buffer
  ;   TODO: end with idle
  ; TODO: movement processing
  ;   xCharPosition
  ;   xTilePosition
  ;   startAddress
  ; TODO: game logic
  ;   jumping
  ;   collision detection
  ;   enemy motion

  mov sp,endCode + 6  ; == plotter buffer start
  pop si
  pop di
  pop bx
  pop cx
  sti
  ret


escPressed:
  jmp teardown
upPressed:
  mov byte[upDown],1
  jmp noKey
upReleased:
  mov byte[upDown],0
  jmp noKey
downPressed:
  mov byte[downDown],1
  jmp noKey
downReleased:
  mov byte[downDown],0
  jmp noKey
leftPressed:
  mov byte[leftDown],1
doLeft:
  mov word[xAcceleration],accelerateLeft
  jmp leftRightLeft
leftReleased:
  mov byte[leftDown],0
  test byte[rightDown],1
  jnz doRight
  mov word[xAcceleration],accelerateNone
  jmp noKey
rightPressed:
  mov byte[rightDown],1
doRight:
  mov word[xAcceleration],accelerateRight
  jmp noKey
rightReleased:
  mov byte[rightDown],0
  test byte[leftDown],1
  jnz doLeft
  mov word[xAcceleration],accelerateNone
  jmp noKey
spacePressed:
  mov byte[spaceDown],1
  jmp noKey
spaceReleased:
  mov byte[spaceDown],0
  jmp noKey



  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 0       Esc  1   2   3   4   5      6    7    8     9      0    -     =     Backspace Tab
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 1  Q    W    E   R   T   Y   U      I    O    P     [      ]    Enter Ctrl  A         S
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 2  D    F    G   H   J   K   L      ;    '    `     LShift \    Z     X     C         V
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 3  B    N    M   ,   .   /   RShift KP*  Alt  Space Caps   F1   F2    F3    F4        F5
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, upReleased, noKey, noKey, leftReleased, noKey, rightReleased, noKey, noKey  ; 4  F6   F7   F8  F9  F10 Num Scroll Home Up   PgUp  KP-    Left KP5   Right KP+       End
  dw downReleased, noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 5  Down PgDn Ins Del                F11  F12
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey
keyboardTable:                                                                                                                                        ;    0    1    2   3   4   5   6      7    8    9     A      B    C     D     E         F
  dw noKey,        escPressed,  noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 0       Esc  1   2   3   4   5      6    7    8     9      0    -     =     Backspace Tab
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 1  Q    W    E   R   T   Y   U      I    O    P     [      ]    Enter Ctrl  A         S
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 2  D    F    G   H   J   K   L      ;    '    `     LShift \    Z     X     C         V
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 3  B    N    M   ,   .   /   RShift KP*  Alt  Space Caps   F1   F2    F3    F4        F5
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, upPressed,  noKey, noKey, leftPressed,  noKey, rightPressed,  noKey, noKey  ; 4  F6   F7   F8  F9  F10 Num Scroll Home Up   PgUp  KP-    Left KP5   Right KP+       End
  dw downPressed,  noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey  ; 5  Down PgDn Ins Del                F11  F12
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey
  dw noKey,        noKey,       noKey, noKey, noKey, noKey, noKey, noKey, noKey,      noKey, noKey, noKey,        noKey,                noKey, noKey


%assign i 1
%rep screenWidth
  %assign n plotterHeights%[i]
  %if n > 0
    %assign j 0
    %rep n
      times i movsw
      %if j < n - 1
        add di,bx
      %endif
      %assign j j+1
    %endrep
  plotter%[i]:
    pop si
    pop di
    pop bx
    ret
  %endif

  %assign n updaterHeights%[i]
  %if n > 0
    %assign j 0
    %rep n
      times i movsw
      %if j < n - 1
        %if i != screenWidth
          add di,bx
        %endif
        add si,cx
      %endif
      %assign j j+1
    %endrep
  updater%[i]:
    pop si
    pop di
    pop bx
    pop cx
    ret
  %endif

  %assign i i+1
%endrep

endCode:
