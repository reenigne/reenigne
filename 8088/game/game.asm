cpu 8086

pitCyclesPerScanline equ 76     ; Fixed by CGA hardware
scanlinesPerFrame    equ 262    ; Fixed by NTSC standard
activeScanlines      equ 200    ; Standard CGA full-screen
screenColumns        equ 80     ; Standard CGA full-screen
scanlinesPerRow      equ 2
tileColumns          equ 8
tileRows             equ 16
bufferStride         equ 0x100
mapStride            equ 0x100
horizontalAcceleration equ 0x10
verticalAcceleration   equ 0x10
horizontalMaxVelocity  equ 0x100
verticalMaxVelocity    equ 0x100

screenWidthBytes     equ screenColumns*2
bufferTileStride     equ tileRows*bufferStride
overscanPitCycles    equ pitCyclesPerScanline*(scanlinesPerFrame - activeScanlines)
onScreenPitCycles    equ pitCyclesPerScanline*activeScanlines
tileWidthBytes       equ tileColumns*2
screenRows           equ activeScanlines/scanlinesPerRow
tilesPerScreenHorizontally  equ (screenColumns + 2*tileColumns - 2) / tileColumns
tilesPerScreenVertically    equ (screenRows + 2*tileRows - 2) / tileRows
midTileHorizontally         equ tilesPerScreenHorizontally/2
midTileVertically           equ tilesPerScreenVertically/2
mapBottom                   equ 1 + (tilesPerScreenVertically + 1)*mapStride
%define bufferPosition(x, y) (tileWidthBytes*(x) + bufferTileStride*(y))
%define mapPosition(x, y)    ((x) + mapStride*(y))
topLeftBuffer               equ bufferPosition(1, 1)
topLeftMap                  equ mapPosition(1, 1)
topRightBuffer              equ bufferPosition(tilesPerScreenHorizontally, 1)
topRightMap                 equ mapPosition(tilesPerScreenHorizontally, 1)
bottomLeftBuffer            equ bufferPosition(1, tilesPerScreenVertically)
bottomLeftMap               equ mapPosition(1, tilesPerScreenVertically)
bottomRightBuffer           equ bufferPosition(tilesPerScreenHorizontally, tilesPerScreenVertically)
bottomRightMap              equ mapPosition(tilesPerScreenHorizontally, tilesPerScreenVertically)
xPlayer                     equ (screenColumns - tileColumns)/2
yPlayer                     equ (screenRows - tileRows)/2
playerTopLeft               equ yPlayer*bufferStride + xPlayer*2

plotBufferSize       equ 100
updateBufferSize     equ 100
updateBufferStart    equ endCode + plotBufferSize*8 + 12
updateBufferEnd      equ updateBufferStart + updateBufferSize*10 + 14
preBufferParagraphs  equ (updateBufferEnd & 15) >> 4
;%assign bufferWidthTiles  screenColumns/tileColumns + 3
;%assign bufferHeightTiles (activeScanlines/2)/tileRows + 3


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
  mov [foregroundSegment],ax
  add ax,0x1000
  mov [backgroundSegment],ax
  add ax,0x1000
  mov [tilesSegment],ax


%assign i 1
%rep screenColumns
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


; TODO: Load world.dat
; TODO: Draw initial screen
;        for (int y = 0; y < _tilesPerScreenVertically + 2; ++y) {
;            int buffer = bufferRow;
;            int map = mapRow;
;            for (int x = 0; x < _tilesPerScreenHorizontally + 2; ++x) {
;                drawTile(buffer, map);
;                buffer += _tileWidthBytes;
;                ++map;
;            }
;            bufferRow += _bufferTileStride;
;            mapRow += _mapStride;
;        }
;
;        _underPlayer.allocate(_tileWidthBytes*_tileRows);
;        drawPlayer();




startup:
  mov ax,0x40
  mov ds,ax
checkMotorShutoff:
  cmp byte[0x40],0
  je noMotorShutoff
  mov byte[0x40],1
  jmp checkMotorShutoff
noMotorShutoff:

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
  mov al,1
  out dx,al

  mov ax,0xb800
  mov es,ax
  xor si,si
  mov di,[bufferTopLeft]
  mov ds,[bufferSegment]
  mov ax,bufferStride - screenWidthBytes
  mov bx,screenRows
  mov bp,screenColumns
firstDrawY:
  mov cx,bp
  rep movsw
  add di,ax
  dec bx
  jnz firstDrawY

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
vramTopLeft: dw 0
bufferTopLeft: dw topLeftBuffer
bufferTL: dw 0
mapTL: dw 0x8080  ; Start location
soundEnd: dw 0
soundStart: dw 0
musicEnd: dw 0
musicStart: dw 0
bufferSegment: dw 0
foregroundSegment: dw 0
backgroundSegment: dw 0
tilesSegment: dw 0
xVelocity: dw 0  ; In characters per 0x100 frames
yVelocity: dw 0  ; In rows per 0x100 frames
;xTilePosition: db 0  ; In tiles
;yTilePosition: db 0  ; In tiles
xSubTile: dw 0  ; In characters /0x100
ySubTile: dw 0  ; In rows /0x100
xSubTileHighOld: db 0
ySubTileHighOld: db 0
imr: db 0
;xAcceleration: dw accelerateNone ; Pointer to acceleration table
leftStart: db 0
leftEnd: db tilesPerScreenVertically
topStart: db 0
topEnd: db tilesPerScreenHorizontally
rightStart: db 0
rightEnd: db tilesPerScreenVertically
bottomStart: db 0
bottomEnd: db tilesPerScreenHorizontally
shifts: db 1,2,4,8,0x10,0x20,0x40,0x80
keyboardFlags: db 8 dup (0)

bufferLeft:
%assign i 0
%rep tilesPerScreenVertically
  db bufferPosition(0, i + 1)
%assign i i + 1
%endrep

mapLeft:
%assign i 0
%rep tilesPerScreenVertically
  db mapPosition(0, i + 1)
%assign i i + 1
%endrep

bufferTop:
%assign i 0
%rep tilesPerScreenHorizontally
  db bufferPosition(i + 1, 0)
%assign i i + 1
%endrep

bufferRight:
%assign i 0
%rep tilesPerScreenVertically
  db bufferPosition(tilesPerScreenHorizontally + 1, i + 1)
%assign i i + 1
%endrep

mapRight:
%assign i 0
%rep tilesPerScreenVertically
  db mapPosition(tilesPerScreenHorizontally + 1, i + 1)
%assign i i + 1
%endrep

bufferBottom:
%assign i 0
%rep tilesPerScreenHorizontally
  db bufferPosition(i + 1, tilesPerScreenVertically + 1)
%assign i i + 1
%endrep

%macro positive 1
  %if %1 < 0
    db 0
  %else
    db %1
  %endif
%endmacro

transitionCountLeft:
%assign i 0
%rep tileColumns
positive (tileColumns - i)*(tilesPerScreenVertically + 1)/tileColumns - 1
%assign i i + 1
%endrep

transitionCountTop:
%assign i 0
%rep tileRows
positive (tileRows - i)*(tilesPerScreenHorizonally + 1)/tileRows - 1
%assign i i + 1
%endrep

transitionCountLeft:
%assign i 0
%rep tileColumns
positive (1 + i)*(tilesPerScreenVertically + 1)/tileColumns - 1
%assign i i + 1
%endrep

transitionCountBottom:
%assign i 0
%rep tileRows
positive (1 + i)*(tilesPerScreenHorizonally + 1)/tileRows - 1
%assign i i + 1
%endrep



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

  inc word[frameCount]
  jz noFrameCountCarry
  inc word[frameCount+2]
noFrameCountCarry:

checkKey:
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
  jmp checkKey
keyPressed:
  or [keyboardFlags+bx],cl
  jmp checkKey

; keyboardFlags    1     2      4    8  0x10   0x20      0x40 0x08

;  0                   Esc      1    2     3      4         5    6
;  1               7     8      9    0     -      = Backspace  Tab
;  2               Q     W      E    R     T      Y         U    I
;  3               O     P      [    ] Enter   Ctrl         A    S
;  4               D     F      G    H     J      K         L    ;
;  5               '     ` LShift    \     Z      X         C    B
;  6               B     N      M    ,     .      /    RShift  KP*
;  7             Alt Space   Caps   F1    F2     F3        F4   F5
;  8              F6    F7     F8   F9   F10    Num    Scroll Home
;  9              Up  PgUp    KP- Left   KP5  Right       KP+  End
; 10            Down  PgDn    Ins  Del                         F11
; 11             F12

noKey:
  mov ax,[xVelocity]
  test byte[keyboardFlags+9],8
  jz leftNotPressed
  test byte[keyboardFlags+9],0x20
  jnz noHorizontalAcceleration
  ; Speed up leftwards
  sub ax,horizontalAcceleration
  cmp ax,-horizontalMaxVelocity
  jge noHorizontalAcceleration
  mov ax,-horizontalMaxVelocity
  jmp noHorizontalAcceleration
leftNotPressed:
  test byte[keyboardFlags+9],0x20
  jz rightNotPressed
  ; Speed up rightwards
  add ax,horizontalAcceleration
  cmp ax,horizontalMaxVelocity
  jle noHorizontalAcceleration
  mov ax,horizontalMaxVelocity
  jmp noHorizontalAcceleration
rightNotPressed:
  ; Slow down
  cmp word[xVelocity],0
  jl slowDownLeftwards
  sub ax,horizontalAcceleration
  cmp ax,0
  jg noHorizontalAcceleration
  xor ax,ax









  mov dx,0x3d4
  mov al,0x0c
  mov ah,[startAddress+1]
  out dx,ax
  inc ax
  mov ah,[startAddress]
  out dx,ax






  mov bx,[xVelocity]
  mov si,[xAcceleration]
  mov bx,[bx+si]
  mov [xVelocity],bx

  add bx,[xSubTile]
  cmp bh,
  mov [xSubTile],bx


  ; TODO: create update buffer
  ;   TODO: figure out where we got to
  ;   TODO: end with offScreenHandlerEnd
  ; TODO: create plotter buffer
  ;   TODO: end with idle
  ; TODO: movement processing
  ;   xSubTile
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




%assign i 1
%rep screenColumns
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
        %if i != screenColumns
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
