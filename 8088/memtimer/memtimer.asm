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

  sub ax,8880  ; Correct for the 74 cycle multiply: 8880 = 480*74/4

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
  mov al,0
  mul cl
codePreambleEnd:

experimentData:

experiment1:
  db "stosb; xor di,di$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  stosb
  xor di,di
.endCode

experiment2:
  db "xor di,di$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  xor di,di
.endCode

experiment3:
  db "stosw; xor di,di$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  stosw
  xor di,di
.endCode

experiment4:
  db "scasb$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  scasb
.endCode

experiment5:
  db "scasw$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  scasw
.endCode

experiment6:
  db "mov al,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov al,0x12
.endCode

experiment7:
  db "mov cl,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov cl,0x12
.endCode

experiment8:
  db "mov dl,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov dl,0x12
.endCode

experiment9:
  db "mov bl,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov bl,0x12
.endCode

experiment10:
  db "mov ah,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov ah,0x12
.endCode

experiment11:
  db "mov ch,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov ch,0x12
.endCode

experiment12:
  db "mov dh,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov dh,0x12
.endCode

experiment13:
  db "mov bh,0x12$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov bh,0x12
.endCode

experiment14:
  db "mov ax,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov ax,0x1234
.endCode

experiment15:
  db "mov cx,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov cx,0x1234
.endCode

experiment16:
  db "mov dx,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov dx,0x1234
.endCode

experiment17:
  db "mov bx,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov bx,0x1234
.endCode

experiment18:
  db "mov sp,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov sp,0x1234
.endCode

experiment19:
  db "mov bp,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov bp,0x1234
.endCode

experiment20:
  db "mov si,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov si,0x1234
.endCode

experiment21:
  db "mov di,0x1234$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  mov di,0x1234
.endCode

experiment22:
  db "into$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  into
.endCode

experiment23:
  db "aam$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  aam
.endCode

experiment24:
  db "aad$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  aad
.endCode

experiment25:
  db "salc$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  db 0xd6
.endCode

experiment26:
  db "xlatb$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  xlatb
.endCode

experiment27:
  db "loopne +2$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  loopne $+2
.endCode

experiment28:
  db "loope +2$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  loope $+2
.endCode

experiment29:
  db "loop +2$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  loop $+2
.endCode

experiment30:
  db "jcxz +2$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  jcxz $+2
.endCode

experiment31:
  db "in al,0x42$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  in al,0x42
.endCode

experiment32:
  db "in ax,0x42$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  in ax,0x42
.endCode

experiment33:
  db "out 0x42,al$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  out 0x42,al
.endCode

experiment34:
  db "out 0x42,ax$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  out 0x42,ax
.endCode

experiment35:
  db "call +3$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  call $+3
.endCode

experiment36:
  db "jmp +3$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  db 0xe9, 0x00, 0x00
.endCode

experiment37:
  db "jmp +2$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  db 0xeb, 0x00
.endCode

experiment38:
  db "in al,dx$"
  dw .endInit - ($+2)
  mov dx,0x3da
.endInit:
  dw .endCode - ($+2)
  in al,dx
.endCode

experiment39:
  db "in ax,dx$"
  dw .endInit - ($+2)
  mov dx,0x3da
.endInit:
  dw .endCode - ($+2)
  in ax,dx
.endCode

experiment40:
  db "out dx,al$"
  dw .endInit - ($+2)
  mov dx,0x3d9
.endInit:
  dw .endCode - ($+2)
  out dx,al
.endCode

experiment41:
  db "out dx,ax$"
  dw .endInit - ($+2)
  mov dx,0x3d9
.endInit:
  dw .endCode - ($+2)
  out dx,ax
.endCode

experiment42:
  db "lock$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  lock
.endCode

experiment43:
  db "repne$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  repne
.endCode

experiment44:
  db "rep$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  rep
.endCode

experiment45:
  db "cmc$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  cmc
.endCode

experiment46:
  db "clc$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  clc
.endCode

experiment47:
  db "stc$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  stc
.endCode

experiment48:
  db "cli$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  cli
.endCode

experiment49:
  db "nothing$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
;  sti
.endCode

experiment50:
  db "cld$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  cld
.endCode

experiment51:
  db "dtd$"
  dw .endInit - ($+2)
.endInit:
  dw .endCode - ($+2)
  std
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


