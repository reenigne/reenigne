%include "..\defaults_common.asm"

; Set to 1 for emulator, 0 for real hardware (won't work without a proper
; prefetch queue). Unfortunately moving the "mov cl,1" instruction to before
; the patch causes the loop to take more than 288 cycles.
DOSBOX EQU 0

extern song, creditstxt, background

initialScrollPosition equ 2064

segment code

; o must be at address 0 after linking

o:

..start:
  cli

  mov ax,1
  int 0x10

  mov ah,2
  mov bh,0
  mov dh,24
  mov dl,0
  int 0x10


  ; Set video mode to 40-column text mode, black border
  mov dx,0x3da
  waitForVerticalSync
  waitForDisplayEnable
  initCGA 8, 0

  ; Clear screen
  mov ax,0xb800
  mov es,ax
  mov ax,seg background
  mov ds,ax
  xor si,si
  mov di,initialScrollPosition
  mov cx,0x2000
  cld
  rep movsw

  ; Save stack for later
  mov [cs:savedSS],ss
  mov [cs:savedSP],sp

  ; Insert h-instructions into song data
  mov cx,seg song
  add cx,0x1000
  mov es,cx
  mov ax,seg creditstxt
  mov ds,ax
  xor si,si
  mov di,initialScrollPosition/2

  mov dx,initialScrollPosition + 40*24*2  ; Initial draw position
  mov [cs:screenUpdate+2],dx
  mov bh,15 ; Colour
  jmp startLine
insertLoop:
  lodsb
  cmp al,13
  jne doCharacter
  lodsb

startLine:
  push si
  mov ah,0
countCharactersLoop:
  lodsb
  cmp al,0x5b
  jne notColour
  sub ah,3
notColour:
  cmp al,0x5d
  jne notMove
  sub ah,5
notMove:
  cmp al,13
  je doneCountingCharacters
  inc ah
  jmp countCharactersLoop
doneCountingCharacters:
  pop si

  mov al,40
  sub al,ah
  mov ah,0
  and ax,0xfe
  add dx,40*2
  push dx
  add dx,ax
  mov ax,hPointerMove
  call addToHProgram
  pop dx

  push dx
  mov ax,hCrtcUpdate
  add di,40
  and di,0x1fff
  mov dx,di
  call addToHProgram
  pop dx

  jmp insertLoop

doCharacter:
  cmp al,0x5b
  je doColour
  cmp al,0x5d
  je doMove
  cmp al,0xff
  je doNoPlot
  cmp al,'$'
  je doneCredits

  push dx
  mov dh,bh
  mov dl,al
  mov ax,hScreenOutput
  call addToHProgram
  pop dx

  jmp insertLoop

doNoPlot:
  mov ax,hNOP
  call addToHProgram
  jmp insertLoop

doColour:
  lodsb
  shl al,1
  shl al,1
  shl al,1
  shl al,1
  mov bh,al
  lodsb
  and al,0x0f
  or bh,al

  jmp insertLoop

doMove:
  lodsb
  shl al,1
  shl al,1
  shl al,1
  shl al,1
  mov dh,al
  lodsb
  and al,0x0f
  or dh,al
  lodsb
  shl al,1
  shl al,1
  shl al,1
  shl al,1
  mov dl,al
  lodsb
  and al,0x0f
  or dl,al

  mov ax,hPointerMove
  call addToHProgram

  jmp insertLoop

doneCredits:
  mov si,0
  mov ax,di
  xor dx,dx
  mov bx,40
  div bx
  mov dh,al
  mov dl,0
  shr dx,1
crazyScrollLoop:
  add dx,si
  cmp dx,0x6600
  jle dxOk
  sub dx,0x6600
dxOk:
  push dx

  mov ax,dx
  mov cl,7
  shr ax,cl
  mov dx,40
  imul dx

  mov dx,ax
  mov ax,hCrtcUpdate
  call addToHProgram
  pop dx
  add si,2
  jmp crazyScrollLoop


addToHProgram:
  mov word[es:0x0c],ax
  mov word[es:0x0e],dx
  mov cx,es
  inc cx
  mov es,cx
  cmp cx,seg creditstxt
  je finishedHProgram
  ret

finishedHProgram:
  mov ss,[cs:savedSS]
  mov sp,[cs:savedSP]
  mov word[es:0x0c],hFinish



  ; Set DRAM refresh rate to 18 so it synchronizes with the routine
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  ; Enable PC speaker, set PIT to correct mode
  in al,0x61
  or al,3
  out 0x61,al
  mov al,TIMER2 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01
  out 0x42,al  ; Counter 2 count = 1 - terminate count quickly

  ; Set up segments
  mov ax,seg song
  mov ds,ax
  add ax,0x1000
  dec ax
  mov [cs:songP],ax

  mov ax,cs
  mov ss,ax
  mov sp,hStart

  ; Set up other initial registers
  xor bp,bp
  xor si,si
  xor di,di
  xor dx,dx
  mov cx,1

  mov word[cs:mixPatch+2],0
  mov word[cs:mixPatch+12],0
  mov word[cs:mixPatch+22],0
  mov word[cs:mixPatch+32],0
  mov byte[cs:mixPatch+7],0
  mov byte[cs:mixPatch+17],0
  mov byte[cs:mixPatch+27],0
  mov byte[cs:mixPatch+37],0
  mov word[cs:cxPatch],0x01b1

%define nopCount 15

v:
  times nopCount nop
mixPatch:
  add bp,9999  ; 0
  mov bx,bp    ; 4
  mov bl,99    ; 6
  mov al,[bx]  ; 8
  add si,9999  ; 10
  mov bx,si    ; 14
  mov bl,99    ; 16
  add al,[bx]  ; 18
  add di,9999  ; 20
  mov bx,di    ; 24
  mov bl,99    ; 26
  add al,[bx]  ; 28
  add dx,9999  ; 30
  mov bx,dx    ; 34
  mov bl,99    ; 36
  add al,[bx]
  out 0x42,al  ; Output total to speaker
loopPatch:
  loop v
%if DOSBOX != 0
cxPatch:
  mov cl,99    ; Count until next patch
%endif
  pop bx
  pop word[cs:bx]
%if DOSBOX == 0
cxPatch:
  mov cl,99    ; Count until next patch
%endif
endPatch:
  jmp mixPatch

  ; Restore PC speaker to normal, set PIT to correct mode
  in al,0x61
  and al,0xfc
  out 0x61,al
  mov al,TIMER2 | BOTH | MODE3 | BINARY
  out 0x43,al
  mov al,0x00
  out 0x42,al
  out 0x42,al

  ; Restore stack so we can call the interrupt to exit.
  mov ss,[cs:savedSS]
  mov sp,[cs:savedSP]

;  mov ax,3
;  int 0x10
  ; Exit
  mov ax,0x4c00
  int 0x21


savedSS: dw 0
savedSP: dw 0

songP: dw 0

%assign i 0
%rep nopCount
  %assign array%[i] 0x90
  %assign i i+1
%endrep
%assign array%[nopCount] 0x81

%assign gotByte 0
%assign tmpByte 0
%assign vLoc 0
%assign lastCX 1
%assign lastStartAt 0
%assign time 0

%macro restore 0
  %assign time savedTime
  %assign lastStartAt savedLastStartAt
  %assign lastCX savedLastCX
  %assign i 0
  %rep nopCount
    %assign array%[i] savedArray%[i]
    %assign i i+1
  %endrep
%endmacro

%macro advanceTime 0
  %assign time time+1
%endmacro

%macro emitPatch 2.nolist
  dw %1, %2
  %assign time time+1
%endmacro

%macro setCX 1
  %if %1 >= 256
    %error CX too large!
  %endif
  %if lastCX != %1
    emitPatch cxPatch, (%1 << 8) | 0xb1  ; mov cl,XX
  %endif
  %assign lastCX %1
%endmacro

; This is where (relative to v) we will start patching to. We'll need to use
; something other than 0 here if our patched instructions take longer to run
; than the equivalent length of NOPs.
%macro startAt 1
  %assign vLoc %1
  %if lastStartAt != %1
    emitPatch loopPatch, 0xe2 | (((v + %1)-(loopPatch + 2)) << 8)  ; loop v+vLoc
  %endif
  %assign lastStartAt %1
%endmacro

; This will finish and run the v-instruction. It will take care of erasing the
; old v-instruction and padding with NOPs.
; The argument is the number of times to run the v-instruction (accomplished by
; setting CX appropriately). 0 to run until the end of the tick.
%macro runV 1
  %rep nopCount
  ;  %warning vLoc nopCount gotByte
    %if ((vLoc == nopCount-1) && (gotByte != 0)) || (vLoc == nopCount)
;      %warning exitrep
      %exitrep
    %else
;      %warning about to pad 0x90
      b 0x90  ; Pad with NOPs
    %endif
  %endrep
  %if vLoc == nopCount-1 && gotByte != 0
;    %warning about to pad 0x81
    b 0x81  ; Squeeze out the final byte in the vInstruction area by overwriting the first byte of mixPatch
  %endif
  %assign gotByte 0
  %assign vLoc lastStartAt
  %assign cycles %1
  %if cycles == 0
;    %if DOSBOX != 0
;      %assign cycles 1  ; To simplify debugging
;    %else
      %assign cycles 331-(2+time)  ; Adjust by 2 because there are two setCX patches that need to follow
;    %endif
  %endif
  setCX 1+cycles
  %assign time time+cycles
  setCX 1
%endmacro

%macro forget 1-*
  %rep %0
    %assign i %1+lastStartAt
    %assign array%[i] -1
    %rotate 1
  %endrep
%endmacro

; This will emit bytes to the v-instruction.
%macro b 1-*
;  %warning adding %1
  %rep %0
    %if gotByte == 0
      %if array%[vLoc] == %1
        %assign vLoc vLoc+1
      %else
        %assign tmpByte %1
        %assign gotByte 1
      %endif
    %else
      %if vLoc == nopCount || (vLoc == nopCount-1 && %1 != 0x81)
        %error Too many bytes in vInstruction! vLoc nopCount %1 tmpByte
      %endif
      emitPatch v+vLoc, (%1 << 8) | tmpByte
      %assign gotByte 0
      %assign array%[vLoc] tmpByte
      %assign vLoc vLoc+1
      %assign array%[vLoc] %1
      %assign vLoc vLoc+1
    %endif
    %rotate 1
  %endrep
%endmacro

; Flush any cached byte so that the next word will be whole
%macro flush 0
  %if gotByte != 0
    b 0x90
    %assign vLoc vLoc-1
  %endif
%endmacro

; This will emit a word to the v-instruction.
%macro w 1
  b ((%1) & 0xff)
  b ((%1) >> 8)
%endmacro

; This will emit a label to the v-instruction.
%macro l 1
  w %1-o
%endmacro

; This just allows us to write multiple instructions on one source line.
%macro multi 1-*.nolist
  %rep %0
    %1
    %rotate 1
  %endrep
%endmacro

; pseudo-assembly instructions used in the v-instructions
%define CS_       {b 0x2e}
%define ES_       {b 0x26}
%define SS_       {b 0x36}
%define INC_mw    {b 0xff, 0x06}
%define MOV_ES_mw {b 0x8e, 0x06}
%define MOV_AL_xb {b 0xa0}
%define MOV_AX_xw {b 0xa1}
%define MOV_xb_AL {b 0xa2}
%define MOV_xw_AX {b 0xa3}
%define MOV_SP_mw {b 0x8b, 0x26}
%define MOV_BX_DX {b 0x89, 0xd3}
%define MOV_DX_iw {b 0xba}
%define MOV_AX_iw {b 0xb8}
%define OUT_DX_AX {b 0xef}
%define MOV_DX_BX {b 0x89, 0xda}
%define MOV_SP_iw {b 0xbc}
%define MOV_ES_AX {b 0x8e, 0xc0}
%define MOV_mw_iw {b 0xc7, 0x06}


; Each line in a v-instruction in the v-program. ES is preservaed across
; v-instructions, other registers are stomped. Registers CX, BX, SI, DI, BP
; and DS must be preserved. SS:SP is the current v-instruction pointer.

; Each v-instruction (including any padding NOPs at the end) must take the same
; amount of time to run as nopCount NOPs.

; We do the waveform and frequency updates as close together as possible so
; that all 4 channels switch note at as close as possible to the same time.
hStart:
  w0: emitPatch mixPatch+6,  0x00b3                                                                             ; w0: waveform0 = "mov bl, xx"
  f0: emitPatch mixPatch+2,  0                                                                                  ; f0: frequency0 = xxxx
  w1: emitPatch mixPatch+16, 0x00b3                                                                             ; w1: waveform1 = "mov bl, xx"
  f1: emitPatch mixPatch+12, 0                                                                                  ; f1: frequency1 = xxxx
  w2: emitPatch mixPatch+26, 0x00b3                                                                             ; w2: waveform2 = "mov bl, xx"
  f2: emitPatch mixPatch+22, 0                                                                                  ; f2: frequency2 = xxxx
  w3: emitPatch mixPatch+36, 0x00b3                                                                             ; w3: waveform3 = "mov bl, xx"
  f3: emitPatch mixPatch+32, 0                                                                                  ; f3: frequency3 = xxxx
  multi startAt 7, CS_, INC_mw, l songP,                                                                runV 1  ; inc word[cs:songP]                                                 3 NOPs remain
  multi startAt 4, CS_, MOV_ES_mw, l songP,                                                             runV 1  ; mov es,[cs:songP]                                                  6
  multi startAt 4, ES_, MOV_AL_xb, w 0x0,  SS_, MOV_xb_AL, l w0+3,                                      runV 1  ; mov al,[es:0x0]  mov [ss:w0+3],al                                  3
  multi startAt 4, ES_, MOV_AL_xb, w 0x3,  SS_, MOV_xb_AL, l w1+3,                                      runV 1  ; mov al,[es:0x3]  mov [ss:w1+3],al                                  3
  multi startAt 4, ES_, MOV_AL_xb, w 0x6,  SS_, MOV_xb_AL, l w2+3,                                      runV 1  ; mov al,[es:0x6]  mov [ss:w2+3],al                                  3
  multi startAt 4, ES_, MOV_AL_xb, w 0x9,  SS_, MOV_xb_AL, l w3+3,                                      runV 1  ; mov al,[es:0x9]  mov [ss:w3+3],al                                  3
  multi startAt 6, ES_, MOV_AX_xw, w 0x1,  SS_, MOV_xw_AX, l f0+2,                                      runV 1  ; mov ax,[es:0x1]  mov [ss:f0+2],ax                                  1
  multi startAt 6, ES_, MOV_AX_xw, w 0x4,  SS_, MOV_xw_AX, l f1+2,                                      runV 1  ; mov ax,[es:0x4]  mov [ss:f1+2],ax                                  1
  multi startAt 6, ES_, MOV_AX_xw, w 0x7,  SS_, MOV_xw_AX, l f2+2,                                      runV 1  ; mov ax,[es:0x7]  mov [ss:f2+2],ax                                  1
  multi startAt 6, ES_, MOV_AX_xw, w 0xa,  SS_, MOV_xw_AX, l f3+2,                                      runV 1  ; mov ax,[es:0xa]  mov [ss:f3+2],ax                                  1
  multi startAt 4, ES_, MOV_SP_mw, w 0xc,                                                               runV 1  ; mov sp,[es:0xc]                                                    6

%assign savedTime time
%assign savedLastStartAt lastStartAt
%assign savedLastCX lastCX
%assign i 0
%rep nopCount
  %assign savedArray%[i] array%[i]
  %assign i i+1
%endrep

; Each of these blocks forms an h-instruction. One h-instruction is executed
; every 20ms. The h-instruction and its operand are stored in bytes 12-15 of
; the song data record.

; We can't load the operand from the song data and update the CRTC register in
; the same v-instruction as it would take too long. Therefore we need to break
; it up like this.
hCrtcUpdate:
  restore
  multi startAt 4, ES_, MOV_AL_xb, w 0xe,  SS_, MOV_xb_AL, l hCrtcUpdate+0x37,                          runV 1  ; mov al,[es:0xe]  mov [ss:sa1+7],al                                 3
  multi startAt 4, ES_, MOV_AL_xb, w 0xf,  SS_, MOV_xb_AL, l hCrtcUpdate+0x4b,                          runV 1  ; mov al,[es:0xf]  mov [ss:sa2+7],al                                 3
  multi forget 7, startAt 4, MOV_BX_DX, MOV_DX_iw, w 0x3d4, MOV_AX_iw, w 0x990d, OUT_DX_AX, MOV_DX_BX,  runV 1  ; sa1: mov bx,dx  mov dx,0x3d4 mov ax,0xXX0c  out dx,ax  mov dx,bx   0
  multi forget 7, startAt 4, MOV_BX_DX, MOV_DX_iw, w 0x3d4, MOV_AX_iw, w 0x990c, OUT_DX_AX, MOV_DX_BX,  runV 1  ; sa2: mov bx,dx  mov dx,0x3d4 mov ax,0xXX0d  out dx,ax  mov dx,bx   0
  multi forget 7, startAt 0, MOV_SP_iw, l hStart,                                                       runV 0  ; mov sp,hStart                                                     12

hScreenOutput:
  restore
  multi startAt 6, ES_, MOV_AX_xw, w 0x0e,  SS_, MOV_xw_AX, l hScreenOutput+0x56,                       runV 1  ; mov ax,[es:0xe]  mov [ss:screenUpdate+5],ax                        1
  multi startAt 0, MOV_AX_iw, w 0xb800, MOV_ES_AX,                                                      runV 1  ; mov ax,0xb800  mov es,ax                                          10
  multi {forget 3, 4, 5, 6}, startAt 7, ES_, MOV_mw_iw, flush,
  screenUpdate: multi w 0, w 0,                                                                         runV 1  ; screenUpdate: mov [es:xxxx],yyyy                                   1
  multi {forget 3, 4, 5, 6}, startAt 7, CS_, INC_mw, l screenUpdate+2,                                  runV 2  ; inc word[cs:screenUpdate+3] *twice*                                3
  multi startAt 0, MOV_SP_iw, l hStart,                                                                 runV 0  ; mov sp,hStart                                                     12

hPointerMove:
  restore
  multi startAt 6, ES_, MOV_AX_xw, w 0x0e,  SS_, MOV_xw_AX, l screenUpdate+2,                           runV 1  ; mov ax,[es:0xe]  mov [ss:screenUpdate+3],ax                        1
  multi startAt 0, MOV_SP_iw, l hStart,                                                                 runV 0  ; mov sp,hStart                                                     12

hFinish:
  restore
  emitPatch endPatch, 0x00eb                                                                                    ; "jmp mixPatch" = "jmp $+2"
  emitPatch endPatch, 0x00eb                                                                                    ; Just in case of prefetch queue difficulties

hNOP:
  restore
  multi startAt 0, MOV_SP_iw, l hStart,                                                                 runV 0  ; mov sp,hStart                                                     12

segment STACK STACK class=STACK
  resb 256

