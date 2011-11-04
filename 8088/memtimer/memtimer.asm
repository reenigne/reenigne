org 0
cpu 8086

  mov al,0x34
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  cli

  mov ax,0
  mov ds,ax
  mov ax,cs
  mov word[0x20],interrupt8
  mov [0x22],ax

  mov ds,ax
  mov es,ax
  mov ss,ax
  mov sp,0

  mov si,experimentData
nextExperiment:
  xor bx,bx
  mov [lastQuotient],bx

  ; Print name of experiment
printLoop:
  lodsb
  cmp al,'$'
  je donePrint
  int 0x62
  inc bx
  jmp printLoop
donePrint:
  cmp bx,0
  jne printSpaces

  ; Finish
  int 0x67

  ; Print spaces for alignment
printSpaces:
  mov cx,21  ; Width of column
  sub cx,bx
  jg spaceLoop
  mov cx,1
spaceLoop:
  mov al,' '
  int 0x62
  loop spaceLoop

  mov cx,5    ; Number of repeats
repeatLoop:
  push cx

  mov cx,480+48  ; Number of iterations in primary measurement
  call doMeasurement
  push bx
  mov cx,48      ; Number of iterations in secondary measurement
  call doMeasurement
  pop ax         ; The primary measurement will have the lower value, since the counter counts down
  sub ax,bx      ; Subtract the secondary value, which will be higher, now AX is negative
  neg ax         ; Negate to get the positive difference.

;  int 0x60
;  mov al,' '
;  int 0x62

;  sub ax,8880  ; Correct for the 74 cycle multiply: 8880 = 480*74/4

  xor dx,dx
  mov cx,120
  div cx       ; Divide by 120 to get number of cycles (quotient) and number of extra tcycles (remainder)

  push dx      ; Store remainder

  ; Output quotient
  xor dx,dx
  mov [quotient],ax
  mov cx,10
  div cx
  add dl,'0'
  mov [output+2],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+1],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+0],dl

  ; Output remainder
  pop ax
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+7],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+6],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+5],dl

  ; Emit the final result text
  push si
  mov ax,[quotient]
  cmp ax,[lastQuotient]
  jne fullPrint

  mov cx,6
  mov si,output+4
  jmp doPrint
fullPrint:
  mov [lastQuotient],ax
  mov cx,10
  mov si,output
doPrint:
  int 0x61
  pop si
  pop cx
  loop repeatLoop1

  ; Advance to the next experiment
  lodsw
  add si,ax
  lodsw
  add si,ax

  ; Print a newline
  mov al,10
  int 0x62

  jmp nextExperiment

repeatLoop1:
  jmp repeatLoop

quotient: dw 0
lastQuotient: dw 0

output:
  db "000 +000  "


doMeasurement:
  push si
  push cx  ; Save number of iterations

  ; Copy init
  lodsw    ; Number of init bytes
  mov cx,ax
  mov di,timerStartEnd
  call codeCopy

  ; Copy code
  lodsw    ; Number of code bytes
  pop cx
iterationLoop:
  push cx

  push si
  mov si,codePreambleStart
  mov cx,codePreambleEnd-codePreambleStart
  call codeCopy
  pop si

  push si
  mov cx,ax
  call codeCopy
  pop si

  pop cx
  loop iterationLoop

  ; Copy timer end
  mov si,timerEndStart
  mov cx,timerEndEnd-timerEndStart
  call codeCopy

  ; Turn off refresh
  mov al,0x60  ; Timer 1, write LSB, mode 0, binary
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al

  ; Enable IRQ0
  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
  out 0x21,al

  ; Use IRQ0 to go into lockstep with timer 0
  mov al,0x24  ; Timer 0, write LSB, mode 2, binary
  out 0x43,al
  mov al,0x04  ; Count = 0x0004 which should be after the hlt instruction has
  out 0x40,al  ; taken effect.
  sti
  hlt

  ; The actual measurement happens in the the IRQ0 handler which runs here and
  ; returns the timer value in BX.

  ; Pop the flags pushed when the interrupt occurred
  pop ax

  pop si
  ret

codeCopy:
  cmp cx,0
  je codeCopyDone
codeCopyLoop:
  cmp di,0
  je codeCopyOutOfSpace
  movsb
  loop codeCopyLoop
codeCopyDone:
  ret
codeCopyOutOfSpace:
  mov si,outOfSpaceMessage
  mov cx,outOfSpaceMessageEnd-outOfSpaceMessage
  int 0x61
  int 0x67

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations"
outOfSpaceMessageEnd:


codePreambleStart:
;  mov al,0
;  mul cl
codePreambleEnd:

experimentData:

%macro scanLineSegment 0  ; 15 (measured)
    inc ax
    out dx,al
%endmacro

%macro scanLine 0           ;         304
  %rep 20
    scanLineSegment         ; 20*15 = 300
  %endrep
    sahf                    ;           4
%endmacro

experimentA:
  db "frozen$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
.endInit:
  dw .endCode - ($+2)
  mov si,0x4000-80*25*2
  mov di,si
  mov cx,4
  rep movsw
  times 14 nop
;  cwd
  db 0xeb, 0x00
.endCode

experimentB:
  db "frozenLine$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,0
  mov si,0
  mov di,0
.endInit:
  dw .endCode - ($+2)
  mov cl,6
  rep movsw
.endCode

experimentC:
  db "frozenEnd$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,0
  mov si,0
  mov di,0
.endInit:
  dw .endCode - ($+2)
  mov cl,5
  rep movsw
  mov si,0x4000-80*25*2  ; 3 0    12      10
  mov di,si              ; 2 0     8       3
  nop
  db 0xe9, 0x00, 0x00

.endCode


;experiment0:
;  db "partial$"
;  dw .endInit - ($+2)
;  mov dx,0x03d9
;.endInit:
;  dw .endCode - ($+2)
;  %rep 16
;    inc ax
;    out dx,al
;  %endrep
;  times 2 sahf
;  mov al,3
;  mov cl,0
;  mul cl
;  times 2 sahf
;  mov al,3
;  mov cl,0
;  mul cl
;  db 0xe9, 0x00, 0x00       ;              24
;.endCode
;
;experiment1:
;  db "scanLineSegment$"
;  dw .endInit - ($+2)
;  mov dx,0x03d9
;.endInit:
;  dw .endCode - ($+2)
;  scanLineSegment
;.endCode
;
;experiment2:
;  db "scanLine$"
;  dw .endInit - ($+2)
;  mov dx,0x03d9
;.endInit:
;  dw .endCode - ($+2)
;  scanLine
;.endCode
;
;experiment3:
;  db "oddScanLine$"
;  dw .endInit - ($+2)
;  mov dx,0x03d9
;.endInit:
;  dw .endCode - ($+2)
;%rep 12
;  scanLineSegment           ; 12*15 =     180
;%endrep
;  times 4 sahf
;  mov al,3
;  mov cl,0
;  mul cl
;  db 0xeb, 0x00       ;              24
;.endCode


;experiment1:
;  db "rep lodsb$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep lodsb
;.endCode
;
;experiment2:
;  db "rep lodsw$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep lodsw
;.endCode
;
;experiment3:
;  db "rep stosb$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep stosb
;.endCode
;
;experiment4:
;  db "rep stosw$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep stosw
;.endCode
;
;experiment5:
;  db "rep movsb$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsb
;.endCode
;
;experiment6:
;  db "rep movsw$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsw
;.endCode
;
;experiment7:
;  db "rep cmpsb$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsb
;.endCode
;
;experiment8:
;  db "rep cmpsw$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsw
;.endCode
;
;experiment9:
;  db "rep scasb$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;  mov ax,0x7f7f
;.endInit:
;  dw .endCode - ($+2)
;  rep scasb
;.endCode
;
;experiment10:
;  db "rep scasw$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ds,ax
;  mov ax,0x7f7f
;.endInit:
;  dw .endCode - ($+2)
;  rep scasw
;.endCode
;
;experiment11:
;  db "rep lodsb CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep lodsb
;.endCode
;
;experiment12:
;  db "rep lodsw CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep lodsw
;.endCode
;
;experiment13:
;  db "rep stosb CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep stosb
;.endCode
;
;experiment14:
;  db "rep stosw CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep stosw
;.endCode
;
;experiment15:
;  db "rep movsb CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsb
;.endCode
;
;experiment16:
;  db "rep movsw CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsw
;.endCode
;
;experiment17:
;  db "rep cmpsb CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsb
;.endCode
;
;experiment18:
;  db "rep cmpsw CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsw
;.endCode
;
;experiment19:
;  db "rep scasb CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;  mov ax,0x7f7f
;.endInit:
;  dw .endCode - ($+2)
;  rep scasb
;.endCode
;
;experiment20:
;  db "rep scasw CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ds,ax
;  mov ax,0x7f7f
;.endInit:
;  dw .endCode - ($+2)
;  rep scasw
;.endCode
;
;experiment21:
;  db "rep movsb RAM->CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ax,0x8000
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsb
;.endCode
;
;experiment22:
;  db "rep movsw RAM->CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ax,0x8000
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsw
;.endCode
;
;experiment23:
;  db "rep movsb CGA->RAM$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ax,0xb800
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsb
;.endCode
;
;experiment24:
;  db "rep movsw CGA->RAM$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ax,0xb800
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep movsw
;.endCode
;
;experiment25:
;  db "rep cmpsb RAM->CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ax,0x8000
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsb
;.endCode
;
;experiment26:
;  db "rep cmpsw RAM->CGA$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0xb800
;  mov es,ax
;  mov ax,0x8000
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsw
;.endCode
;
;experiment27:
;  db "rep cmpsb CGA->RAM$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ax,0xb800
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsb
;.endCode
;
;experiment28:
;  db "rep cmpsw CGA->RAM$"
;  dw .endInit - ($+2)
;  mov cx,2048
;  mov ax,0x8000
;  mov es,ax
;  mov ax,0xb800
;  mov ds,ax
;.endInit:
;  dw .endCode - ($+2)
;  rep cmpsw
;.endCode




lastExperiment:
  db '$'


savedSS: dw 0
savedSP: dw 0

timerEndStart:
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al

  mov al,0x54  ; Timer 1, write LSB, mode 2, binary
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  mov al,0x20
  out 0x20,al

  mov ax,cs
  mov ds,ax
  mov es,ax
  mov ss,[savedSS]
  mov sp,[savedSP]

  ; Don't use IRET here - it'll turn interrupts back on and IRQ0 will be
  ; triggered a second time.
  popf
  retf
timerEndEnd:


  ; This must come last in the program so that the experiment code can be
  ; copied after it.

interrupt8:
  pushf
  mov ax,cs
  mov ds,ax
  mov es,ax
  mov [savedSS],ss
  mov [savedSP],sp
  mov ss,ax
  mov dx,0;xffff
  mov cx,0
  mov bx,0
  mov ax,0
  mov si,0
  mov di,0
  mov bp,0
;  mov sp,0

times 528 push cs

  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
timerStartEnd:


