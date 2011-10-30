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

  mov cx,10    ; Number of repeats
repeatLoop:
  mov [repeat],cx
  push cx

  mov cx,480     ; Number of iterations
  call doMeasurement
  mov ax,bx
  neg ax         ; Negate to get the positive number of cycles.

 ; sub ax,8880  ; Correct for the 74 cycle multiply: 8880 = 480*74/4

  xor dx,dx
  mov cx,40
  div cx       ; Divide by 40 to get number of hdots (quotient) and number of extra tcycles (remainder)

  push dx      ; Store remainder

  ; Output quotient
  xor dx,dx
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
  db "0000 +00  "

repeat: dw 0

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
  mul cl
codePreambleEnd:

experimentData:

experiment1:
  db "mul 0x00; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x00
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment2:
  db "mul 0x01; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x01
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment3:
  db "mul 0x03; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x03
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment4:
  db "mul 0x07; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x07
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment5:
  db "mul 0x0f; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x0f
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment6:
  db "mul 0x1f; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x1f
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment7:
  db "mul 0x3f; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x3f
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment8:
  db "mul 0x7f; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0x7f
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode

experiment9:
  db "mul 0xff; lodsb; mul 0x00; lodsb$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov ds,ax
  mov cx,480
.loopTop
  mov al,0x00
  stosb
  mov al,0xff
  stosb
  loop .loopTop
  mov si,0
  mov cl,1
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
.endInit:
  dw .endCode - ($+2)
  lodsb
  mul cl
  lodsb
.endCode



lastExperiment:
  db '$'


randomTable:
  db 0xff, 0x7f, 0xff, 0x7f, 0x7f, 0x03, 0xff, 0x03
  db 0x1f, 0x03, 0x00, 0x00, 0xff, 0xff, 0x03, 0x0f
  db 0x0f, 0x01, 0x03, 0xff, 0x1f, 0x1f, 0x03, 0x7f
  db 0x0f, 0xff, 0xff, 0x3f, 0x1f, 0x1f, 0xff, 0x3f
  db 0x00, 0x3f, 0x00, 0x01, 0x1f, 0x0f, 0x01, 0xff
  db 0x07, 0x00, 0xff, 0x0f, 0x01, 0x07, 0x01, 0x03
  db 0x03, 0x01, 0x3f, 0x01, 0x00, 0x00, 0x0f, 0x1f
  db 0x0f, 0x7f, 0x0f, 0x00, 0x1f, 0x03, 0x01, 0x0f
  db 0x07, 0x0f, 0x1f, 0x3f, 0x07, 0xff, 0x7f, 0x7f
  db 0x7f, 0x3f, 0x03, 0x0f, 0x01, 0x00, 0x00, 0x3f
  db 0xff, 0x0f, 0x03, 0x3f, 0xff, 0x01, 0x7f, 0x00
  db 0xff, 0xff, 0x1f, 0x1f, 0x0f, 0x01, 0x03, 0x00
  db 0x7f, 0x7f, 0x01, 0x0f, 0x00, 0x0f, 0x00, 0x3f
  db 0x0f, 0x07, 0x1f, 0x7f, 0x0f, 0xff, 0x03, 0xff
  db 0x07, 0x03, 0x00, 0x03, 0x03, 0xff, 0x7f, 0x01
  db 0x3f, 0x3f, 0x7f, 0x1f, 0x7f, 0x3f, 0x7f, 0xff
  db 0x00, 0x07, 0xff, 0x00, 0x07, 0x7f, 0x0f, 0x0f
  db 0x1f, 0x07, 0x00, 0x7f, 0x3f, 0x01, 0x0f, 0x01
  db 0x3f, 0x0f, 0x0f, 0x7f, 0xff, 0x3f, 0xff, 0x01
  db 0x3f, 0x01, 0x01, 0x01, 0x07, 0x03, 0x00, 0x3f
  db 0x01, 0xff, 0xff, 0x01, 0x1f, 0xff, 0x7f, 0x03
  db 0x03, 0xff, 0x00, 0xff, 0x01, 0x3f, 0x03, 0x7f
  db 0xff, 0x1f, 0x07, 0x7f, 0x00, 0x03, 0x3f, 0x7f
  db 0x00, 0xff, 0x00, 0x00, 0x03, 0x07, 0x03, 0x03
  db 0x00, 0x00, 0x0f, 0x7f, 0x3f, 0x0f, 0x3f, 0x0f
  db 0x00, 0x1f, 0x7f, 0x3f, 0x7f, 0x7f, 0x1f, 0x3f
  db 0xff, 0x3f, 0x01, 0xff, 0x00, 0x00, 0x03, 0x7f
  db 0x1f, 0x01, 0x1f, 0x7f, 0x7f, 0x01, 0xff, 0x3f
  db 0x01, 0x1f, 0x0f, 0x01, 0x03, 0x1f, 0x3f, 0x00
  db 0x03, 0x0f, 0x07, 0x01, 0x7f, 0x1f, 0xff, 0xff
  db 0xff, 0x01, 0x07, 0x03, 0x00, 0xff, 0x00, 0x01
  db 0x01, 0x0f, 0x7f, 0x7f, 0x01, 0x07, 0x0f, 0x1f


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

  ; Wait a random number of cycles to eliminate aliasing effects
  mov bx,[repeat]
  mov al,[bx+randomTable]
  mov cl,1
  mul cl

  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
timerStartEnd:


