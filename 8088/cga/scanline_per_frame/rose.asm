%ifdef bin
%include "../../defaults_bin.asm"
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

  in al,0x61
  or al,0x80
  mov [port61high+1],al
  and al,0x7f
  mov [port61low+1],al

  xor ax,ax
  mov ds,ax
  mov ax,[0x20]
  mov [cs:oldInterrupt8],ax
  mov ax,[0x22]
  mov [cs:oldInterrupt8+2],ax
  in al,0x21
  mov [imr],al
  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
  out 0x21,al

  ; Determine and print phase
  lockstep 1
  mov ax,cs
  mov es,ax
  mov ds,ax
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
  xor ax,ax
  mov cx,200
initAddressesLoopTop:
  stosw
  inc ax
  loop initAddressesLoopTop

  mov di,rasterData
  xor ax,ax
  mov cx,200
initRastersLoopTop:
  stosb
;  inc ax
  loop initRastersLoopTop



  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  mov si,data
  xor di,di
  mov cx,8000
  cld
  rep movsw
  mov cx,192
  xor ax,ax
  rep stosw

  lockstep 1

  xor ax,ax
  mov ds,ax
  mov word[0x20],dummyInterrupt8
  mov [0x22],cs
  mov ax,cs
  mov ss,ax
  mov sp,0xfffe

  initCGA 9
  mov dx,0x3d4
  mov ax,0x0009
  out dx,ax
  mov ax,0x0005
  out dx,ax

  mov dl,0xda
  waitForVerticalSync
  waitForDisplayEnable
  mov dl,0xd4
  mov ax,0x0106
  out dx,ax
  mov ax,0x0104
  out dx,ax
  mov ax,0x0101
  out dx,ax
  mov ax,0x0100
  out dx,ax



  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x09
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,15
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             71
  mov ax,0x0100
  out dx,ax

  ;   0xff Horizontal Displayed                         50
  mov ax,0x0101
  out dx,ax

  ;   0xff Horizontal Sync Position                     5a
  mov ax,0x5a02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0003 ;0x0f03
  out dx,ax

  ;   0x7f Vertical Total                               3d
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0206
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x1907
  out dx,ax

  ;   0x03 Interlace Mode                               02   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0008 ; 0x0308
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0009
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x080b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x010d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x3f0e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xff0f
  out dx,ax


  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8
  mov word[0x22],cs
  writePIT16 0, 2, 2         ; Ensure we have a pending IRQ0

;  safeRefreshOff

  mov dx,0x3da
  waitForDisplayDisable
  waitForDisplayEnable

  xor bx,bx
  mov cx,60000


  times 6 nop
;  times 6 nop
;  times 6 nop
;  times 6 nop


  mov dl,0xd4
  mov ax,0x0101
  out dx,ax
  mov ax,0x2100
  out dx,ax
  mov ax,0x5a02
  out dx,ax


  mov bx,[cs:initial]
  add bx,timeSlide
  call bx

  writePIT16 0, 2, 19912     ; Now counting down with the frame, one IRQ0 pending

  ensureRefresh

;  times 9 nop

  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate

  sti
  hlt
interrupt8:
  mov al,0x20
  out 0x20,al
  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8second
  sti
  hlt
interrupt8second:
  mov al,0x20
  out 0x20,al

  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,startAddresses
  mov dx,0x3d4
    pop cx
    mov al,0x0c
    mov ah,ch
    out dx,ax
    inc ax
    mov ah,cl
    out dx,ax
  mov bp,0x5001
  mov di,0x1900
  mov ax,0x5702
  mov si,sampleData
  mov bx,rasterData-sampleData
  mov es,ax

  ; Scanlines 0-199

%macro scanline 1
  mov al,0x00
  out dx,ax        ; e  Horizontal Total         left  0x5700  88
  mov ax,0x0202
  out dx,ax        ; f  Horizontal Sync Position right 0x0202   2

  %if %1 != 199
    pop cx
    mov al,0x0c
    mov ah,ch
    out dx,ax
    inc ax
    mov ah,cl
    out dx,ax
  %else
    mov ax,0x3f04
    out dx,ax      ;    Vertical Total                 0x3f04  64  (2 for scanline 199, 62 for overscan)
    times 9 nop  ; TODO: tune
  %endif

  lodsb
  out 0xe0,al

  %if %1 == 0
    mov ax,0x0104
    out dx,ax      ;    Vertical Total
    times 2 nop
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
%assign i 0
%rep 200
  scanline i
  %assign i i+1
%endrep

  ; Scanline 200

  mov ax,0x7100
  out dx,ax        ; e  Horizontal Total         left  0x7100 114
  mov ax,0x5a02
  out dx,ax        ; f  Horizontal Sync Position right 0x5a02  90

  ; TODO: We are now free to do per-frame vertical-overscan stuff
  ; with no special timing requirements except:
  ;   HLT before overscan is over
  ;   Sound (if in use)

;  mov ax,[frame]
;  inc ax
;  mov [frame],ax
;  cmp ax,600
;  je doneFrame

  mov ax,[increment]
  inc ax
  mov cx,ax
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  add cx,ax
  mov ax,cx
  mov [increment],ax
  mov di,100 ;50
  mul di
  neg ax
  mov dx,ax
  add dx,0x6400

  mov ax,cs
  mov es,ax
  mov di,startAddresses
  mov bh,0
;  mov si,lineTable
%rep 200
  add dx,cx
  mov bl,dh
  mov si,bx
  mov ax,[bx+si+lineTable]
;  mov ax,[bx+si]
  stosw
%endrep

  inc word[frameCount]
  jnz noFrameCountCarry
  inc word[frameCount+2]
noFrameCountCarry:

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
  je tearDown


  mov sp,stackTop
  sti
  hlt


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


timeSlide:
  times 46 nop
  ret


dummyInterrupt8:
  push ax
  mov al,0x20
  out 0x20,al
  pop ax
  iret


initial: dw 3
increment: dw 0
frameCount: dw 0, 0
oldInterrupt8: dw 0, 0
imr: db 0
phase: db 0

lineTable:
%assign i 0
%rep 256
  %if i >= 200
     dw (i&1)*80 + 8000
  %else
     dw (i >> 1)*80 - 2
  %endif
  %assign i i+1
%endrep

align 2

section .bss
data:
  resw 8000

  resw 128
stackTop:
startAddresses:
  resw 200
rasterData:
  resb 200
sampleData:
  resb 200


data2:
