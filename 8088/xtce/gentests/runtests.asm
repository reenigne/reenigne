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

;  outputCharacter 8
    mov dx,0x3d9
    mov al,0
    out dx,al

  xor ax,ax
  mov ds,ax
  mov word[8*4],irq0
  mov [8*4+2],cs

  mov ax,cs
  mov ds,ax

    mov ax,ds
    outputHex
    outputCharacter ':'
    mov ax,testCases
    outputHex
    outputCharacter ' '

  cli
  mov ss,ax
  xor sp,sp
  sti
  mov si,testCases
  mov [testCaseOffset],si
testLoop:
  mov ax,si
  sub ax,testCases
  cmp ax,[testCases]
  jb notDone

;  mov si,passMessage
;  mov cx,4
;  outputString

  complete
    mov dx,0x3d9
    mov al,11
    out dx,al
    cli
    hlt
notDone:

  mov cx,ITERS+1   ; Number of iterations in primary measurement
  call doMeasurement
  push bx
  mov cx,1       ; Number of iterations in secondary measurement
  call doMeasurement
  pop ax         ; The primary measurement will have the lower value, since the counter counts down
  sub ax,bx      ; Subtract the secondary value, which will be higher, now AX is negative
  neg ax         ; Negate to get the positive difference.
  cmp ax,[si]
  jne testFailed

  inc word[testCaseIndex]
  mov bl,[si+2]
  mov bh,0
  lea si,[si+bx+3]
  mov [testCaseOffset],si
  jmp testLoop

testFailed:
    mov dx,0x3d9
    mov al,8
    out dx,al
;    cli
;    hlt

  mov si,failMessage
  mov cx,5
  outputString
  mov si,[testCaseIndex]

  mov bx,10000
  cmp si,bx
  jl no1e4
  mov ax,si
  div bx
  mov si,dx
  add al,'0'
  outputCharacter
no1e4:

  mov bx,1000
  cmp si,bx
  jl no1e3
  mov ax,si
  div bx
  mov si,dx
  add al,'0'
  outputCharacter
no1e3:

  mov bx,100
  cmp si,bx
  jl no1e2
  mov ax,si
  div bx
  mov si,dx
  add al,'0'
  outputCharacter
no1e2:

  mov bx,10
  cmp si,bx
  jl no1e1
  mov ax,si
  div bx
  mov si,dx
  add al,'0'
  outputCharacter
no1e1:

  mov ax,si
  add al,'0'
  outputCharacter

  mov word[sniffer],0x8000

  mov cx,16
loopTop:
  mov [savedCX],cx

  mov al,cl
  add al,0x40
  outputCharacter

  call doMeasurement

  mov si,[testCaseOffset]
  mov al,[si+2]
  mov ah,25
  mul ah
  mov cx,ax
flushLoop2:
  loop flushLoop2

  mov cx,[savedCX]
  loop loopTop2
  complete
    mov dx,0x3d9
    mov al,9
    out dx,al
    cli
    hlt
loopTop2:
  jmp loopTop

doMeasurement:
    mov dx,0x3d9
    mov al,1
    out dx,al

    outputCharacter '*'

  mov ax,cs
  add ax,0x1000
  mov es,ax
  xor di,di
  mov si,[testCaseOffset]
  mov bl,[si+1]
repeatLoop:

    outputCharacter '+'

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

    mov ax,ds
    outputHex
    outputCharacter ':'
    mov ax,si
    outputHex

    mov dx,0x3d9
    mov al,10
    out dx,al
    cli
    hlt
  jmp testFailed
doneQueueFiller:
  mov cl,bl
  and cl,0x1f
  mov al,0x90
  rep stosb
  mov cl,[si+2]
  push si
  add si,3
  rep movsb
  pop si
  mov ax,0x00eb  ; 'jmp ip+0'
  stosw
  pop cx
  loop repeatLoop
  mov ax,0xffcd  ; 'int 0xff'
  stosw

    mov dx,0x3d9
    mov al,2
    out dx,al

  cli
  xor ax,ax
  mov es,ax
  mov word[es:0x3fc],interruptFF
  mov word[es:0x3fe],cs

    mov dx,0x3d9
    mov al,3
    out dx,al

;  safeRefreshOff
;  writePIT16 0, 2, 2
;  writePIT16 0, 2, 100
;  sti
;  hlt
;  hlt
;  writePIT16 0, 2, 0
;  cli

    mov dx,0x3d9
    mov al,4
    out dx,al

  mov ax,[sniffer]
  mov ds,ax
  mov ax,[0]      ; Trigger: Start of command load sequence
  times 10 nop
  mov dl,16
  mov cx,[cs:savedCX]
  sub dl,cl

  outputByte
  mov si,[cs:testCaseOffset]
  mov dl,[cs:si+2]
  dec dx
  outputByte
  outputByte
  mov dx,18996 + 492*3   ;65534 ;
  outputByte
  outputByte

    mov dx,0x3d9
    mov al,5
    out dx,al

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
;  mov word[cs:testBuffer],0
;  mov [cs:testBuffer+2],ds
  mov word[cs:testBuffer],interruptFF
  mov word[cs:testBuffer+2],cs

  jmp far [cs:testBuffer]

irq0:
  push ax
  mov al,0x20
  out 0x20,al
  pop ax
  iret

interruptFF:
    mov dx,0x3d9
    mov al,6
    out dx,al

  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al

  mov sp,[cs:savedSP]
  mov ss,[cs:savedSS]

;  safeRefreshOn

  mov ax,cs
  mov ds,ax
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

testCases:

; Format of testCases:
;   2 bytes: total length of testCases data excluding length field
;   For each testcase:
;     1 byte: cycle count
;     1 byte: queueFiller operation (0 = MUL) * 32 + number of NOPs
;     1 byte: number of instruction bytes
;     N bytes: instructions


