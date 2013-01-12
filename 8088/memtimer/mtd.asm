  %include "../defaults_com.asm"

  mov ax,3
  int 0x10

  mov al,0x34
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  cli

  mov ax,0
  mov ds,ax

  mov ax,word[0x20]
  mov word[cs:interrupt8save],ax
  mov ax,word[0x22]
  mov word[cs:interrupt8save+2],ax

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
  call printCharacter
  inc bx
  jmp printLoop
donePrint:
  cmp bx,0
  jne printSpaces

  ; Finish
finish:
  mov ax,0
  mov ds,ax
  mov ax,word[cs:interrupt8save]
  mov word[0x20],ax
  mov ax,word[cs:interrupt8save+2]
  mov word[0x22],ax
  mov ax,0x4c00
  int 0x21

  ; Print spaces for alignment
printSpaces:
  mov cx,21  ; Width of column
  sub cx,bx
  jg spaceLoop
  mov cx,1
spaceLoop:
  mov al,' '
  call printCharacter
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
  call printMessage
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
  call printCharacter

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
  call printMessage
  jmp finish

printMessage:
  push ax
printMessageLoop:
  lodsb
  call printCharacter
  loop printMessageLoop
  pop ax
  ret

printCharacter:
  push ax
  push bx
  push cx
  push dx
  push si
  push di
  push bp
  mov dl,al
  mov ah,2
  int 0x21
  pop bp
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  ret

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations"
outOfSpaceMessageEnd:

interrupt8save: dw 0,0

codePreambleStart:
;  mov al,0
;  mul cl
codePreambleEnd:

experimentData:

experiment1:
  db "inc ax; out dx,al$"
  dw .endInit - ($+2)
  mov dx,0x3d9
  mov ax,0
.endInit
  dw .endCode - ($+2)
  inc ax
  out dx,al
.endCode:

experiment2:
  db "mov al,XX; out dx,al$"
  dw .endInit - ($+2)
  mov dx,0x3d9
  mov ax,0
.endInit
  dw .endCode - ($+2)
  mov al,0x10
  out dx,al
.endCode:

experiment3:
  db "mov ax,XXXX; out dx,ax$"
  dw .endInit - ($+2)
  mov dx,0x3d8
  mov ax,0
.endInit
  dw .endCode - ($+2)
  mov ax,0x002d
  out dx,ax
.endCode:

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

