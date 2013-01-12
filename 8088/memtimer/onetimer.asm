  %include "../defaults_bin.asm"

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

  mov cx,480+48  ; Number of iterations in primary measurement
  call doMeasurement
  push bx
  mov cx,48      ; Number of iterations in secondary measurement
  call doMeasurement
  pop ax         ; The primary measurement will have the lower value, since the counter counts down
  sub ax,bx      ; Subtract the secondary value, which will be higher, now AX is negative
  neg ax         ; Negate to get the positive difference.

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
  printString
  pop si
  pop cx
  loop repeatLoop1

  ; Advance to the next experiment
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
  push cx  ; Save number of iterations

  ; Copy init
  lodsw    ; Number of init bytes
  mov cx,ax
  mov di,timerStartEnd
  call codeCopy

  ; Copy timer end
  mov si,timerEndStart
  mov cx,timerEndEnd-timerEndStart
  call codeCopy

  pop cx

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
  printString
  complete

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations"
outOfSpaceMessageEnd:


experimentData:

experimeentJJ:
  db "pearce_jj$"
  dw .endCode - ($+2)
  mov dx,0x3d8
  mov ax,0x9000
  mov es,ax
  mov di,0
.l:
  in ax,dx
  stosw
  loop .l
.endCode

experiment1:
  db "samples 1$"
  dw .endCode - ($+2)

  mov al,0x92  ; Timer 2, write LSB, mode 1, binary
  out 0x43,al
  mov dx,0x42
  mov ax,0x8000
  mov ds,ax
  mov si,0
  mov di,0
  sub di,cx
  mov cl,5      ;5
.l:
  pop bx        ;nnnnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000nnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;0000000000nSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  lodsb
  add si,4
  shr al,1
  shr al,1
  and al,0x1f
  out dx,al
  pop bx        ;nnnnnnSSSSSppppp
  shr bx,cl     ;00000nnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;0000000000nSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  dec sp
  pop bx        ;nnnnnnnnnnSSSSSp
  shr bx,1      ;0nnnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;000000nnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000000000SSSSS
  mov al,bl
  out dx,al
  add di,1
  jnz .l
.endCode


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


