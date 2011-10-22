org 0
cpu 8086

  mov ax,cs
  mov ds,ax
  mov es,ax

  mov cx,256
majorLoop:
  mov bx,256
  sub bx,cx
  add bx,bx
  mov ax,[tests+bx]
  mov [experimentInit + 1],ax

  push cx
  mov cx,256
minorLoop:
  mov bx,256
  sub bx,cx
  add bx,bx
  mov ax,[tests+bx]
  mov [experimentCodeStart + 1],ax

  push cx
  call doExperiments
  pop cx

  loop minorLoop

;  mov al,10
;  int 0x62

  pop cx
  loop majorLoop

exit:
  int 0x67


experimentInit:
  mov bx,0
experimentCodeStart:
  mov ax,0
  mul bx
experimentCodeEnd:

tests:
  db 0x0000, 0x0001, 0xc000, 0x0007, 0xf000, 0x001f, 0xfc00, 0x007f, 0xff00, 0x01ff, 0xffc0, 0x07ff, 0xfff0, 0x1fff, 0xfffc, 0x7fff, 0xffff


print:
  push si
  mov si,dx
printLoop:
  lodsb
  cmp al,'$'
  je donePrint
  int 0x62
  jmp printLoop
donePrint:
  pop si
  ret


output:
  db "000.000 $"

printNumber:
  mov cx,10
  div cx
  add dl,'0'
  mov [output+6],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+5],dl
  xor dx,dx
  div cx
  add dl,'0'
  mov [output+4],dl
  xor dx,dx
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
  mov [output],dl
  mov dx,output
  jmp print


codeCopy:
  cmp si,dx
  je codeCopyDone
  cmp di,0xffff
  je codeCopyOutOfSpace
  movsb
  jmp codeCopy
codeCopyDone:
  ret
codeCopyOutOfSpace:
  mov dx,outOfSpaceMessage
  call print
  jmp exit

outOfSpaceMessage:
  db "Copy out of space - use fewer iterations$"


init: dw 0
codeStart: dw 0
codeEnd: dw 0


colonSpace:
  db ": $"
newLine:
  db "  $" ; db 0d,0a,"$"
newLine2:
  db 10,'$'


doExperiments:
  ; Find number of timer cycles for 1000 iterations in DX:AX
  mov cx,100
  call doExperiment
  push ax
  mov cx,1100
  call doExperiment
  pop dx
  sub ax,dx
  xor dx,dx

  ; Multiply by 4 to get CPU cycles *1000
  shl ax,1
  rcl dx,1
  shl ax,1
  rcl dx,1

  ; Add 500 for rounding
  add ax,500
  adc dx,0

  ; Divide by 1000 to get CPU cycles
  mov cx,1000
  div cx

  add al,'A'-74
  int 0x62
  ret

  ret


startTime: dw 0


  ; The following code isn't executed directly, it's copied elsewhere first
timerStartStart:
  ; Don't allow any hardware interrupts to upset the timing
  cli

  ; Ensure all memory rows are recently refreshed before turning off refresh
  times 256 nop

  ; Turn off refresh
  mov al,0x70  ; Timer 1, write LSB+MSB, mode 0, binary
  out 0x43,al
  mov al,0
  out 0x41,al
  out 0x41,al

  ; Wait for any pending refresh to occur (unnecessary?)
  times 18 nop

  ; Reset timer 0 so that the CPU is in lockstep with timer 0
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  mov [startTime],ax
  ; Code to be timed will be copied here
timerEndStart:
  in al,0x40
  mov ah,al
  in al,0x40
  xchg ah,al
  mov dx,[startTime]
  sub dx,ax

  mov al,0x54  ; Timer 1, write LSB, mode 2, binary
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  sti
  mov ax,dx
  ret
timerEndEnd:


doExperiment:
  mov di,codeSpace

  ; Copy timer start routine
  mov si,timerStartStart
  mov dx,timerEndStart
  call codeCopy

  ; Copy experiment init routine
  mov si,experimentInit
  mov dx,experimentCodeStart
  call codeCopy

  ; Copy experiment iterations
doExperimentCopyLoop:
  mov si,experimentCodeStart
  mov dx,experimentCodeEnd
  call codeCopy
  loop doExperimentCopyLoop

  ; Copy timer end routine
  mov si,timerEndStart
  mov dx,timerEndEnd
  call codeCopy

  ; Run experiment
codeSpace:



; Rewrite this as a routine which can be called - passed a structure containing:
;   Start code address
;   End code address
;   Text
;   Number of iterations
