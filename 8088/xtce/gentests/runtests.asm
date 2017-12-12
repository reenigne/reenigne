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

  xor ax,ax
  mov ds,ax
  mov word[8*4],irq0
  mov [8*4+2],cs

  mov ax,cs
  mov ds,ax
  cli
  mov ss,ax
  xor sp,sp
  sti
  mov si,testCases
testLoop:
  mov ax,si
  sub ax,testCases
  cmp ax,[testCases]
  jb notDone
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
  cmp ax,[si]
  jne testFailed

  inc word[testCaseIndex]
  mov bl,[si+2]
  mov bh,0
  lea si,[si+bx+3]
  mov [testCaseOffset],si
  jmp testLoop

testFailed:
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

  mov cx,16
loopTop:
  mov [savedCX],cx

  call doCopy

  mov ax,0x8000
  mov ds,ax
  mov ax,[0]      ; Trigger: Start of command load sequence
  times 10 nop
  mov dl,16
  mov cx,[cs:savedCX]
  sub dl,cl

  outputByte
  mov dx,LENGTH
  dec dx
  outputByte
  outputByte
  mov dx,18996 + 492*3   ;65534 ;
  outputByte
  outputByte

  call runTest

  mov cx,25*LENGTH
flushLoop2:
  loop flushLoop2

  mov cx,[cs:savedCX]
  loop loopTop2
  complete
loopTop2:
  jmp loopTop

doCopy:
  mov ax,cs
  add ax,0x1000
  push ax
  mov es,ax
  xor di,di
  mov si,[testCaseOffset]
  mov bl,[si+1]
repeatLoop:
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
  mov cl,[si+2]
  add si,3
  rep movsb
  mov ax,0x00eb  ; 'jmp ip+0'
  stosw
  pop cx
  loop repeatLoop
  mov ax,0xffcd  ; 'int 0xff'
  stosw

  cli
  xor ax,ax
  mov es,ax
  mov word[es:0x3fc],interruptFF
  mov word[es:0x3fe],cs

  safeRefreshOff
  writePIT16 0, 2, 2
  writePIT16 0, 2, 100
  sti
  hlt
  hlt
  writePIT16 0, 2, 0
  cli
  ret

doMeasurement:
  call doCopy
runTest:
  mov [cs:savedSP],sp
  mov [cs:savedSS],ss
  mov sp,ax
  mov ax,cs
  add ax,0x1000
  mov ds,ax
  mov es,ax
  mov ss,ax
  push ax
  xor ax,ax
  push ax
  mov dx,ax
  mov bx,ax
  mov cx,ax
  mov si,ax
  mov di,ax
  mov bp,ax
  mov sp,ax
  retf

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
  ret



failMessage: db "FAIL "

testCaseIndex: dw 0
testCaseOffset: dw 0
savedSP: dw 0
savedSS: dw 0
savedCX: dw 0
lut: db 0x88,8

testCases:

; Format of testCases:
;   2 bytes: total length of testCases data excluding length field
;   For each testcase:
;     1 byte: cycle count
;     1 byte: queueFiller operation (0 = MUL) * 32 + number of NOPs
;     1 byte: number of instruction bytes
;     N bytes: instructions


