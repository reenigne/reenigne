cpu 8086

pitCyclesPerScanline equ 76     ; Fixed by CGA hardware
scanlinesPerFrame    equ 262    ; Fixed by NTSC standard
activeScanlines      equ 200    ; Standard CGA full-screen
screenSize_x         equ 80     ; Standard CGA full-screen
scanlinesPerRow      equ 2
tileSize_x           equ 8
tileSize_y           equ 16
bufferStride         equ 0x100
mapStride            equ 0x100
horizontalAcceleration equ 0x10
verticalAcceleration   equ 0x10
horizontalMaxVelocity  equ 0x100
verticalMaxVelocity    equ 0x100

screenWidthBytes     equ screenSize_x*2
bufferTileStride     equ tileSize_y*bufferStride
overscanPitCycles    equ pitCyclesPerScanline*(scanlinesPerFrame - activeScanlines)
onScreenPitCycles    equ pitCyclesPerScanline*activeScanlines
tileWidthBytes       equ tileSize_x*2
screenSize_y         equ activeScanlines/scanlinesPerRow
tilesPerScreen_x     equ (screenSize_x + 2*tileSize_x - 2) / tileSize_x
tilesPerScreen_y     equ (screenSize_y + 2*tileSize_y - 2) / tileSize_y
midTile_x            equ tilesPerScreen_x/2
midTile_y            equ tilesPerScreen_y/2
mapBottom                   equ 1 + (tilesPerScreen_y + 1)*mapStride
%define bufferPosition(x, y) (tileWidthBytes*(x) + bufferTileStride*(y))
%define mapPosition(x, y)    ((x) + mapStride*(y))
up_leftBuffer               equ bufferPosition(1, 1)
up_leftMap                  equ mapPosition(1, 1)
up_rightBuffer              equ bufferPosition(tilesPerScreen_x, 1)
up_rightMap                 equ mapPosition(tilesPerScreen_x, 1)
down_leftBuffer             equ bufferPosition(1, tilesPerScreen_y)
down_leftMap                equ mapPosition(1, tilesPerScreen_y)
down_rightBuffer            equ bufferPosition(tilesPerScreen_x, tilesPerScreen_y)
down_rightMap               equ mapPosition(tilesPerScreen_x, tilesPerScreen_y)
xPlayer                     equ (screenSize_x - tileSize_x)/2
yPlayer                     equ (screenSize_y - tileSize_y)/2
playerTopLeft               equ yPlayer*bufferStride + xPlayer*2
leftTileIncrement           equ -tileSize_x
upTileIncrement             equ -tileSize_y
rightTileIncrement          equ tileSize_x
downTileIncrement           equ tileSize_y
leftMapIncrement            equ -1
upMapIncrement              equ -mapStride
rightMapIncrement           equ 1
downMapIncrement            equ mapStride
leftBufferIncrement         equ -tileWidthBytes
upBufferIncrement           equ -bufferTileStride
rightBufferIncrement        equ tileWidthBytes
downBufferIncrement         equ bufferTileStride
leftScrollIncrement         equ -1
upScrollIncrement           equ -screenSize_x
rightScrollIncrement        equ 1
downScrollIncrement         equ screenSize_x
leftBufferScrollIncrement   equ -1
upBufferScrollIncrement     equ -bufferStride
rightBufferScrollIncrement  equ 1
downBufferScrollIncrement   equ bufferStride
leftTotal                   equ tilesPerScreen_y
upTotal                     equ tilesPerScreen_x
rightTotal                  equ tilesPerScreen_y
downTotal                   equ tilesPerScreen_x
leftMidTile                 equ midTile_y
upMidTile                   equ midTile_x
rightMidTile                equ midTile_y
downMidTile                 equ midTile_x

plotBufferSize       equ 100
updateBufferSize     equ 100
updateBufferStart    equ endCode + plotBufferSize*8 + 12
updateBufferEnd      equ updateBufferStart + updateBufferSize*10 + 14
preBufferParagraphs  equ (updateBufferEnd & 15) >> 4
;%assign bufferWidthTiles  screenSize_x/tileSize_x + 3
;%assign bufferHeightTiles (activeScanlines/2)/tileSize_y + 3


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
%rep screenSize_x
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
;        for (int y = 0; y < _tilesPerScreen_y + 2; ++y) {
;            int buffer = bufferRow;
;            int map = mapRow;
;            for (int x = 0; x < _tilesPerScreen_x + 2; ++x) {
;                drawTile(buffer, map);
;                buffer += _tileWidthBytes;
;                ++map;
;            }
;            bufferRow += _bufferTileStride;
;            mapRow += _mapStride;
;        }
;
;        _underPlayer.allocate(_tileWidthBytes*_tileSize_y);
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
  mov bx,screenSize_y
  mov bp,screenSize_x
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
imr: db 0
;xAcceleration: dw accelerateNone ; Pointer to acceleration table
leftStart: db 0
leftEnd: db tilesPerScreen_y
upStart: db 0
upEnd: db tilesPerScreen_x
rightStart: db 0
rightEnd: db tilesPerScreen_y
downStart: db 0
downEnd: db tilesPerScreen_x
shifts: db 1,2,4,8,0x10,0x20,0x40,0x80
keyboardFlags: db 8 dup (0)

leftBuffer:
%assign i 0
%rep tilesPerScreen_y
  db bufferPosition(0, i + 1)
%assign i i + 1
%endrep

leftMap:
%assign i 0
%rep tilesPerScreen_y
  db mapPosition(0, i + 1)
%assign i i + 1
%endrep

upBuffer:
%assign i 0
%rep tilesPerScreen_x
  db bufferPosition(i + 1, 0)
%assign i i + 1
%endrep

rightBuffer:
%assign i 0
%rep tilesPerScreen_y
  db bufferPosition(tilesPerScreen_x + 1, i + 1)
%assign i i + 1
%endrep

rightMap:
%assign i 0
%rep tilesPerScreen_y
  db mapPosition(tilesPerScreen_x + 1, i + 1)
%assign i i + 1
%endrep

downBuffer:
%assign i 0
%rep tilesPerScreen_x
  db bufferPosition(i + 1, tilesPerScreen_y + 1)
%assign i i + 1
%endrep

%macro positive 1
  %if %1 < 0
    db 0
  %else
    db %1
  %endif
%endmacro

leftTransitionCount:
%assign i 0
%rep tileSize_x
positive (tileSize_x - i)*(tilesPerScreen_y + 1)/tileSize_x - 1
%assign i i + 1
%endrep

upTransitionCount:
%assign i 0
%rep tileSize_y
positive (tileSize_y - i)*(tilesPerScreenHorizonally + 1)/tileSize_y - 1
%assign i i + 1
%endrep

rightTransitionCount:
%assign i 0
%rep tileSize_x
positive (1 + i)*(tilesPerScreen_y + 1)/tileSize_x - 1
%assign i i + 1
%endrep

downTransitionCount:
%assign i 0
%rep tileSize_y
positive (1 + i)*(tilesPerScreenHorizonally + 1)/tileSize_y - 1
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
  cmp ax,0
  jl slowDownLeftwards
  sub ax,horizontalAcceleration
  jge noHorizontalAcceleration
stopHorizontal:
  xor ax,ax
  jmp noHorizontalAcceleration
slowDownLeftwards:
  add ax,horizontalAcceleration
  jg stopHorizontal
noHorizontalAcceleration:
  mov [xVelocity],ax
  xchg ax,si
  mov dx,[xSubTile]
  mov cl,dh
  add dx,si

  mov ax,[yVelocity]
  test byte[keyboardFlags+9],1
  jz upNotPressed
  test byte[keyboardFlags+10],1
  jnz noVerticalAcceleration
  ; Speed up upwards
  sub ax,verticalAcceleration
  cmp ax,-verticalMaxVelocity
  jge noVerticalAcceleration
  mov ax,-verticalMaxVelocity
  jmp noVerticalAcceleration
upNotPressed:
  test byte[keyboardFlags+10],1
  jz downNotPressed
  ; Speed up downwards
  add ax,verticalAcceleration
  cmp ax,verticalMaxVelocity
  jle noVerticalAcceleration
  mov ax,verticalMaxVelocity
  jmp noVerticalAcceleration
downNotPressed:
  ; Slow down
  cmp ax,0
  jl slowDownUpwards
  sub ax,verticalAcceleration
  jge noVerticalalAcceleration
stopVertical:
  xor ax,ax
  jmp noVerticalAcceleration
slowDownUpwards:
  add ax,verticalAcceleration
  jg stopVertical
noVerticalAcceleration:
  mov [yVelocity],ax
  xchg ax,di
  mov bx,[ySubtile]
  mov ch,bh
  add bx,di

%macro addConstant 2
  %if %2==1
    inc %1
  %elif %2==-1
    dec %1
  %elif %2!=0
    add %1,%2
  %endif
%endmacro

%macro fillEdge 1
  mov byte[%1Start],0
  mov byte[%1End],%1Total
%endmacro

%macro emptyEdge 2
  %if %2==-1
    mov byte[%1Start],%1Total
    mov byte[%1End],%1Total
  %elif %2==1
    mov byte[%1Start],0
    mov byte[%1End],0
  %else
    mov byte[%1Start],%2MidTile
    mov byte[%1End],%2MidTile
  %endif
%endmacro

%macro incrementBound 2
  cmp byte[%1%2],%1Total
  jge %%noIncrement
  inc byte[%1%2]
  %%noIncrement:
%endmacro

%macro incrementEdge 1
  incrementBound %1,Start
  incrementBound %1,End
  mov al,[%1Start]
  cmp al,[%1End]
  jne %%noClear
  emptyEdge %1,1
  %%noClear:
%endmacro

%macro decrementBound 2
  cmp byte[%1%2],0
  jle %%noDecrement
  dec byte[%1%2]
  %%noDecrement:
%endmacro

%macro decrementEdge 1
  decrementBound %1,Start
  decrementBound %1,End
  mov al,[%1Start]
  cmp al,[%1End]
  jne %%noClear
  emptyEdge %1,-1
  %%noClear:
%endmacro

%macro checkTileBoundary 3
  %ifnidn %1,none
    %ifidn %2,x
      %assign subTile dh
      %assign increase right
    %else
      %assign subTile bh
      %assign increase down
    %endif
    %ifidn %1,increase
      cmp subTile,tileSize_%2
      jl %%noTileBoundary
    %else
      cmp subTile,0
      jge %%noTileBoundary
    %endif
    add byte[%2SubTile+1],-%1TileIncrement
    addConstant word[mapTL],%1MapIncrement
    add word[bufferTL],%1BufferIncrement
    emptyEdge %1,0
    %ifidn %1,left
      fillEdge right
      incrementEdge up
      incrementEdge down
    %endif
    %ifidn %1,up
      fillEdge down
      incrementEdge left
      incrementEdge right
    %endif
    %ifidn %1,right
      fillEdge left
      decrementEdge up
      decrementEdge down
    %endif
    %ifidn %1,down
      fillEdge up
      decrementEdge left
      decrementEdge right
    %endif
    %3
    %%noTileBoundary:
  %endif
%endmacro

%macro diagonal 2
  checkTileBoundary %2, x, {drawTile %1_%2Buffer, %1_%2Map}
%endmacro

%macro saveTile 2
; TODO
%endmacro

%macro drawTransparentTile 2
; TODO
%endmacro

%macro ensureEnoughTiles 1
  %%loopTop:
  mov al,[rightEnd]
  sub al,[rightStart]
  mov bl,[xSubTile+1]
  mov bh,0
  cmp al,[%1TransitionCount + bx]

;        while (_rightEnd - _rightStart < _transitionCountRight[_xSubTile >> 8]) {
;            int y;
;            if (_yVelocity > 0) {
;                if (_rightEnd < _tilesPerScreenVertically) {
;                    y = _rightEnd;
;                    ++_rightEnd;
;                }
;                else {
;                    y = _rightStart - 1;
;                    --_rightStart;
;                }
;            }
;            else {
;                if (_rightStart > 0) {
;                    y = _rightStart - 1;
;                    --_rightStart;
;                }
;                else {
;                    y = _rightEnd;
;                    ++_rightEnd;
;                }
;            }
;            drawTile(_bufferRight[y], _mapRight[y]);
;        }

%endmacro

%macro scroll 2                  ; %1 == up/down/none, %2 == left/right/none
  checkTileBoundary %2, x, { }
  addConstant word[startAddress],%1ScrollIncrement + %2ScrollIncrement
  addConstant word[vramTopLeft],2*(%1ScrollIncrement + %2ScrollIncrement)
  addConstant word[bufferTopLeft],%1BufferScrollIncrement + %2BufferScrollIncrement
  %ifidn %2,left
    addUpdateBlock 0, 0, 1, screenSize_y
    %ifidn %1,up
      addUpdateBlock 1, 0, screenSize_x - 1, 1
      addUpdateBlock xPlayer, yPlayer, tileSize_x + 1, tileSize_y + 1
    %elifidn %1,none
      addUpdateBlock xPlayer, yPlayer, tileSize_x + 1, tileSize_y
    %else
      addUpdateBlock 1, screenSize_y - 1, screenSize_x - 1, 1
      addUpdateBlock xPlayer, yPlayer - 1, tileSize_x + 1, tileSize_y + 1
    %endif
  %elifidn %1,none
    %ifidn %1,up
      addUpdateBlock 0, 0, screenSize_x, 1
      addUpdateBlock xPlayer, yPlayer, tileSize_x, tileSize_y + 1
    %elifidn %1,none
    %else
      addUpdateBlock 0, _screenSize_y - 1, _screenSize_x, 1
      addUpdateBlock xPlayer, yPlayer - 1, tileSize_x, tileSize_y + 1
    %endif
  %else
    addUpdateBlock screenSize_x - 1, 0, 1, screenSize_y
    %ifidn %1,up
      addUpdateBlock 0, 0, screenSize_x - 1, 1
      addUpdateBlock xPlayer - 1, yPlayer, tileSize_x + 1, tileSize_y + 1
    %elifidn %1,none
      addUpdateBlock xPlayer - 1, yPlayer, tileSize_x + 1, tileSize_y
    %else
      addUpdateBlock 0, _screenSize_y - 1, _screenSize_x, 1
      addUpdateBlock xPlayer - 1, yPlayer - 1, tileSize_x + 1, tileSize_y + 1
    %endif
  %endif
  %ifnidn %1_%2,none_none
    %ifnidn %2,left
      ensureEnoughTiles right
    %endif
    %ifnidn %1,up
;        while (_bottomEnd - _bottomStart < _transitionCountBottom[_ySubTile >> 8]) {
;            int x;
;            if (_xVelocity > 0) {
;                if (_bottomEnd < _tilesPerScreenHorizontally) {
;                    x = _bottomEnd;
;                    ++_bottomEnd;
;                }
;                else {
;                    x = _bottomStart - 1;
;                    --_bottomStart;
;                }
;            }
;            else {
;                if (_bottomStart > 0) {
;                    x = _bottomStart - 1;
;                    --_bottomStart;
;                }
;                else {
;                    x = _bottomEnd;
;                    ++_bottomEnd;
;                }
;            }
;            drawTile(_bufferBottom[x], _mapBottom + x);
;        }
    %endif
    %ifnidn %2,right
;        while (_leftEnd - _leftStart < _transitionCountLeft[_xSubTile >> 8]) {
;            int y;
;            if (_yVelocity > 0) {
;                if (_leftEnd < _tilesPerScreenVertically) {
;                    y = _leftEnd;
;                    ++_leftEnd;
;                }
;                else {
;                    y = _leftStart - 1;
;                    --_leftStart;
;                }
;            }
;            else {
;                if (_leftStart > 0) {
;                    y = _leftStart - 1;
;                    --_leftStart;
;                }
;                else {
;                    y = _leftEnd;
;                    ++_leftEnd;
;                }
;            }
;            drawTile(_bufferLeft[y], _mapLeft[y]);
;        }
    %endif
    %ifnidn %1,down
;        while (_topEnd - _topStart < _transitionCountTop[_ySubTile >> 8]) {
;            int x;
;            if (_xVelocity > 0) {
;                if (_topEnd < _tilesPerScreenHorizontally) {
;                    x = _topEnd;
;                    ++_topEnd;
;                }
;                else {
;                    x = _topStart - 1;
;                    --_topStart;
;                }
;            }
;            else {
;                if (_topStart > 0) {
;                    x = _topStart - 1;
;                    --_topStart;
;                }
;                else {
;                    x = _topEnd;
;                    ++_topEnd;
;                }
;            }
;            drawTile(_bufferTop[x], x + 1);
;        }
    %endif
    saveTile playerTopLeft, underPlayer
    drawTransparentTile playerTopLeft, 0
  %endif
%endmacro

%macro restoreTile 2
; TODO
%endmacro

%macro vertical 2
  cmp bh,ch
  je %2Move
  restoreTile playerTopLeft, underPlayer
  checkTileBoundary %1, y, {diagonal %1, %2}
  scroll %1, %2
  jmp noneMove
%endmacro

%macro verticals 1
  cmp di,0
  jl %%yVelocityNegative
  vertical down, %1
%%yVelocityNegative:
  vertical up, %1
%endmacro

%macro horizontal 1
  cmp dh,cl
  je notMovingHorizontally
  restoreTile playerTopLeft, underPlayer
  verticals %1
%1Move:
  scroll none, %1
%endmacro

  cmp si,0
  jl xVelocityNegative
  horizontal right
xVelocityNegative:
  horizontal left
notMovingHorizontally:
  verticals none
noneMove:











  mov dx,0x3d4
  mov al,0x0c
  mov ah,[startAddress+1]
  out dx,ax
  inc ax
  mov ah,[startAddress]
  out dx,ax



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
%rep screenSize_x
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
        %if i != screenSize_x
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
