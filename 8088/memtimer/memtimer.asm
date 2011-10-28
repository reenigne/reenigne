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
  xor bx,bx
nextExperiment:

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
  mov cx,20
  sub cl,bl
spaceLoop:
  mov al,' '
  int 0x62
  loop spaceLoop

  mov cx,5    ; Number of repeats
repeatLoop:
  push cx

  mov cx,480+48  ; Number of iterations in primary measurement
  call doMeasurement
  push ax
  mov cx,48      ; Number of iterations in secondary measurement
  call doMeasurement
  pop dx
  sub dx,ax
  xchg ax,dx

  sub ax,8400  ; Correct for the 70 cycle multiply: 8400 = 480*70/4

  xor dx,dx
  mov cx,120
  div cx       ; Divide by 120 to get number of cycles (quotient) and number of extra tcycles (remainder)

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

  mov si,output
  mov cx,10
  int 0x61

  pop cx
  loop repeatLoop
  lodsw
  add si,ax
  lodsw
  add si,ax
  jmp nextExperiment

output:
  db "000 +000  "


doMeasurement:
  push si
  push cx  ; Save number of iterations

  ; Copy init
  lodsw    ; Number of init bytes
  mov cx,ax
  mov di,timerStartEnd
  rep movsb

  ; Copy code
  lodsw    ; Number of code bytes
  pop cx
iterationLoop:
  push cx
  push si
  mov cx,ax
  rep movsb
  pop si
  pop cx
  loop iterationLoop

  ; Copy timer end
  mov si,timerEndStart
  mov cx,timerEndEnd-timerEndStart
  rep movsb


  mov al,0x70  ; Timer 1, write LSB+MSB, mode 0, binary
  out 0x43,al
  mov al,0
  out 0x41,al
  out 0x41,al

  ; Wait for any pending refresh to occur (unnecessary?)
  times 18 nop

  mov al,0xfe  ; Enable IRQ 0 (timer), disable others
  out 0x21,al

  mov al,0x24  ; Timer 0, write LSB, mode 2, binary
  out 0x43,al
  mov al,0x04
  out 0x40,al
  sti
  hlt

  ; The actual measurement happens in the the IRQ0 handler which runs here.

  pop si
  ret




experimentData:

experiment1:
  db "add [0],ah$"
  dw .endInit - .startInit
.startInit:
  mov cl,0
.endInit:
  dw .endCode - .startCode
.startCode
  mul cl
  add [0],al
.endCode

  ; TODO: Add more experiments here

lastExperiment:
  db '$'



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
  iret
timerEndEnd:


interrupt8:
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al
timerStartEnd:

  ;


;%rep 480
;;  lodsw
;;  push ax
;
;;  stosw
;;  nop
;;  nop
;;  nop
;;  nop
;;  nop
;;  nop
;
;  mul cl
;  add [endCode],al
;%endrep
;
;  rep stosb

  add sp,6

  xor ax,ax
  sub ax,bx



align 16
endCode:
  dw 0,0

