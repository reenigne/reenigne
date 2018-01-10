  %include "../../defaults_bin.asm"

ITERS EQU 8
LENGTH EQU 2048

%macro outputByte 0
%rep 8
  rcr dx,1
  sbb bx,bx
  mov bl,[cs:lut+1+bx]
  mov bh,0
  mov ax,[bx]
  times 10 nop
%endrep
%endmacro

; Loop over tests
;   Do {1, 9} iterations
;     Copy N instances of test code to CS + 64kB
;     Safe refresh off
;     Read timer
;     Execute test
;     Read timer
;     Subtract timer
;     Safe refresh on
;   Subtract timer deltas
;   Compare to expected
;   If not equal
;     Print failing test number
;     Copy an instance of test code
;     Execute under trace

  outputCharacter 8

  xor ax,ax
  mov ds,ax
  mov word[8*4],irq0
  mov [8*4+2],cs
  mov word[0xff*4],interruptFF
  mov word[0xff*4+2],cs

  mov ax,cs
  mov ds,ax

  cli
  mov ss,ax
  xor sp,sp
  sti
  mov si,testCases+2
  mov [testCaseOffset],si
testLoop:
  mov ax,si
  sub ax,testCases+2
  cmp ax,[testCases]
  jb notDone

  mov si,passMessage
  mov cx,4
  outputString

  complete
notDone:

  mov cx,ITERS+1   ; Number of iterations in primary measurement
  call doMeasurement
  push bx
  mov cx,1       ; Number of iterations in secondary measurement
  call doMeasurement
  pop ax         ; The primary measurement will have the lower value, since the counter counts down
  sub ax,bx      ; Subtract the secondary value, which will be higher, now AX is negative
  neg ax         ; Negate to get the positive difference.
  mov si,[testCaseOffset]
  cmp ax,[si]
  jne testFailed

  inc word[testCaseIndex]
  mov bl,[si+3]
  mov bh,0
  lea si,[si+bx+4]
  mov [testCaseOffset],si
  jmp testLoop

testFailed:
  shr ax,1
  mov [countedCycles],ax

  push si
  mov si,ax
  call outputDecimal
  outputCharacter ' '
  pop si
  mov si,[si]
  shr si,1
  call outputDecimal
  outputCharacter 10

  mov si,failMessage
  mov cx,5
  outputString
  mov si,[testCaseIndex]
  call outputDecimal

  outputCharacter 10

  mov word[sniffer],0x8000

  outputCharacter 6

  mov cx,16
loopTop:
  mov [savedCX],cx
  mov cx,1
  call doMeasurement

  mov ax,[countedCycles]
  mov dx,25
  mul dx
  mov cx,ax
flushLoop2:
  loop flushLoop2

  mov cx,[savedCX]
  loop loopTop2
  outputCharacter 7
  complete
loopTop2:
  jmp loopTop

doMeasurement:
  mov ax,cs
  add ax,0x1000
  mov es,ax
  xor di,di
  mov si,[testCaseOffset]
  mov bl,[si+2]
repeatLoop:

  push cx
  mov al,bl
  and al,0xe0
  cmp al,0
  jne notQueueFiller0
  mov ax,0x00b0  ; 'mov al,0'
  stosw
  mov ax,0xe0f6  ; 'mul al'
  stosw
  jmp doneQueueFiller
notQueueFiller0:

  jmp testFailed
doneQueueFiller:
  mov cl,bl
  and cl,0x1f
  mov al,0x90
  rep stosb
  mov cl,[si+3]
  push si
  add si,4
  rep movsb
  pop si
  mov ax,0x00eb  ; 'jmp ip+0'
  stosw
  pop cx
  loop repeatLoop
  mov ax,0xffcd  ; 'int 0xff'
  stosw
  xor ax,ax
  stosw
  stosw

  safeRefreshOff
  writePIT16 0, 2, 2
  writePIT16 0, 2, 100
  sti
  hlt
  hlt
  writePIT16 0, 2, 0
  cli

  mov ds,[cs:sniffer]
  mov ax,[0]      ; Trigger: Start of command load sequence
  times 10 nop
  mov dl,16
  mov cx,[cs:savedCX]
  sub dl,cl

  outputByte
  mov si,[cs:testCaseOffset]
  mov dx,[cs:countedCycles]
;  dec dx
  outputByte
  outputByte
  mov dx,714
  outputByte
  outputByte

  mov [cs:savedSP],sp
  mov [cs:savedSS],ss
  mov ax,cs
  add ax,0x1000
  mov ds,ax
  mov es,ax
  mov ss,ax

  xor ax,ax
  mov dx,ax
  mov bx,ax
  mov cx,ax
  mov si,ax
  mov di,ax
  mov bp,ax
  mov sp,ax
  mov word[cs:testBuffer],0
  mov [cs:testBuffer+2],ds
  jmp far [cs:testBuffer]

irq0:
  push ax
  mov al,0x20
  out 0x20,al
  pop ax
  iret

interruptFF:
  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al

  mov sp,[cs:savedSP]
  mov ss,[cs:savedSS]

  safeRefreshOn

  mov ax,cs
  mov ds,ax
  ret


outputDecimal:
;  push si
;  mov ax,si
;  outputHex
;  outputCharacter ':'
;  pop si
  mov bx,10000
  cmp si,bx
  jb no1e4
  mov ax,si
  xor dx,dx
  div bx
  mov si,dx
  add al,'0'
;    outputHex
  outputCharacter
no1e4:
  mov bx,1000
  cmp si,bx
  jb no1e3
  mov ax,si
  xor dx,dx
  div bx
  mov si,dx
  add al,'0'
;    outputHex
  outputCharacter
no1e3:
  mov bx,100
  cmp si,bx
  jb no1e2
  mov ax,si
  xor dx,dx
  div bx
  mov si,dx
  add al,'0'
;    outputHex
  outputCharacter
no1e2:
  mov bx,10
  cmp si,bx
  jb no1e1
  mov ax,si
  xor dx,dx
  div bx
  mov si,dx
  add al,'0'
;    outputHex
  outputCharacter
no1e1:
  mov ax,si
  add al,'0'
;    outputHex
  outputCharacter
  ret


failMessage: db "FAIL "
passMessage: db "PASS"

testCaseIndex: dw 0
testCaseOffset: dw 0
testBuffer: dw 0, 0
savedSP: dw 0
savedSS: dw 0
savedCX: dw 0
lut: db 0x88,8
sniffer: dw 0x7000
countedCycles: dw 1

testCases:

; Format of testCases:
;   2 bytes: total length of testCases data excluding length field
;   For each testcase:
;     2 bytes: cycle count
;     1 byte: queueFiller operation (0 = MUL) * 32 + number of NOPs
;     1 byte: number of instruction bytes
;     N bytes: instructions


