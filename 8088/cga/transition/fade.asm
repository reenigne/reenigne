org 0x100
cpu 8086
programBase:

pitCyclesPerScanline equ 76     ; Fixed by CGA hardware
scanlinesPerFrame    equ 262    ; Fixed by NTSC standard
activeScanlines      equ 200    ; Standard CGA full-screen
visual_profiler      equ 0
onScreenPitCycles    equ pitCyclesPerScanline*activeScanlines - 22
offScreenPitCycles   equ pitCyclesPerScanline*scanlinesPerFrame - (onScreenPitCycles)

%include "transitionTables.inc"

bssOffset equ (((programEnd - programBase) + 15) & -16) + 0x100

setupMemory:
  mov ax,cs
  mov ds,ax
  cli
  mov ss,ax
  mov sp,stackHigh
  sti
  mov bx,ax
  add ax,((picturesData - bssBase) + bssOffset) >> 4
  mov [picturesBinSegment],ax
  mov ax,bx
  add ax,((redGreenImages - programBase) + 0x100) >> 4
  mov [innerLoopDS],ax
  add bx,((blueImages - bssBase) + bssOffset) >> 4
  mov [innerLoopES],bx

  push ds
  mov bx,0x40
  mov ds,bx
  mov bx,[0x13]
  pop ds
  add ax,2000*imageCount
  mov cl,6
  shl bx,cl
  cmp ax,bx
  jbe .noError
  mov ah,9
  mov dx,memoryError
  int 0x21
  jmp exit
.noError:

loadPicturesBin:
  mov dx,picturesBin
  mov ax,0x3d00
  int 0x21
  jnc .noError
.error:
  mov ah,9
  mov dx,picturesBinError
  int 0x21
  jmp exit
.noError:
  mov bx,ax
  mov bp,[picturesBinSegment]
  mov ax,bp
  add ax,imageCount*2000
  mov [picturesEndSegment],ax

%rep imageCount
  mov ds,bp
  mov ah,0x3f
  mov cx,32000
  xor dx,dx
  int 0x21
  jc .error
  add bp,2000
%endrep

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
  mov es,ax
  in al,0x61
  or al,0x80
  mov [port61high+1],al
  and al,0x7f
  mov [port61low+1],al

  mov si,wipeSequence
  mov di,movedWipeSequence
  mov cx,8000
  rep movsw

%if fadeType==1
  maximumUpdates equ 368
%else
;  maximumUpdates equ ;TODO: figure this out
%endif

initUpdateBuffer:
  mov cx,maximumUpdates
%if fadeType==1
  mov ax,0x06c7
  mov bx,4
%else
  mov cx,
  mov ax,0x06c6
  mov bx,3
%endif
  mov di,updateBuffer
.loopTop:
  stosw
  add di,bx
  loop .loopTop

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
  cli
  xor ax,ax
  mov ds,ax

  mov al,0x34
  out 0x43,al

%macro setPIT0Count 1
  mov al,(%1) & 0xff
  out 0x40,al
  %if ((%1) & 0xff) != ((%1) >> 8)
  mov al,(%1) >> 8
  %endif
  out 0x40,al
%endmacro

  setPIT0Count 2  ; PIT was reset so we start counting down from 2 immediately

%macro waitForVerticalSync 0
  %%waitForVerticalSync:
    in al,dx
    test al,8
    jz %%waitForVerticalSync       ;         jump if not +VSYNC, finish if +VSYNC
%endmacro

%macro waitForNoVerticalSync 0
  %%waitForNoVerticalSync:
    in al,dx
    test al,8
    jnz %%waitForNoVerticalSync    ;         jump if +VSYNC, finish if -VSYNC
%endmacro

  ; Wait for a while to be sure that IRQ0 is pending
  waitForVerticalSync
  waitForNoVerticalSync
  waitForVerticalSync

waitForDisplayEnable:
  in al,dx
  test al,1
  jnz waitForDisplayEnable

  setPIT0Count onScreenPitCycles

  ; PIT channel 0 is now counting down from onScreenPitCycles in top half of onscreen area and IRQ0 is pending

  mov ax,[0x20]
  mov [cs:oldInterrupt8],ax
  mov ax,[0x22]
  mov [cs:oldInterrupt8+2],ax
  mov word[0x20],transitionHandler
  mov [0x22],cs

idle:
  sti
.loop:
  hlt
  jmp .loop

transitionHandler:
  mov al,0x20
  out 0x20,al

  ; PIT channel 0 is now counting down from onScreenPitCycles in onscreen area

  setPIT0Count offScreenPitCycles

  ; When the next interrupt happens, PIT channel 0 will start counting down from offScreenPitCycles in offscreen area

  mov word[0x20],offScreenHandler
  mov [0x22],cs

  mov ax,cs
  mov ds,ax
  mov ax,[picturesBinSegment]
  mov [oldImageSegment],ax
  add ax,2000
  mov [newImageSegment],ax
  sti
foregroundTask:
  cmp byte[transitionActive],0
  jne foregroundTask
  ; Transition not active - init a new one

  mov word[spaceStartFrame],spaceStartInitial
  mov word[spaceEndFrame],spaceEndInitial
  mov word[spaceStartFracFrame],spaceStartFracInitial
  mov word[spaceEndFracFrame],spaceEndFracInitial
  mov word[transitionFrame],0

; Process RGB tables from images (bbbgggrr r0000000, BBBGGGRR R0000000) to blueImages/redGreenImages (rrrRRR00 gggGGG00, bbbBBB00 00000000)
processRGB:
  mov cx,8000
  mov si,[oldImageSegment]
  add si,2000
  mov ax,[picturesEndSegment]
  cmp si,ax
  jb .noWrapOld
  sub si,imageCount*2000
.noWrapOld:
  mov [oldImageSegment],si
  mov bp,[newImageSegment]
  add bp,2000
  cmp bp,ax
  jb .noWrapNew
  sub bp,imageCount*2000
.noWrapNew:
  mov [newImageSegment],bp

  xor di,di
  mov es,[innerLoopDS]
  add si,1000
  add bp,1000

.loopTop:
  mov ds,bp                                    ; 2
  mov dx,[di]   ; dx = bbbgggrr r0000000       ; 4
  mov ds,si                                    ; 2
  mov bx,[di]   ; bx = BBBGGGRR R0000000       ; 4
  mov ah,bl     ; ah = BBBGGGRR                ; 2

  shl bx,1                                     ; 2
  shl bx,1                                     ; 2
  shl bx,1      ; bx = 000BBBGG GRRR0000       ; 2
  mov al,dl     ; al = bbbgggrr                ; 2
  and al,7      ; al = bbb00000                ; 2
  or al,bl      ; al = bbbBBBGG                ; 2
  and al,0x3f   ; al = bbbBBB00                ; 2
  mov [es:di + blueImages - redGreenImages],al ; 6

  shl dx,1                                     ; 2
  shl dx,1      ; dx = 00bbbggg rrr00000       ; 2
  shl bx,1                                     ; 2
  shl bx,1      ; bx = 00000BBB GGGRRR00       ; 2
  mov al,bh     ; al = GGGRRR00                ; 2
  and al,0x38   ; al = 000RRR00                ; 2
  or al,dh      ; al = rrrRRR00                ; 2
  shl dx,1                                     ; 2
  shl dx,1                                     ; 2
  shl dx,1      ; dx = 00000bbb gggrrr00       ; 2
  and ah,0x38   ; ah = 000GGG00                ; 3
  and dh,7      ; dh = ggg00000                ; 3
  or ah,dh      ; ah = gggGGG00                ; 2
  stosw                                        ; 3

  loop .loopTop                                ; 2    total 67 IOs = ~27 frames

  mov ax,cs
  mov ds,ax
  mov byte[transitionActive],1

  jmp foregroundTask



align 256, db 0
redGreenImages:
dataTables

oldInterrupt8: dw 0, 0
frameCount: dw 0, 0
updatePointer: dw updateBuffer
picturesBinSegment: dw 0
picturesEndSegment: dw 0
picturesBin: db 'pictures.bin',0
picturesBinError: db 'Error reading pictures.bin file.$'
memoryError: db 'Not enough memory.$'
imr: db 0
transitionActive: db 0
oldImageSegment: dw 0
newImageSegment: dw 0
spaceStartFrame: dw 0
spaceEndFrame: dw 0
spaceStartFracFrame: dw 0
spaceEndFracFrame: dw 0
spaceStart: dw 0
spaceEnd: dw 0
spaceStartFrac: dw 0
spaceEndFrac: dw 0
transitionFrame: dw 0
innerLoopDS: dw 0
innerLoopES: dw 0


offScreenHandler:
  push ax
  push ds
  push es
  push bx
  mov al,0x20
  out 0x20,al

  xor ax,ax
  mov ds,ax
  mov word[0x20],onScreenHandler

  setPIT0Count onScreenPitCycles

  mov ax,0xb800
  mov ds,ax
  mov bx,[cs:updatePointer]
  mov byte[cs:bx],0xc3  ; ret
  call updateBuffer
%if fadeType==1
  mov byte[cs:bx],0xc7
%else
  mov byte[cs:bx],0xc6
%endif
  pop bx
  pop es
  pop ds
  pop ax
  iret


onScreenHandler:
  push ax
  push bx
  push cx
  push dx
  push si
  push di
  push bp
  push es
  push ds
  mov al,0x20
  out 0x20,al

  xor ax,ax
  mov ds,ax
  mov word[0x20],offScreenHandler

  setPIT0Count offScreenPitCycles

  mov ax,cs
  mov ds,ax

  inc word[frameCount]
  jnz noFrameCountCarry
  inc word[frameCount+2]
noFrameCountCarry:

checkKey:
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

  cmp bl,1
  je teardown

  cmp byte[transitionActive],0
  je doneOnScreenHandler

newFrame:
  mov bx,[spaceStartFrame]
  add bx,frameIncrement
  mov ax,[spaceStartFracFrame]
  add ax,frameFracIncrement
  cmp ax,denominator
  jl .noStartCarry
  sub ax,denominator
  inc bx
.noStartCarry:
  mov [spaceStartFrame],bx
  mov [spaceStart],bx
  mov [spaceStartFracFrame],ax
  mov [spaceStartFrac],ax

  mov bx,[spaceEndFrame]
  add bx,frameIncrement
  mov ax,[spaceEndFracFrame]
  add ax,frameFracIncrement
  cmp ax,denominator
  jl .noEndCarry
  sub ax,denominator
  inc bx
.noEndCarry:
  mov [spaceEndFrame],bx
  mov [spaceEnd],bx
  mov [spaceEndFracFrame],ax
  mov [spaceEndFrac],ax

  mov di,updateBuffer + 2 - blueImages
  mov es,[innerLoopES]

%macro doStep 1
    mov si,[spaceStart]
    add si,spaceStepIncrement
    mov ax,[spaceStartFrac]
    add ax,spaceStepFracIncrement
    cmp ax,0
    jge %%noStartCarry
    add ax,denominator
    dec si
%%noStartCarry:
    mov [spaceStart],si
    mov [spaceStartFrac],ax

    mov bx,[spaceEnd]
    add bx,spaceStepIncrement
    mov ax,[spaceEndFrac]
    add ax,spaceStepFracIncrement
    cmp ax,0
    jge %%noEndCarry
    add ax,denominator
    dec bx
%%noEndCarry:
    mov [spaceEnd],bx
    mov [spaceEndFrac],ax

    cmp si,0
    jge %%noSaturateStart
    xor si,si
%%noSaturateStart:
    cmp bx,8000
    jl %%noSaturateEnd
    mov bx,8000
%%noSaturateEnd:

%if %1 < fadeSteps-1

    sub bx,si
    jle %%rgbIteration0
    add bx,bx
    add si,si

    add si,movedWipeSequence - redGreenImages
    mov ds,[innerLoopDS]
    jmp [cs:%%rgbIterationsTable + bx]

%%rgbIterationsTable:
%assign iteration 0
%rep maximumIterations
    dw %%rgbIteration%[iteration]
    %assign iteration iteration+1
%endrep

%assign iteration maximumIterations
%rep maximumIterations
%%rgbIteration%[iteration]:
    ; RGB cube:
    ; ds:si = pointer into wipeSequence (*2)
    ; es:di = pointer into updateBuffer
    ; ds:0 (or 0x4000) = redGreen (red even bytes, green odd bytes) combined RGB values from both images, 2 bytes per character = 16000 bytes
    ; es:0 (or 0x4000) = blue = (even bytes, 0 odd bytes) combined RGB values from both images, 2 bytes per character = 16000 bytes
    ; ds:redTable, greenTable, blueTable (all -redGreenImages) = 8*64*3 = 1536 bytes
    ; ds:rgb (at 00000000 000TABLE) = 2048 bytes, 256 byte aligned
    lodsw                                                        ; 3
    stosw                                                        ; 3
    xchg bx,ax                                                   ; 1
    mov dx,[bx]     ; dx =  rrrRRR00 gggGGG00                    ; 4
    mov bx,[es:bx]  ; bx =  bbbBBB00 00000000                    ; 5
    mov al,[bx+blueTable%[step] - redGreenImages]   ; al = 0bbb0000            ; 5
    mov bl,dh                                                    ; 2
    add al,[bx+greenTable%[step] - redGreenImages]  ; al = 0bbbggg0            ; 5
    mov bl,dl                                                    ; 2
    mov ah,[bx+redTable%[step] - redGreenImages]    ; ax = 0bbbggg0 rrrTABLE   ; 5
    xchg ax,si                                                   ; 1
    movsw                                                        ; 5
    xchg ax,si                                                   ; 1
    inc di                                                       ; 1
    inc di                                                       ; 1  ; 201.61 cycles    301 iterations possible in active             29 bytes
    %assign iteration iteration-1
%endrep
%%rgbIteration0:
    mov ax,cs
    mov ds,ax

%else  ; %1 == fadeSteps-1

    sub bx,si
    jle %%lastStepIteration0
    add bx,bx
    add si,si

    add si,movedWipeSequence
    mov cx,ds
    jmp [%%lastStepIterationsTable + bx]

%%lastStepIterationsTable:
%assign iteration 0
%rep maximumIterations
    dw %%lastStepIteration%[iteration]
    %assign iteration iteration+1
%endrep

%assign iteration maximumIterations
%rep maximumIterations
%%lastStepIteration%[iteration]:
    lodsw
    stosw
    xchg bx,ax
    mov ds,[newImageSegment]
    mov ax,[bx]
    stosw
    inc di
    inc di
    mov ds,cx
    %assign iteration iteration-1
%endrep
%%lastStepIteration0:
%endif   ; %1 == fadeSteps-1

%endmacro  ;  doStep

%assign step 1
%rep fadeSteps
  doStep step
  %assign step step+1
%endrep

  add di,blueImages - 2
  mov [updatePointer],di

  mov ax,[transitionFrame]
  inc ax
  cmp ax,totalFrames
  jl noEndTransition
  xor ax,ax
  mov byte[transitionActive],al
noEndTransition:
  mov [transitionFrame],ax

doneOnScreenHandler:
  pop ds
  pop es
  pop bp
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  iret

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

  setPIT0Count 0

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
exit:
  mov ax,0x4c00
  int 0x21

programEnd:

section .bss

bssBase:

stackLow:
  resb 1024
stackHigh:

movedWipeSequence:
  resb 16000

align 16
blueImages:
  resb 16000

updateBuffer:
  resb 368*6 + 1

align 16, resb 1
picturesData:
