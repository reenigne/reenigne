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


codePreambleStart:
;  mov al,0
;  mul cl
codePreambleEnd:

experimentData:

experimentCRTC:
  db "CRTC$"
  dw .endInit - ($+2)
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,((114 - 1) << 8) | 0x00       ;Horizontal total
  out dx,ax
  mov ax,(23 << 8) | 0x01              ;Horizontal displayed
  out dx,ax
  mov ax,(90 << 8) | 0x02              ;Horizontal sync position
  out dx,ax
.endCode:

experimentLine:
  db "Line$"
  dw .endInit - ($+2)
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)
  mov ax,((114 - 1) << 8) | 0x00       ;Horizontal total
  out dx,ax
  mov ax,(23 << 8) | 0x01              ;Horizontal displayed
  out dx,ax
  mov ax,(90 << 8) | 0x02              ;Horizontal sync position
  out dx,ax

  times 17 nop

  mov ax,((114 - 1) << 8) | 0x00       ;Horizontal total
  out dx,ax
  mov ax,(23 << 8) | 0x01              ;Horizontal displayed
  out dx,ax
  mov ax,(90 << 8) | 0x02              ;Horizontal sync position
  out dx,ax

  times 17 nop
.endCode:

experimentLine2:
  db "Line2$"
  dw .endInit - ($+2)
  mov dx,0x3d4
.endInit:
  dw .endCode - ($+2)

  mov dl,0xd9
  mov al,9
  out dx,al
  mov al,6
  out dx,al

  times 26 nop

  mov al,0x0a
  out dx,al
  mov al,6
  out dx,al
  mov dl,0xd4

  times 27 nop
.endCode:


experimentBlock:
  db "Block$"
  dw .endInit - ($+2)
  mov dx,0x3d9
.endInit:
  dw .endCode - ($+2)
  mov al,8
  out dx,al
  mov al,ah
  out dx,al
.endCode:

experimentBlock2:
  db "Block2$"
  dw .endInit - ($+2)
  mov dx,0x3d9
.endInit:
  dw .endCode - ($+2)
  inc ax
  out dx,al
.endCode:

experimentMOVSBstring:
  db "MOVSB string$"
  dw .endInit - ($+2)
  mov ax,0x9000
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov si,0
  mov di,0
.endInit:
  dw .endCode - ($+2)
  movsb
.endCode:

experimentMOVSWstring:
  db "MOVSW string$"
  dw .endInit - ($+2)
  mov ax,0x9000
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov si,0
  mov di,0
.endInit:
  dw .endCode - ($+2)
  movsw
.endCode:

experimentLODSBSTOSBstring:
  db "LODSB STOSB string$"
  dw .endInit - ($+2)
  mov ax,0x9000
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov si,0
  mov di,0
.endInit:
  dw .endCode - ($+2)
  lodsb
  stosb
.endCode:

experimentLODSWSTOSWstring:
  db "LODSW STOSW string$"
  dw .endInit - ($+2)
  mov ax,0x9000
  mov ds,ax
  mov ax,0xb800
  mov es,ax
  mov si,0
  mov di,0
.endInit:
  dw .endCode - ($+2)
  lodsw
  stosw
.endCode:

;experimentPaletteRegister:
;  db "palette change$"
;  dw .endInit - ($+2)
;  mov dx,0x3d9
;.endInit:
;  dw .endCode - ($+2)
;  mov al,99
;  out dx,al
;.endCode:
;
;experimentJJ:
;  db "pearce_jj$"
;  dw .endInit - ($+2)
;  mov ax,0x9000
;  mov ds,ax
;  mov es,ax
;  mov dx,0x3d8
;  mov si,0
;  mov di,0
;.endInit:
;  dw .endCode - ($+2)
;  in ax,dx
;  stosw
;.endCode:
;
;experimentJK:
;  db "Jordan Knight$"
;  dw .endInit - ($+2)
;  mov ax,0x9000
;  mov ds,ax
;  mov es,ax
;  mov si,0
;  mov di,0
;.endInit:
;  dw .endCode - ($+2)
;  lodsw
;  mov bx,ax
;  mov ax,[es:di]
;  and al,bh
;  or al,bl
;  stosw
;.endCode:
;
;experimentJL:
;  db "Jim Leonard$"
;  dw .endInit - ($+2)
;  mov ax,0x9000
;  mov ds,ax
;  mov es,ax
;  mov si,0
;  mov di,0
;.endInit:
;  dw .endCode - ($+2)
;  mov dx,[bx+3]
;  lodsw
;  and al,ah
;  or al,dl
;  stosw
;.endCode:
;
;experimentAJ:
;  db "Andrew Jenner$"
;  dw .endInit - ($+2)
;  mov ax,0x9000
;  mov ds,ax
;  mov es,ax
;  mov si,0
;  mov di,0
;.endInit:
;  dw .endCode - ($+2)
;  mov ax,[bx+3]
;  and al,[di]
;  or al,ah
;  stosb
;  inc di
;.endCode:
;
;
;experiment1:
;  db "retrace loop$"
;  dw .endInit - ($+2)
;  mov dx,0x3d9
;.endInit:
;  dw .endCode - ($+2)
;  in al,dx
;  test al,1
;  jz .endCode
;.endCode
;
;experiment2:
;  db "retrace found$"
;  dw .endInit - ($+2)
;  mov dx,0x3d9
;.endInit:
;  dw .endCode - ($+2)
;  in al,dx
;  test al,1
;  jnz .endCode
;.endCode
;
;%macro setNextStartAddress 0       ;    23
;    mov bl,ch                      ; 2 0 2
;    mov bh,0x34                    ; 2 0 2
;    mov ah,[bx]                    ; 2 1 3
;    mov al,0x0d                    ; 2 0 2
;    out dx,ax                      ; 1 2 3
;    mov ah,[bx+0x100]              ; 4 1 5
;    dec ax                         ; 1 0 1
;    out dx,ax                      ; 1 2 3
;    add cx,si                      ; 2 0 2
;%endMacro
;
;experiment3:
;  db "scanline 0$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  mov ax,0x1902 ; 2: Horizontal sync position: 25       ; 3 0 3  11
;  out dx,ax                                             ; 1 2 3  14
;  mov ax,0x0104 ; 4: Vertical total: 2 rows/frame       ; 3 0 3  17
;  out dx,ax                                             ; 1 2 3  20
;  times 7 nop                                           ; 1 0 1  27
;
;  ; Set length of second part of scanline
;  mov ax,0x2400 ; 0: Horizontal total: 37 characters    ; 3 0 3  30   = length of second half-line, total 57 characters
;  out dx,ax                                             ; 1 2 3  33
;
;  setNextStartAddress                                   ;    23  56   Display will be disabled at around IO 53
;
;  ; Might want to put this back in for compatibility with faster machines - see if it makes it too slow on the XT
;  mov dl,0xda                                           ; 2 0 2  58
;.endCode
;
;experiment3a:
;  db "scanline 0a$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  mov ax,0x1902 ; 2: Horizontal sync position: 25       ; 3 0 3  11
;  out dx,ax                                             ; 1 2 3  14
;  mov ax,0x0104 ; 4: Vertical total: 2 rows/frame       ; 3 0 3  17
;  out dx,ax                                             ; 1 2 3  20
;  times 7 nop                                           ; 1 0 1  27
;.endCode
;
;experiment4:
;  db "scanlines 1-198$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  times 19 nop                                          ; 1 0 1  27
;
;  ; Set length of second part of scanline
;  mov ax,0x2400 ; 0: Horizontal total: 37 characters    ; 3 0 3  30   = length of second half-line, total 57 characters
;  out dx,ax                                             ; 1 2 3  33
;
;  setNextStartAddress                                   ;    23  56   Display will be disabled at around IO 53
;
;  ; Might want to put this back in for compatibility with faster machines - see if it makes it too slow on the XT
;  mov dl,0xda                                           ; 2 0 2  58
;.endCode
;
;experiment4a:
;  db "scanlines 1-198a$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  times 19 nop                                          ; 1 0 1  27
;.endCode
;
;experiment5:
;  db "scanline 199$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  mov ax,0x3f04 ; 4: Vertical total: 64 rows/frame      ; 3 0 3  11
;  out dx,ax                                             ; 1 2 3  14
;  mov cx,bp     ; Initial offset in lines/256           ; 2 0 2  16
;  times 11 nop                                          ; 1 0 1  27
;
;  ; Set length of second part of scanline
;  mov ax,0x2400 ; 0: Horizontal total: 37 characters    ; 3 0 3  30   = length of second half-line, total 57 characters
;  out dx,ax                                             ; 1 2 3  33
;
;  setNextStartAddress                                   ;    23  56   Display will be disabled at around IO 53
;
;  ; Might want to put this back in for compatibility with faster machines - see if it makes it too slow on the XT
;  mov dl,0xda                                           ; 2 0 2  58
;.endCode
;
;experiment5a:
;  db "scanline 199a$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  mov ax,0x3f04 ; 4: Vertical total: 64 rows/frame      ; 3 0 3  11
;  out dx,ax                                             ; 1 2 3  14
;  mov cx,bp     ; Initial offset in lines/256           ; 2 0 2  16
;  times 11 nop                                          ; 1 0 1  27
;.endCode
;
;experiment6:
;  db "scanline 199-200$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  mov ax,0x3f04 ; 4: Vertical total: 64 rows/frame      ; 3 0 3  11
;  out dx,ax                                             ; 1 2 3  14
;  mov cx,bp     ; Initial offset in lines/256           ; 2 0 2  16
;  times 11 nop                                          ; 1 0 1  27
;
;  ; Set length of second part of scanline
;  mov ax,0x2400 ; 0: Horizontal total: 37 characters    ; 3 0 3  30   = length of second half-line, total 57 characters
;  out dx,ax                                             ; 1 2 3  33
;
;  setNextStartAddress                                   ;    23  56   Display will be disabled at around IO 53
;
;  ; Might want to put this back in for compatibility with faster machines - see if it makes it too slow on the XT
;  mov dl,0xda                                           ; 2 0 2  58
;  ; waitForDisplayDisable
;
;
;  ; During line 200 we:
;  ;   change the horizontal total to 0x38
;  ;   change the horizontal sync position to 0x2d
;  ; Can't use waitForDisplayEnable here because it won't activate again until line 0
;  times 18 nop
;  mov dl,0xd4
;  mov ax,0x3800 ; 0: Horizontal total: 57 characters
;  out dx,ax
;  mov ax,0x2d02 ; 2: Horizontal sync position: 45
;  out dx,ax
;
;.endCode
;
;experiment6a:
;  db "scanline 199-200$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;  mov dl,0xd4                                           ; 2 0 2   2
;  mov ax,0x1300 ; 0: Horizontal total: 20 characters    ; 3 0 3   5   = length of first half-line
;  out dx,ax                                             ; 1 2 3   8
;  mov ax,0x3f04 ; 4: Vertical total: 64 rows/frame      ; 3 0 3  11
;  out dx,ax                                             ; 1 2 3  14
;  mov cx,bp     ; Initial offset in lines/256           ; 2 0 2  16
;  times 11 nop                                          ; 1 0 1  27
;
;  ; Set length of second part of scanline
;  mov ax,0x2400 ; 0: Horizontal total: 37 characters    ; 3 0 3  30   = length of second half-line, total 57 characters
;  out dx,ax                                             ; 1 2 3  33
;
;  setNextStartAddress                                   ;    23  56   Display will be disabled at around IO 53
;
;  ; Might want to put this back in for compatibility with faster machines - see if it makes it too slow on the XT
;  mov dl,0xda                                           ; 2 0 2  58
;  ; waitForDisplayDisable
;
;
;  ; During line 200 we:
;  ;   change the horizontal total to 0x38
;  ;   change the horizontal sync position to 0x2d
;  ; Can't use waitForDisplayEnable here because it won't activate again until line 0
;  times 18 nop
;
;.endCode



;experiment1:
;  db "load to bh 1$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;  lodsb
;  mov bh,al
;.endCode


;experiment2:
;  db "load to bh 2$"
;  dw .endInit - ($+2)
;.endInit:
;  dw .endCode - ($+2)
;  mov bh,[si]
;  inc si
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


