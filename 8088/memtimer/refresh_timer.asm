  %include "../defaults_bin.asm"

REFRESH_RATE EQU 19
ITERATIONS EQU 120

  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  cli

  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8null
  mov ax,cs
  mov [0x22],ax

  mov ds,ax
  mov es,ax
  mov ss,ax
  xor sp,sp

  sti

  mov si,experimentData
nextExperiment:
  xor bx,bx
  mov [lastQuotient],bx

  ; Print name of experiment
printLoop:
  lodsb
  cmp al,'$'
  je donePrint
  outputCharacter
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
  outputCharacter ' '
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
  outputString
  pop si
  pop cx
  loop repeatLoop1

  ; Advance to the next experiment
  lodsw
  add si,ax
  lodsw
  add si,ax

  outputNewLine

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

  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8

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
  outputString
  complete

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations"
outOfSpaceMessageEnd:


experimentData:

experimentUpdateMusic0:
  db "UpdateMusic0$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
    mov ax,cs
    mov es,ax
    mov es,ax  ; "More magic"
.endCode:

experimentUpdateMusic1:
  db "UpdateMusic1$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
    mov bx,0
    cmp bx,[cs:0]
    je $+2
    mov es,[cs:1]
    mov si,[cs:bx]
    inc bx
    inc bx
    mov [cs:9999],bx
    mov ax,cs
    mov es,ax
    lodsb
    out 0xe0,al
.endCode:

experimentUpdateMusic:
  db "UpdateMusic$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)

    push   si
    push   ds
    lds    si, [cs:9999]
    lodsb
    out    0xe0, al
    lodsb
    out    0xe0, al
    pop    ds
    mov    [cs:9999], si
    pop    si

.endCode:

experimentUpdateLake_Nop:
  db "Lake_Nop$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
.endInit:
  dw .endCode - ($+2)

  times 65 nop

  lodsb
  out 0xe0,al

.endCode:

experimentUpdateLake_261:
  db "Lake_261$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
  mov dx,0x3d4
  mov bx,[cs:1236]
.endInit:
  dw .endCode - ($+2)

  lodsb

  times 42 nop

  mov si,1234
  cmp si,1235
  je .endCode
  inc si
  inc si
  mov [cs:1236],bx
  mov si,[cs:si]

  out 0xe0,al
.endCode:


experimentUpdateLake_262:
  db "Lake_262$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
  mov dx,0x3d4
  mov bx,[cs:1236]
.endInit:
  dw .endCode - ($+2)

;  lodsb

;  mov si,1234
;  cmp si,1235
;  je .endCode
;  inc si
;  inc si
;  mov [cs:1236],bx
;  mov si,[cs:si]

  out 0xe0,al

  jmp near .interrupt8
.interrupt8:
  mov cx,ds
  mov ds,cx
  mov ds,cx
  mov ax,1234
  mov ax,1234
  mov ax,1234
  mov ax,1234
  mov ax,1234
  mov ax,1234
  mov ax,1234
  mov ds,cx

  mov al,0
  out dx,ax

  mov ax,0x0202
  out dx,ax

  pop cx
  mov al,0x0c
  mov ah,ch
  out dx,ax
  inc ax
  mov ah,cl
  out dx,ax

  lodsb

.endCode:


experimentUpdateLake_200a:
  db "Lake_200a$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
  mov dx,0x3d4
  mov bp,1234
.endInit:
  dw .endCode - ($+2)

  lodsb
  out 0xe0,al

  mov bx,[cs:1234]
  inc cx
  inc cx
  cmp bp,1234
  jne .noRestart
  mov cx,1235
.noRestart:
  mov [cs:1234],bp
  mov cx,[cs:bx]
  add cx,97
  mov ax,[bp-97]
  stosw
  mov ax,[bp-95]
  stosw
  mov ax,[bp-93]
  stosw
  times 6 nop

.endCode:

experimentUpdateLake_200b:
  db "Lake_200b$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
  mov dx,0x3d4
  mov bp,1234
.endInit:
  dw .endCode - ($+2)

  lodsb
  out 0xe0,al

  mov bx,[cs:1234]
  inc cx
  inc cx
  cmp bp,1234
  je .noRestart
  mov cx,1235
.noRestart:
  mov [cs:1234],bx
  mov cx,[cs:bp]
  add cx,97
  mov ax,[bp-97]
  stosw
  mov ax,[bp-95]
  stosw
  mov ax,[bp-93]
  stosw
  times 6 nop

.endCode:


experimentUpdateLake_199a:
  db "Lake_199a$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)

  lodsb
  out 0xe0,al

  mov ax,0x3f04
  out dx,ax
  times 3 nop
  mov ax,0x0101
  out dx,ax
  xchg ax,di
  out dx,ax
  xchg ax,di
  xchg ax,bp
  out dx,ax
  xchg ax,bp
  mov ax,es
  out dx,ax

  mov ax,0x7100
  out dx,ax        ; e  Horizontal Total         left  0x7100 114
  mov ax,0x5a02
  out dx,ax        ; f  Horizontal Sync Position right 0x5a02  90

  mov bp,1234

  mov di,1234
  mov ax,ds
  mov es,ax
  nop
  nop
  nop
  nop
  nop
.endCode:


experimentUpdateLake_7:
  db "Lake_7$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
.endInit:
  dw .endCode - ($+2)

  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw

  lodsb
  out 0xe0,al

.endCode:


experimentUpdateLake_4:
  db "Lake_4$"
  dw .endInit - ($+2)
  mov ax,0x7000
  mov es,ax
  mov ds,ax
  xor di,di
  xor si,si
.endInit:
  dw .endCode - ($+2)

  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  mov ax,[bp-1]
  stosw
  times 30 nop

  lodsb
  out 0xe0,al

.endCode:



experimentUpdate4_6:
  db "Update4_6$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  xor di,di
.endInit:
  dw .endCode - ($+2)

  mov al,[bx]
  inc bx
  out 0xe0,al
  lodsw
  xchg ax,di
  movsw
  lodsw
  xchg ax,di
  movsw
  lodsw
  xchg ax,di
  movsw
  lodsw
  xchg ax,di
  movsw
  nop
  nop
  nop
  nop
  nop
  nop

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

  xor ax,ax
  mov ds,ax
  mov word[0x20],interrupt8null

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

interrupt8null:
  push ax
  mov al,0x20
  out 0x20,al
  pop ax
  iret


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


