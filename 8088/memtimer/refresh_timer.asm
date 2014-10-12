  %include "../defaults_bin.asm"

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


codePreambleStart:
;  mov al,0
;  mul cl
codePreambleEnd:

experimentData:

experimentSlide:
  db "IntroSlide$"
  dw .endInit - ($+2)
  mov dx,0x3d4
  mov si,0x3d9
  mov ax,0xb800
  mov es,ax
.endInit:
  dw .endCode - ($+2)

  add sp,[bx]   ; 2 2
  cmp sp,dx
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  cmp di,dx
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  cmp bp,dx
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  cmp cx,dx
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  mov ax,0x0d06
  out dx,ax

  xchg ax,di
  mov di,5678
  movsw
  movsw
  xchg ax,di
.endCode


experimentSlide2:
  db "IntroSlide2$"
  dw .endInit - ($+2)
  mov dx,0x3d4
  mov si,0x3d9
  mov ax,0xb800
  mov es,ax
.endInit:
  dw .endCode - ($+2)

  add sp,[bx]   ; 2 2
  cmp sp,dx
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  cmp di,dx
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  cmp bp,dx
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  cmp cx,dx
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  xchg ax,di
  mov di,5678
  movsw
  movsw
  movsw
  xchg ax,di
.endCode


experimentIntroCMP:
  db "IntroCMP$"
  dw .endInit - ($+2)
  mov dx,0x3d9
  mov si,0x3d9
  mov ax,0xb800
  mov es,ax
.endInit:
  dw .endCode - ($+2)

  add sp,[bx]   ; 2 2
  cmp sp,dx
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  cmp di,dx
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  cmp bp,dx
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  cmp cx,dx
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  mov al,[ss:bx+12]
  out dx,al

  xchg ax,di
  mov di,5678
  movsw
  movsw
  xchg ax,di
.endCode

experimentIntroCMP_NoRaster:
  db "IntroCMP_NoRaster$"
  dw .endInit - ($+2)
  mov dx,0x3d9
  mov si,0x3d9
  mov ax,0xb800
  mov es,ax
.endInit:
  dw .endCode - ($+2)

  add sp,[bx]   ; 2 2
  cmp sp,dx
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  cmp di,dx
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  cmp bp,dx
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  cmp cx,dx
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  xchg ax,di
  mov di,5678
  movsw
  movsw
  movsw
  xchg ax,di
.endCode



experimentIntro:
  db "Intro$"
  dw .endInit - ($+2)
  mov dx,0x3d9
  mov si,0x3d9
  mov ax,0xb800
  mov es,ax
.endInit:
  dw .endCode - ($+2)

  add sp,[bx]   ; 2 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  mov al,[ss:bx+12]
  out dx,al

  xchg ax,di
  mov di,5678
  movsw
  movsb
  xchg ax,di
.endCode


experimentIntroB:
  db "Intro$"
  dw .endInit - ($+2)
  mov dx,0xe0
  mov ax,0xb800
  mov es,ax
.endInit:
  dw .endCode - ($+2)

  add sp,[bx]   ; 2 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  sbb ax,ax     ; 2 0    10

  add di,[bx+2] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add bp,[bx+4] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  add cx,[bx+6] ; 3 2
  lahf          ; 1 0
  rcl ah,1      ; 3 0
  adc ax,bx     ; 2 0    11

  xlatb
  out 0xe0,al

  xchg ax,di
  mov di,5678
  movsw
  movsw
  movsb
  xchg ax,di

.endCode

experimentMod:
  db "Mod$"
  dw .endInit - ($+2)
  mov dx,0x3d9
.endInit:
  dw .endCode - ($+2)
  xor ax,ax

  add si,[bp]
  mov bx,si
  mov bl,99
  mov al,[bx]

  add di,[bp+2]
  mov bx,di
  mov bl,99
  add al,[bx]

  add cx,[bp+4]
  mov bx,cx
  mov bl,99
  add al,[bx]

  add sp,[bp+6]
  mov bx,sp
  mov bl,99
  add al,[bx]

  out 0xe0,al

  out dx,al
.endCode


experimentIntro2:
  db "Intro2$"
  dw .endInit - ($+2)
  mov dx,0x3d9
  xor si,si
  xor di,di
  xor bp,bp
  xor cx,cx
  mov [bx],si
  mov [bx+2],si
  mov [bx+4],si
  mov [bx+6],si

.endInit:
  dw .endCode - ($+2)
  xor ax,ax

  add si,[bx]   ; 2 2
  js .p1
  inc ax
.p1:

  add di,[bx+2] ; 3 2
  js .p2
  inc ax
.p2:

  add bp,[bx+4] ; 3 2
  js .p3
  inc ax
.p3:

  add cx,[bx+6] ; 3 2
  js .p4
  inc ax
.p4:

  xlatb
  out 0xe0,al

  pop ax
  out dx,al
.endCode

experimentIntro2b:
  db "Intro2b$"
  dw .endInit - ($+2)
  mov dx,0x3d9
  xor si,si
  xor di,di
  xor bp,bp
  xor cx,cx
  mov [bx],si
  mov [bx+2],si
  mov [bx+4],si
  mov [bx+6],si
.endInit:
  dw .endCode - ($+2)
  xor ax,ax

  add si,[bx]   ; 2 2
  jns .p1
  inc ax
.p1:

  add di,[bx+2] ; 3 2
  jns .p2
  inc ax
.p2:

  add bp,[bx+4] ; 3 2
  jns .p3
  inc ax
.p3:

  add cx,[bx+6] ; 3 2
  jns .p4
  inc ax
.p4:

  xlatb
  out 0xe0,al

  pop ax
  out dx,al
.endCode



experimentMagi1:
  db "Magi1$"
  dw .endInit - ($+2)

  mov ax,0xb800
  mov es,ax
  mov di,0
  mov si,0
  mov ax,0x8000
  mov ds,ax
.endInit:
  dw .endCode - ($+2)

  movsw
  movsw
  add di,76

.endCode

experimentMagi2:
  db "Magi2$"
  dw .endInit - ($+2)

  mov ax,0xb800
  mov es,ax
  mov di,0
  mov si,0
  mov ax,0x8000
  mov ds,ax
.endInit:
  dw .endCode - ($+2)

  movsb
  inc di
  movsb
  add di,77

.endCode


experimentDomKeyb:
  db "DomKeyb$"
  dw .endInit - ($+2)

  mov ax,0x8000
  mov es,ax
  mov dx,0xe0
  mov si,0
.endInit:
  dw .endCode - ($+2)

xchg ax,bx    ; bx=?, ax=0
out dx,al
xchg ax,cx    ; cx=0, ax=1
out dx,al
xchg ax,bx    ; ax=?, bx=1
in al,0xe0
stosb
xchg ax,cx    ; cx=?, ax=0
out dx,al
xchg ax,bx    ; bx=0, ax=1
out dx,al
xchg ax,cx    ; cx=1, ax=?
in al,0xe0
stosb

.endCode


experimentMod1:
  db "Mod1$"
  dw .endInit - ($+2)

  mov ax,0x8000
  mov ds,ax
  mov ss,ax
  mov cx,1
  mov ah,1

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


experimentMod2:
  db "Mod2$"
  dw .endInit - ($+2)

  mov ax,0x8000
  mov ds,ax
  mov ss,ax
  mov cx,1
  mov ah,1

.endInit:
  dw .endCode - ($+2)

  add dl,0x0f
  adc si,-1
  lodsb
  add dh,0x0f
  adc bp,0
  add al,[ds:bp]
  add ch,0x0f
  adc di,0
  add al,[di]
  add cl,0x0f
  adc bx,0
  add al,[bx]
  out 0xe0,al
  dec ah
  jnz .endInit+2

  xchg ax,bx
  pop bx
  pop word[cs:bx]
  xchg ax,bx
  mov ah,1
  jmp $+2

.endCode


experimentMod2a:
  db "Mod2a$"
  dw .endInit - ($+2)

  mov ax,0x8000
  mov ds,ax
  mov ss,ax
  mov cx,1
  mov ah,1

.endInit:
  dw .endCode - ($+2)

  nop

  add dl,0x0f
  adc si,-1
  lodsb
  add dh,0x0f
  adc bp,0
  add al,[ds:bp]
  add ch,0x0f
  adc di,0
  add al,[di]
  add cl,0x0f
  adc bx,0
  add al,[bx]
  out 0xe0,al
  dec ah
  jnz .endInit+2

  xchg ax,bx
  pop bx
  pop word[cs:bx]
  xchg ax,bx
  mov ah,1
  jmp $+2

.endCode


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
  mov al,19
  out 0x41,al  ; Timer 1 rate

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

  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
timerStartEnd:


