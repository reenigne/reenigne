org 0x100
cpu 8086

setupMemory:
  mov ax,cs
  mov ds,ax
  cli
  mov ss,ax
  mov sp,stackHigh
  sti
  add ax,picturesData >> 4
  mov [picturesBinSegment],ax

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
  add ax,1000
  mov [oldImageSegment],ax
  add ax,2000
  mov [newImageSegment],ax
  sti
foregroundTask:
  cmp byte[transitionActive],0
  jne foregroundTask
  ; Transition not active - init a new one


; Process RGB tables from images (bbbgggrr r0000000, BBBGGGRR R0000000) to blueImages/redGreenImages (rrrRRR00 gggGGG00, bbbBBB00 00000000)
processRGB:
  mov cx,8000
  mov si,[oldImageSegment]
  add si,2000
  mov ax,[picturesBinSegment]
  add ax,imageCount*2000
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
.noWRapNew:

  xor di,di
  mov ax,cs
  mov es,ax

.loopTop:
  mov ds,si                                    ; 2
  mov dx,[di]   ; dx = bbbgggrr r0000000       ; 4
  mov ds,bp                                    ; 2
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



align 16, db 0
redGreenImages:

%include "transitionTables.inc"

oldInterrupt8: dw 0, 0
frameCount: dw 0, 0
updateBufferPointer: dw updateBuffer
picturesBinSegment: dw 0
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
  mov es,ax
  mov bx,[cs:updateBufferPointer]
  mov [cs:bx],0xc3  ; ret
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

  mov word[updatePointer],updateBufferStart

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

  mov ax,[spaceStartFrame]
  mov [spaceStart],ax
  mov ax,[spaceEndFrame]
  mov [spaceEnd],ax
  mov ax,[spaceStartFracFrame]
  mov [spaceStartFrac],ax
  mov ax,[spaceEndFracFrame]
  mov [spaceEndFrac],ax

%macro doStep 1
    mov ax,[spaceStart]
    add ax,spaceStepIncrement
    mov bx,[spaceStartFrac]
    add bx,spaceStepFracIncrement
    cmp bx,0
    jge %%noStartWrap
    add bx,denominator
    inc

    mov [spaceStart],ax
%endmacro

%assign step 1
%rep fadeSteps-1
  doStep step
%endrep

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


section .bss

stackLow:
  resb 4096
stackHigh:

movedWipeSequence:
  resb 16000

blueImages:
  resb 16000

updateBuffer:
  resb 368*6 + 1

align 16
picturesData:
