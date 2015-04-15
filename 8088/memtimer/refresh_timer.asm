  %include "../defaults_bin.asm"

REFRESH_RATE EQU 18
ITERATIONS EQU 120

  mov al,TIMER0 | BOTH | MODE2 | BINARY
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
  printCharacter
  inc bx
  jmp printLoop
donePrint:
  cmp bx,0
  jne printSpaces

  ; Finish
  complete

  ; Print spaces for alignment
printSpaces:
  mov cx,21  ; Width of column
  sub cx,bx
  jg spaceLoop
  mov cx,1
spaceLoop:
  printCharacter ' '
  loop spaceLoop

  mov cx,5    ; Number of repeats
repeatLoop:
  push cx

  mov cx,ITERATIONS*4+48  ; Number of iterations in primary measurement
  call doMeasurement
  push bx
  mov cx,48      ; Number of iterations in secondary measurement
  call doMeasurement
  pop ax         ; The primary measurement will have the lower value, since the counter counts down
  sub ax,bx      ; Subtract the secondary value, which will be higher, now AX is negative
  neg ax         ; Negate to get the positive difference.

  xor dx,dx
  mov cx,ITERATIONS
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
  printString
  pop si
  pop cx
  loop repeatLoop1

  ; Advance to the next experiment
  lodsw
  add si,ax
  lodsw
  add si,ax

  printNewLine

  jmp nextExperiment

repeatLoop1:
  jmp repeatLoop

quotient: dw 0
lastQuotient: dw 0

output:
  db "000 +000  "


doMeasurement:
  push si
;  push cx
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
  mov cx,ax
  call codeCopy
  pop si

  pop cx
  loop iterationLoop

  ; Copy timer end
  mov si,timerEndStart
  mov cx,timerEndEnd-timerEndStart
  call codeCopy


  ; Enable IRQ0
  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
  out 0x21,al

  mov cx,256
  cli

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  ; Delay for enough time to refresh 512 columns
  rep lodsw

  ; We now have about 1.5ms during which refresh can be off
  refreshOff

  ; Use IRQ0 to go into lockstep with timer 0
  mov al,TIMER0 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,0x04  ; Count = 0x0004 which should be after the hlt instruction has
  out 0x40,al  ; taken effect.

;  pop cx

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
  printString
  complete

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations"
outOfSpaceMessageEnd:


experimentData:

%assign nops -1
%rep 18
%assign nops nops+1

experimentSIDPatch %+ nops:
  db "SIDPatch$"
  dw .endInit - ($+2)

  mov ax,0x8000
  mov ds,ax
  mov ss,ax
  mov cx,1
  mov ah,1
  times nops nop

.endInit:
  dw .endCode - ($+2)

  add bp,0x0f0f
  mov bx,bp
  mov bl,0x0f
  mov al,[bx]
  add si,0x0f0f
  mov bx,si
  mov bl,0x0f
  add al,[bx]
  add di,0x0f0f
  mov bx,di
  mov bl,0x0f
  add al,[bx]
  add dx,0x0f0f
  mov bx,dx
  mov bl,0x0f
  add al,[bx]
  out 0xe0,al
  loop .endInit+2

  pop bx
  pop word[cs:bx]
  mov cl,1

  jmp $+2

.endCode


;experimentSID_CS_INC_mw %+ nops:
;  db "SID_CS_INC_mw$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  inc word[cs:0]
;  times 3 nop
;
;.endCode
;
;
;experimentSID_CS_MOV_ES_mw %+ nops:
;  db "SID_CS_MOV_ES_mw$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov es,[cs:0]
;  times 6 nop
;
;.endCode
;
;
;experimentSID_mem_mem_byte %+ nops:
;  db "SID_mem_mem_byte$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov es,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov al,[es:0]
;  mov [ss:0],al
;  times 3 nop
;
;.endCode
;
;
;experimentSID_mem_mem_word %+ nops:
;  db "SID_mem_mem_word$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov es,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov ax,[es:0]
;  mov [ss:0],ax
;  times 1 nop
;
;.endCode
;
;
;experimentSID_ES_MOV_SP_mw %+ nops:
;  db "SID_ES_MOV_SP_mw$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov es,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov sp,[es:0]
;  times 6 nop
;
;.endCode
;
;
;experimentSID_CRTC %+ nops:
;  db "SID_CRTC$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov bx,dx
;  mov dx,0x3d4
;  mov ax,0x000c
;  out dx,ax
;  mov dx,bx
;;  times 4 nop
;
;.endCode
;
;
;experimentSID_MOV_SP_iw %+ nops:
;  db "SID_MOV_SP_iw$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov sp,1234
;  times 12 nop
;
;.endCode
;
;
;experimentSID_load_ES %+ nops:
;  db "SID_load_ES$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov cx,0xffff
;  mov ah,1
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov ax,0xb800
;  mov es,ax
;  times 10 nop
;
;.endCode
;
;
;experimentSID_draw %+ nops:
;  db "SID_draw$"
;  dw .endInit - ($+2)
;
;  mov ax,0x8000
;  mov ds,ax
;  mov ss,ax
;  mov cx,0xffff
;  mov ah,1
;  mov ax,0xb800
;  mov es,ax
;  times nops nop
;
;.endInit:
;  dw .endCode - ($+2)
;
;  add bp,0x0f0f
;  mov bx,bp
;  mov bl,0x0f
;  mov al,[bx]
;  add si,0x0f0f
;  mov bx,si
;  mov bl,0x0f
;  add al,[bx]
;  add di,0x0f0f
;  mov bx,di
;  mov bl,0x0f
;  add al,[bx]
;  add dx,0x0f0f
;  mov bx,dx
;  mov bl,0x0f
;  add al,[bx]
;  out 0xe0,al
;  loop $+2
;
;  mov word[es:0x3ffe],1234
;  times 1 nop
;
;.endCode

%endrep

experimentHLT:
  db "HLT$"
  dw .endInit - ($+2)

.endInit:
  dw .endCode - ($+2)

  hlt

.endCode



experimentVScroll:
  db "VScroll$"
  dw .endInit - ($+2)
  mov di,8192
  rep movsw
.endInit:
  dw .endCode - ($+2)
.endCode:

experimentHScroll2:
  db "HScroll$"
  dw .endInit - ($+2)
  mov di,8192
.endInit:
  dw .endCode - ($+2)
  movsw
  add di,bx
  add si,bx
.endCode:

experimentHScroll1:
  db "HScroll1$"
  dw .endInit - ($+2)
  mov di,8192
.endInit:
  dw .endCode - ($+2)
  movsw
  add di,bx
.endCode:

experimentHScroll0:
  db "HScroll0$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov ds,ax
  mov di,8192
.endInit:
  dw .endCode - ($+2)
  mov ax,[bp+1234]
  mov [si+2345],ax
.endCode:

experimentLine10:
  db "Line10$"
  dw .endInit - ($+2)
  mov di,8192
.endInit:
  dw .endCode - ($+2)
  times 10 movsw
  add di,bx
.endCode:

experimentLine9:
  db "Line9$"
  dw .endInit - ($+2)
  mov di,8192
.endInit:
  dw .endCode - ($+2)
  times 9 movsw
  add di,bx
.endCode:

experimentLine8:
  db "Line8$"
  dw .endInit - ($+2)
  mov di,8192
.endInit:
  dw .endCode - ($+2)
  times 8 movsw
  add di,bx
.endCode:

experimentLine9System:
  db "Line9System$"
  dw .endInit - ($+2)
  mov ax,ds
  mov es,ax
.endInit:
  dw .endCode - ($+2)
  times 9 movsw
  add di,bx
.endCode:

experimentLine8System:
  db "Line8System$"
  dw .endInit - ($+2)
  mov ax,ds
  mov es,ax
.endInit:
  dw .endCode - ($+2)
  times 8 movsw
  add di,bx
.endCode:

experimentHScrollPrep:
  db "HScrollPrep$"
  dw .endInit - ($+2)
  mov ax,ds
  mov es,ax
.endInit:
  dw .endCode - ($+2)
  movsw
  add si,bx
.endCode:



lastExperiment:
  db '$'


savedSS: dw 0
savedSP: dw 0
dummy: dw 0

timerEndStart:
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al

  refreshOn

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

  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,REFRESH_RATE
  out 0x41,al  ; Timer 1 rate

  mov ax,cs
  mov ds,ax
  mov [savedSS],ss
  mov [savedSP],sp

  mov ax,0x8000
  mov ds,ax
  mov ax,0x7000
  mov ss,ax
  mov ax,0xb800
  mov es,ax
  xor ax,ax
  mov dx,ax
  mov bx,ax
  mov si,ax
  mov di,ax
  mov bp,ax
  mov sp,0xfffe
  cld


times 528 push cs

  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
timerStartEnd:


