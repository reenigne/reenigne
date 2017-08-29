org 0x100
cpu 8086
programBase:

pitCyclesPerScanline equ 76     ; Fixed by CGA hardware
scanlinesPerFrame    equ 262    ; Fixed by NTSC standard
activeScanlines      equ 200    ; Standard CGA full-screen
visual_profiler      equ 0
onScreenPitCycles    equ pitCyclesPerScanline*activeScanlines - 22
offScreenPitCycles   equ pitCyclesPerScanline*scanlinesPerFrame - (onScreenPitCycles)

%include "tables.inc"

setupMemory:
  mov ax,cs
  mov ds,ax
  cli
  mov ss,ax
  mov sp,stackHigh
  sti

  segmentAdjust equ ((sinTable - programBase) + 0x100)

  add ax,segmentAdjust >> 4
  mov [innerLoopDS],ax

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
  in al,0x61
  or al,0x80
  mov [port61high+1],al
  and al,0x7f
  mov [port61low+1],al

  mov si,image
  mov ax,0xb800
  xor di,di
  mov es,ax
  mov cx,8000
  rep movsw

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
  sti
foregroundTask:
  hlt
  jmp foregroundTask

align 16, db 0

dataTables

oldInterrupt8: dw 0, 0
frameCount: dw 0, 0
imr: db 0
alphaX: dw 0
alphaY: dw 0
betaX: dw 0
betaY: dw 0
innerLoopDS: dw 0


offScreenHandler:
  push ax
  push ds
  push es
  push si
  push di
  mov al,0x20
  out 0x20,al

  xor ax,ax
  mov ds,ax
  mov word[0x20],onScreenHandler

  setPIT0Count onScreenPitCycles

  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  mov si,plasmaData
  mov di,initialUpdateOffset
  updateRoutine

  pop di
  pop si
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
  mov es,ax

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


  ; Plasma inner loop
  ;   Registers:
  ;     SI = pointer into sinTable for alphaX
  ;     BP = pointer into sinTable for betaX
  ;     DX = pointer into sinTable for alphaY
  ;     CX = pointer into sinTable for betaY
  ;     ES:DI = plasmaData buffer pointer
  ;     AL = v
  ;     AH = vy
  ;     BX = gradientTable
  ;     DS:0 = sinTable
  ;     SS:0 = sinTable

%macro plasmaIteration 2
  %if %2 != 0
    add si,%2*5-1
  %else
    dec si
  %endif
  and si,0x1ff
  lodsb
  %if %2 != 0
    add bp,%2*40
  %endif
  and bp,0x1ff
  add al,[bp]
  add al,ah
  xlatb
  %if %1 == 1
    and al,0xf0
  %endif
  stosb                         ; 144 cycles == 422 iterations during active
%endmacro

%macro plasmaIncrementY 0
  add dx,24
  and dx,0x1ff
  mov bx,dx
  mov ah,[bx]
  add cx,3
  and cx,0x1ff
  mov bx,cx
  add ah,[bx]
  mov bx,gradientTable - segmentAdjust
%endmacro

  mov ax,[innerLoopDS]
  mov ds,ax
  mov ss,ax

  mov dx,[alphaY - segmentAdjust]
  add dx,16
  and dx,0x1ff
  mov [alphaY - segmentAdjust],dx
  mov bx,dx
  mov ah,[bx]
  mov cx,[betaY - segmentAdjust]
  dec cx
  and cx,0x1ff
  mov [betaY - segmentAdjust],cx
  mov bx,cx
  add ah,[bx]
  mov bx,gradientTable - segmentAdjust

  mov di,plasmaData
  mov si,[alphaX - segmentAdjust]
  add si,8
  mov [alphaX - segmentAdjust],si
  mov bp,[betaX - segmentAdjust]
  add bp,2
  mov [betaX - segmentAdjust],bp

  plasmaRoutine

  mov ax,cs
  mov ss,ax

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

stackLow:
  resb 1024
stackHigh:

plasmaData:
