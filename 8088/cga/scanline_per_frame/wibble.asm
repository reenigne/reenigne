%ifdef bin
%include "../../defaults_bin.asm"

  stopScreen

%else
%include "../../defaults_com.asm"

main:
  mov ax,0x40
  mov ds,ax
checkMotorShutoff:
  cmp byte[0x40],0
  je noMotorShutoff
  mov byte[0x40],1
  jmp checkMotorShutoff
noMotorShutoff:

%endif

  mov dx,0x3b8
  mov al,29
  out dx,al

  mov dl,0xb4
  ;   0xff Horizontal Total                             38  38  71  71  38  38  38  61
  mov ax,0x6100
  out dx,ax

  ;   0xff Horizontal Displayed                         28  28  50  50  28  28  28  50
  mov ax,0x5001
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d  2d  5a  5a  2d  2d  2d  52
  mov ax,0x5202
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a  0a  0a  0a  0a  0a  0a  0f
  mov ax,0x0f03
  out dx,ax

  ;   0x7f Vertical Total                               1f  1f  1f  1f  7f  7f  7f  19
  mov ax,0x1904
  out dx,ax

  ;   0x1f Vertical Total Adjust                        06  06  06  06  06  06  06  06
  mov ax,0x0605
  out dx,ax

  ;   0x7f Vertical Displayed                           19  19  19  19  64  64  64  19
  mov ax,0x1906
  out dx,ax

  ;   0x7f Vertical Sync Position                       1c  1c  1c  1c  70  70  70  19
  mov ax,0x1907
  out dx,ax

  ;   0x03 Interlace Mode                               02  02  02  02  02  02  02  02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        07  07  07  07  01  01  01  0d
  mov ax,0x0d09
  out dx,ax

  ; Cursor Start                                        06  06  06  06  06  06  06  0b
  ;   0x1f Cursor Start                                  6   6   6   6   6   6   6  0b
  ;   0x60 Cursor Mode                                   0   0   0   0   0   0   0   0
  mov ax,0x0b0a
  out dx,ax

  ;   0x1f Cursor End                                   07  07  07  07  07  07  07  0c
  mov ax,0x0c0b
  out dx,ax

  ;   0x3f Start Address (H)                            00  00  00  00  00  00  00  00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00  00  00  00  00  00  00  00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   00  00  00  00  00  00  00  00
  mov ax,0x000e
  out dx,ax

  ;   0xff Cursor (L)                                   00  00  00  00  00  00  00  00
  mov ax,0x000f
  out dx,ax

  mov ax,0xb000
  mov es,ax
  xor di,di
  mov cx,80*25
  rep stosw


  in al,0x61
  or al,0x80
  mov [cs:port61high+1],al
  and al,0x7f
  mov [cs:port61low+1],al

  xor ax,ax
  mov ds,ax
  mov ax,[0x20]
  mov [cs:oldInterrupt8],ax
  mov ax,[0x22]
  mov [cs:oldInterrupt8+2],ax

  in al,0x21
  mov [cs:imr],al
  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
  out 0x21,al

  ; Determine phase
  lockstep 1
  mov ax,cs
  mov es,ax
  mov ds,ax
  mov ss,ax
  mov sp,stackTop
  mov di,data2

  in al,0x61
  or al,3
  out 0x61,al

  mov al,TIMER2 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov dx,0x42
  mov al,0
  out dx,al
  out dx,al

  %rep 5
    readPIT16 2
    stosw
  %endrep

  refreshOn

  mov ax,'0'
  mov di,[data2+8]
  mov si,[data2+6]
  mov bx,[data2+4]
  mov cx,[data2+2]
  mov dx,[data2]
  sub dx,cx
  sub dx,20
  jnz notPhase0
  add ax,1
notPhase0:
  sub cx,bx
  sub cx,20
  jnz notPhase1
  add ax,2
notPhase1:
  sub bx,si
  sub bx,20
  jnz notPhase2
  add ax,4
notPhase2:
  sub si,di
  sub si,20
  jnz notPhase3
  add ax,8
notPhase3:
  mov [phase],al

  mov di,startAddresses
  mov ax,cs
  mov es,ax
  mov ax,-1
  mov cx,101
initAddressesLoopTop:
  stosw
  add ax,80
  loop initAddressesLoopTop

  mov di,rasterData
  xor ax,ax
  mov cx,200
initRastersLoopTop:
  stosb
;  inc ax
  loop initRastersLoopTop

  call copyImageData

  jmp doneFrame

restart:

%ifdef bin
  mov al,0xff  ; Enable IRQ0 (timer), disable all others
  out 0x21,al
  mov ax,[cs:phase]
  outputHex
  mov al,13
  outputCharacter
  mov al,10
  outputCharacter
  mov ax,[cs:adjustPeriod]
  outputHex
  mov al,13
  outputCharacter
  mov al,10
  outputCharacter
  mov ax,[cs:refreshPhase]
  outputHex
  mov al,13
  outputCharacter
  mov al,10
  outputCharacter
  mov ax,[cs:cgaCrtcPhase]
  outputHex
  mov al,13
  outputCharacter
  mov al,10
  outputCharacter
  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
  out 0x21,al
%endif

  lockstep 1
;  safeRefreshOff

  ; Mode
  ;      1 +HRES
  ;      2 +GRPH
  ;      4 +BW
  ;      8 +VIDEO ENABLE
  ;   0x10 +1BPP
  ;   0x20 +ENABLE BLINK
  mov dx,0x3d8
  mov al,9
  out dx,al

  ; Palette
  ;      1 +OVERSCAN B
  ;      2 +OVERSCAN G
  ;      4 +OVERSCAN R
  ;      8 +OVERSCAN I
  ;   0x10 +BACKGROUND I
  ;   0x20 +COLOR SEL
  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4
  mov ax,0x7100
  out dx,ax
  mov ax,0x5001
  out dx,ax
  mov ax,0x5a02
  out dx,ax
  mov ax,0x0f03
  out dx,ax
  mov ax,0x3f04
  out dx,ax
  mov ax,0x0005
  out dx,ax
  mov ax,0x0206
  out dx,ax
  mov ax,0x1907
  out dx,ax
  mov ax,0x0008
  out dx,ax
  inc ax
  out dx,ax
  mov ax,0x060a
  out dx,ax
  mov ax,0x070b
  out dx,ax
  mov ax,0x000c
  out dx,ax
  inc ax
  out dx,ax
  mov ax,0x3f0e
  out dx,ax
  mov ax,0xff0f
  out dx,ax
  mov dl,0xda
  waitForNoVerticalSync
  waitForVerticalSync
  waitForDisplayEnable
  mov ax,0x0104
  mov dl,0xd4
  out dx,ax

  writePIT16 0, 2, 2   ; Ensure IRQ0 pending

  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8h0
  mov [0x22],cs

  mov dl,0xda
  waitForDisplayDisable
  waitForDisplayEnable


  cmp byte[cs:cgaCrtcPhase],1
  jne noSwitchPhase
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
  waitForDisplayDisable
  waitForDisplayEnable
noSwitchPhase:

  waitForDisplayDisable
  waitForDisplayEnable


  writePIT16 0, 2, 31

  sti
  hlt
interrupt8h0:
  mov al,75                 ; Now counting down from 31
  out 0x40,al
  mov al,0
  out 0x40,al
  mov word[0x20],interrupt8h1
  mov al,0x20
  out 0x20,al
  sti
  hlt

interrupt8h1:
  in al,dx                  ; Now counting down from 75
  test al,1
  jz .noInterruptChange  ; jump if +DISPEN, finish if -DISPEN
  mov word[0x20],interrupt8h2
.noInterruptChange:
  mov al,0x20
  out 0x20,al
  mov sp,stackTop
  sti
  hlt

interrupt8h2:
  mov ax,[cs:refreshPhase]     ; We're still counting down from 75
  out 0x40,al
  mov al,ah
  out 0x40,al
  mov word[0x20],interrupt8h3
  mov al,0x20
  out 0x20,al
  mov sp,stackTop
  sti
  hlt

interrupt8h3:
  mov word[0x20],interrupt8h4  ; We're still counting down from refreshPhase
  mov al,0x20
  out 0x20,al
  mov sp,stackTop
  sti
  hlt

interrupt8h4:
  refreshOn 19                 ; refreshPhase has happened, restart refresh
  mov al,0x20
  out 0x20,al
  mov sp,stackTop

  mov dl,0xd4
  mov ax,0x3f04
  out dx,ax

  mov dl,0xda
  waitForNoVerticalSync
  waitForVerticalSync

  waitForDisplayEnable

  writePIT16 0, 2, 76*64 - 1  ; Start counting down after display enable starts

  mov word[0x20],interrupt8a

  sti
  hlt
interrupt8a:
  in al,dx
  test al,1
  jz .noInterruptChange  ; jump if +DISPEN, finish if -DISPEN
  mov word[0x20],interrupt8b
.noInterruptChange:
  mov al,0x20
  out 0x20,al
  mov sp,stackTop
  sti
  hlt

interrupt8b:
  mov ax,[cs:adjustPeriod]     ; We're still counting down from 76*64 - 1
  out 0x40,al
  mov al,ah
  out 0x40,al
  mov word[0x20],interrupt8c
  mov al,0x20
  out 0x20,al
  mov sp,stackTop
  sti
  hlt

interrupt8c:
  mov ax,(76*262) & 0xff        ; We're still counting down from adjustPeriod
  out 0x40,al
  mov al,(76*262) >> 8
  out 0x40,al
  cmp byte[cs:stableImage],0
  je .notStableImage
  mov word[0x20],interrupt8stable
  jmp .doneImageSelect
.notStableImage:
  mov word[0x20],interrupt8
.doneImageSelect:
  mov al,0x20
  out 0x20,al
  mov sp,stackTop
  sti
  hlt

interrupt8:
  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,startAddresses
  mov dx,0x3d4
  mov bp,0x5001
  mov di,0x1900
  mov ax,0x5702
  mov si,sampleData
  mov bx,rasterData-sampleData
  mov es,ax

  ; Scanlines -1..198

%macro scanline 1
  mov al,0x00
  out dx,ax        ; e  Horizontal Total         left  0x5700  88

  mov ax,0x0202
  out dx,ax        ; f  Horizontal Sync Position right 0x0202   2

  pop cx
  mov al,0x0c
  mov ah,ch
  out dx,ax
  inc ax
  mov ah,cl
  out dx,ax

  lodsb
  out 0xe0,al

  %if %1 == -1
    mov ax,0x0104
    out dx,ax      ;    Vertical Total
    times 3 nop
  %elif %1 == 198
    mov ax,0x3f04
    out dx,ax      ;    Vertical Total                 0x3f04  64  (1 for scanlines -1 and 198, 62 for scanlines 199-260)
    times 3 nop
  %else
    mov al,[bx+si]
    mov dl,0xd9
    out dx,al
    mov dl,0xd4
  %endif

  mov ax,0x0101
  out dx,ax        ; b  Horizontal Displayed     right 0x0101   1
  xchg ax,di
  out dx,ax        ; a  Horizontal Total         right 0x1900  26
  xchg ax,di
  xchg ax,bp
  out dx,ax        ; d  Horizontal Displayed     left  0x5001  80
  xchg ax,bp
  mov ax,es
  out dx,ax        ; c  Horizontal Sync Position left  0x5702  88
%endmacro
%assign i -1
%rep 200
  scanline i
  %assign i i+1
%endrep

  ; Scanline 199

  mov ax,0x7100
  out dx,ax        ; e  Horizontal Total         left  0x7100 114
  mov ax,0x5a02
  out dx,ax        ; f  Horizontal Sync Position right 0x5a02  90

  mov sp,stackTop

  ; TODO: We are now free to do per-frame vertical-overscan stuff
  ; with no special timing requirements except:
  ;   HLT before overscan is over
  ;   Sound (if in use)

;  mov dl,0xd9
;  mov al,1
;  out dx,al

  mov di,startAddresses
  mov ax,cs
  mov es,ax
  mov ax,80*100-1
  mov bx,81*100-1
  mov cx,100
.clearLoop:
  stosw
  xchg ax,bx
  stosw
  xchg ax,bx
  loop .clearLoop

;  %assign i 0
;  %rep 10
;    mov si,[barPointers + i*2]
;    mov bx,[si]
;    add bl,bh
;    mov [si],bl
;    mov bh,0
;    add bx,bx
;    mov di,[bx+sinTable]
;    mov si,[si+2]
;    mov cx,16
;    rep movsw
;    %assign i i+1
;  %endrep

%if 0
  %assign i 0
  %rep 10
    mov si,[barPointers + i*2]
    mov bx,[si]
    add bx,[si+2]
    mov [si],bx
    rol bx,1
    xchg bh,bl
    and bh,1
    add bx,bx
    mov di,[bx+sinTable]
    mov si,[si+4]
    mov cx,16
    rep movsw
    %assign i i+1
  %endrep
%endif



; Step 1: Recompute all y and z positions, re-insert into barPointers

%macro compareSwap 0
  mov bx,[barPointers + (i-1)*2]
  cmp di,[bx+8]
  jle %%noSwap
  mov [barPointers + i*2],bx
  mov [barPointers + (i-1)*2],si
  %%noSwap:
%endmacro

  %assign i 0
  %rep 10
    mov si,[barPointers + i*2]
    mov bx,[si]
    add bx,[si+2]
    mov [si],bx
    rol bx,1
    xchg bh,bl
    and bh,1
    add bx,bx
    mov di,[bx+sinTable]
    mov [si+6],di
    inc bh
    and bh,3
    mov di,[bx+sinTable]
    mov [si+8],di
    %if i>0
      compareSwap
    %endif
    %assign i i+1
  %endrep

  recalculate

; Step 2: Draw bars

  %assign i 0
  %rep 10
    mov si,[barPointers + i*2]
    mov di,[si+6]
    mov si,[si+4]
    mov cx,16
    rep movsw
    %assign i i+1
  %endrep


;%endif



;  mov al,0
;  out dx,al


endOfFrame:
  mov al,0x20
  out 0x20,al

  inc word[frameCount]
  jnz noFrameCountCarry
  inc word[frameCount+2]
noFrameCountCarry:


  call doKeyboard
  mov sp,stackTop
  sti
  hlt

interrupt8stable:
  initCGA 0x0a
  mov dl,0xd9
  %rep 3800
    out dx,al
    inc ax
  %endrep
interrupt8numbers:
  mov ax,cs
  mov ds,ax
  jmp endOfFrame


doKeyboard:

  in al,0x60
  xchg ax,bx
  ; Acknowledge the previous byte
port61high:
  mov al,0xcf
  out 0x61,al
port61low:
  mov al,0x4f
  out 0x61,al
  cmp bl,0x4b               ; left
  je moveLeft
  cmp bl,0x4d               ; right
  je moveRight
  cmp bl,0x48               ; up
  je moveUp
  cmp bl,0x50               ; down
  je moveDown
  cmp bl,0x4a               ; keypad-
  je decreaseRefreshPhase
  cmp bl,0x2c               ; z
  je decreaseRefreshPhase
  cmp bl,0x4e               ; keypad+
  je increaseRefreshPhase
  cmp bl,0x2d               ; x
  je increaseRefreshPhase
  cmp bl,0x39               ; space
  je switchCgaCrtcPhase
  cmp bl,0x1f               ; s
  je switchImage
  cmp bl,0x31               ; n
  je toggleNumbersScreen
  cmp bl,1                  ; esc
  je tearDown2
  ret

tearDown2:
  jmp tearDown
moveLeft:
  dec word[adjustPeriod]
  jmp doneFrame
moveRight:
  inc word[adjustPeriod]
  jmp doneFrame
moveUp:
  sub word[adjustPeriod],76
  jmp doneFrame
moveDown:
  add word[adjustPeriod],76
  jmp doneFrame
decreaseRefreshPhase:
  dec word[refreshPhase]
  cmp word[refreshPhase],64-1
  jne .done
  mov word[refreshPhase],64+18
.done:
  jmp doneFrame
increaseRefreshPhase:
  inc word[refreshPhase]
  cmp word[refreshPhase],64+19
  jne .done
  mov word[refreshPhase],64+0
.done:
  jmp doneFrame
switchCgaCrtcPhase:
  xor byte[cgaCrtcPhase],1
  jmp doneFrame
switchImage:
  xor byte[stableImage],1
  cmp byte[numbersMode],0
  jne .noCopyImageData
  call copyImageData
.noCopyImageData:
  jmp doneFrame
toggleNumbersScreen:
  xor byte[numbersMode],1
  cmp byte[numbersMode],0
  je leavingNumbersMode

  initCGA 9
  call copyImageData

  jmp doneFrame
leavingNumbersMode:
  call copyImageData
doneFrame:

  mov ax,0xb000
  call printNumbers


  cmp byte[numbersMode],0
  jne doNumbersMode
  ; Not numbers mode, update numbers on MDA only

  jmp restart
doNumbersMode:

  mov ax,0xb800
  call printNumbers
  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8numbers
  ret


tearDown:
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  xor ax,ax
  mov ds,ax
  mov ax,[cs:oldInterrupt8]
  mov [0x20],ax
  mov ax,[cs:oldInterrupt8+2]
  mov [0x22],ax

  in al,0x61
  and al,0xfc
  out 0x61,al

  mov ax,cs
  mov ds,ax
  mov al,[imr]
  out 0x21,al

  writePIT16 0, 2, 0

  mov ax,3
  int 0x10

  sti
  mov ax,cs
  mov ds,ax
  mov al,[phase]
  outputCharacter

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
exit:
  mov ax,0x4c00
  int 0x21


copyImageData:
  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  xor di,di
  cld

  cmp byte[stableImage],0
  jne clearVRAM
  cmp byte[numbersMode],0
  jne clearVRAM

  mov si,copperTable
  mov dx,10*8
  mov bl,0xb1
.rowLoop:
  lodsb
  mov ah,al
  mov al,bl
  mov cx,80
  rep stosw
  dec dx
  jnz .rowLoop
  mov cx,8192-(10*8*80)
  xor ax,ax
  rep stosw
  ret

clearVRAM:
  xor ax,ax
  mov cx,8192
  rep stosw
  ret

printNybble:
  and al,0xf
  cmp al,10
  jge .letters
  add al,'0'
  jmp printCharacter
.letters:
  add al,'A'-10

printCharacter:
  mov ah,7
  stosw
  ret

printHex:
  push ax
  mov al,ah
  mov cl,4
  shr al,cl
  call printNybble
  pop ax
  push ax
  mov al,ah
  call printNybble
  pop ax
  push ax
  mov cl,4
  shr al,cl
  call printNybble
  pop ax
  call printNybble
  ret

printNumbers:
  mov es,ax
  xor di,di
  mov ax,[phase]
  call printNybble
  mov di,160
  mov ax,[adjustPeriod]
  call printHex
  mov di,320
  mov ax,[refreshPhase]
  call printHex
  mov di,480
  mov ax,[cgaCrtcPhase]
  call printNybble
  ret

dummyInterrupt8:
  push ax
  mov al,0x20
  out 0x20,al
  pop ax
  iret

frameCount: dw 0, 0
oldInterrupt8: dw 0, 0
imr: db 0
wibblePointer: dw 0, 0
wibbleFrame: dw 0

align 16

section .bss
  resw 128
stackTop:
startAddresses:
  resw 200
rasterData:
  resb 200
sampleData:
  resb 200
data2:
  resb 10

