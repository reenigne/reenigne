  %include "../defaults_common.asm"
  org 0x100

%macro outputCharacter 0
  call doPrintCharacter
%endmacro

  ; Find name of tests file
  mov si,0x81
findEndOfFileName:
  lodsb
  cmp al,0x20
  jne .notSpace
  mov byte[si-1],0
.notSpace:
  cmp al,0x0d
  jne findEndOfFileName
.found:
  mov byte[si-1],0
  mov al,0
  cmp si,0x82
  je error

  ; Open tests file
  mov ax,0x3d00
  mov dx,0x82
  int 0x21
  jnc noError
error:
  add al,0x20
  mov ah,0x0e
  int 0x10
  mov ax,0x4c01
  int 0x21
noError:
  mov bx,ax
  xor cx,cx
  xor dx,dx
  ; Move file pointer to end
  mov ax,0x4202
  int 0x21
  jc error
  cmp dx,0
  jne error
  mov [testsLength],ax

  ; Move file pointer to start
  mov ax,0x4200
  int 0x21
  jc error

  ; Read tests file
  mov ah,0x3f
  mov cx,[testsLength]
  mov dx,experimentData
  int 0x21
  jc error

  ; Close tests file
  mov ah,0x3e
  int 0x21
  jc error


  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  mov ax,0
  mov ds,ax

  cli
  mov ax,word[0x20]
  mov word[cs:interrupt8save],ax
  mov ax,word[0x22]
  mov word[cs:interrupt8save+2],ax
  sti

  mov ax,cs
  mov ds,ax
  mov es,ax
  mov ss,ax
  mov sp,0

  mov si,experimentData
nextExperiment:
  xor bx,bx
  mov [lastQuotient],bx

  mov ax,si
  sub ax,experimentData
  cmp ax,[testsLength]
  jne printLoop

  ; Finish
finish:
  mov ax,0x4c00
  int 0x21

  ; Print name of experiment
printLoop:
  lodsb
  cmp al,'$'
  je donePrint
  outputCharacter
  inc bx
  jmp printLoop
donePrint:

  ; Print spaces for alignment
printSpaces:
  mov cx,21  ; Width of column
  sub cx,bx
  jg spaceLoop
  mov cx,1
spaceLoop:
  mov al,' '
  outputCharacter
  loop spaceLoop

  lodsb
  cmp al,0
  je noRefresh
  mov byte[pitMode],TIMER1 | LSB | MODE2 | BINARY
  mov [pitCount],al
  jmp doneRefresh
noRefresh:
  mov byte[pitMode],TIMER1 | MSB | MODE0 | BINARY
  mov byte[pitCount],1
doneRefresh:

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
  outputCharacter

  jmp nextExperiment

repeatLoop1:
  jmp repeatLoop

testsLength: dw 0

quotient: dw 0
lastQuotient: dw 0

output:
  db "000 +000  "


doPrintCharacter:
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


doMeasurement:
  push si
  push cx  ; Save number of iterations
  push cx

  mov di,experimentData
  add di,[testsLength]

  ; Copy timer start
  push si
  mov si,timerStartStart
  mov cx,timerStartEnd-timerStartStart
  call codeCopy
  pop si

  ; Copy init
  lodsw    ; Number of init bytes
  mov cx,ax
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

  ; Turn off refresh
;  refreshOff

  mov di,experimentData
  add di,[testsLength]
  cli
  mov ax,0
  mov ds,ax
  mov ax,cs
  mov [0x20],di
  mov [0x22],ax
  mov ds,ax
  mov byte[doneMeasurement],0

  pop cx

  ; Enable IRQ0
;  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
;  out 0x21,al

  ; Use IRQ0 to go into lockstep with timer 0
;  mov al,TIMER0 | LSB | MODE2 | BINARY
;  out 0x43,al
;  mov al,0x40  ; Count = 0x0004 which should be after the hlt instruction has
;  out 0x40,al  ; taken effect.
waitForIRQ0:
  sti
  hlt
  ; The actual measurement happens in the the IRQ0 handler which runs here and
  ; returns the timer value in BX.
  cmp byte[doneMeasurement],1
  jne waitForIRQ0

  mov ax,0
  mov ds,ax
  mov ax,word[cs:interrupt8save]
  mov word[0x20],ax
  mov ax,word[cs:interrupt8save+2]
  mov word[0x22],ax
  mov ax,cs
  mov ds,ax
  sti

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
  outputCharacter
  loop printMessageLoop
  pop ax
  ret

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations"
outOfSpaceMessageEnd:

interrupt8save: dw 0,0

savedSS: dw 0
savedSP: dw 0
doneMeasurement: db 0
pitMode: db 0
pitCount: db 0


timerStartStart:
  push bp
  mov bp,sp
  and word[bp+6],0xfdff  ; Turn off interrupts on return so that the interrupt doesn't fire again

  mov ax,cs
  mov ds,ax
  mov byte[doneMeasurement],1
  mov [savedSS],ss
  mov [savedSP],sp

  mov al,[pitMode]
  out 0x43,al
  mov al,[pitCount]
  out 0x41,al

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
  mov sp,ax

  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
timerStartEnd:


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

  pop bp
  iret
timerEndEnd:


experimentData:

