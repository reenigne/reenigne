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

  mov cx,240+48  ; Number of iterations in primary measurement
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

experimentKefrensScanline0:
  db "KefrensScanline0$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,0x4567
  mov ds,ax
  mov sp,[bx]
  pop di
  mov al,[es:di]     ; 3 1 +WS
  pop cx
  and ax,cx          ; 2 0
  pop cx
  or ax,cx           ; 2 0
  stosw              ; 2 2 +WS +WS
  pop ax
  and ah,[es:di+1]
  pop cx
  or ax,cx
  stosw
  pop ax
  out dx,al
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline199_200:
  db "KefrensScanline199_200$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,0x4567
  mov ds,ax
  mov sp,[bx]
  pop di
  mov al,[es:di]     ; 3 1 +WS
  pop cx
  and ax,cx          ; 2 0
  pop cx
  or ax,cx           ; 2 0
  stosw              ; 2 2 +WS +WS
  pop ax
  and ah,[es:di+1]
  pop cx
  or ax,cx
  stosw
  pop ax
  out dx,al
  mov ds,bp
  lodsb
  out 0xe0,al

  mov dl,0xd4
  mov ax,0x3b04
  out dx,ax
  times 45 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline200:
  db "KefrensScanline200$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov dl,0xd4
  mov ax,0x3b04
  out dx,ax
  times 45 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline200_201:
  db "KefrensScanline200_201$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov dl,0xd4
  mov ax,0x3b04
  out dx,ax
  times 45 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline201:
  db "KefrensScanline201$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline201_202:
  db "KefrensScanline201_202$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  xor di,di
  xor ax,ax
  nop
  nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline202:
  db "KefrensScanline202$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  xor di,di
  xor ax,ax
  nop
  nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline202_203:
  db "KefrensScanline202_203$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  xor di,di
  xor ax,ax
  nop
  nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  xor ax,ax
  times 4 nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline203:
  db "KefrensScanline203$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  xor ax,ax
  times 4 nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline211_212:
  db "KefrensScanline211_212$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  xor ax,ax
  times 4 nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline258_259:
  db "KefrensScanline258_259$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline258_259a:
  db "KefrensScanline258_259a$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline258_259b:
  db "KefrensScanline258_259b$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline258_259c:
  db "KefrensScanline258_259c$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259:
  db "KefrensScanline259$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259a:
  db "KefrensScanline259a$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259b:
  db "KefrensScanline259b$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259c:
  db "KefrensScanline259c$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259_260:
  db "KefrensScanline259_260$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259a_260:
  db "KefrensScanline259a_260$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259b_260:
  db "KefrensScanline259b_260$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259c_260:
  db "KefrensScanline259c_260$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259_260a:
  db "KefrensScanline259_260a$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259a_260a:
  db "KefrensScanline259a_260a$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  jne .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259b_260a:
  db "KefrensScanline259b_260a$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline259c_260a:
  db "KefrensScanline259c_260a$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jnz .noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je .effectComplete
  jmp .doneKey
.noKey:
  times 28 nop
.doneKey:
  inc bx
  inc bx
  cmp bx,0xffff
  je .noNewLoop
  mov bx,0x80
.noNewLoop:
  jmp .frameLoop
.effectComplete:
.frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:


experimentKefrensScanline260:
  db "KefrensScanline260$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline260a:
  db "KefrensScanline260a$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline260_261:
  db "KefrensScanline260_261$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline260a_261:
  db "KefrensScanline260a_261$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov bx,[cs:dummy]
  mov si,[cs:bx]
  inc bx
  inc bx
  cmp bx,0xffff
  je .noRestartSong
  mov bx,0x0100
.noRestartSong:
  mov [cs:dummy],bx
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al
.endCode:

experimentKefrensScanline261_0:
  db "KefrensScanline261_0$"
  dw .endInit - ($+2)
  mov ax,0xb800
  mov es,ax
  mov bp,0x8000
  mov ds,bp
  mov ss,bp
  xor si,si
  mov bx,si
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)

  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0xe0,al

  mov ax,0x4567
  mov ds,ax
  mov sp,[bx]
  pop di
  mov al,[es:di]     ; 3 1 +WS
  pop cx
  and ax,cx          ; 2 0
  pop cx
  or ax,cx           ; 2 0
  stosw              ; 2 2 +WS +WS
  pop ax
  and ah,[es:di+1]
  pop cx
  or ax,cx
  stosw
  pop ax
  out dx,al
  mov ds,bp
  lodsb
  out 0xe0,al
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


