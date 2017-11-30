  %include "../../defaults_bin.asm"

ITERS EQU 8

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

  mov si,[testCasePointer]
  call copyTestCase
  ; TODO: bus sniffer


doMeasurement:
  mov ax,cs
  add ax,0x1000
  mov es,ax
  xor di,di
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
  push cx
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








failMessage: db "FAIL "

testCaseIndex: dw 0

testCases:

; Format of testCases:
;   2 bytes: total length of testCases data excluding length field
;   For each testcase:
;     1 byte: cycle count
;     1 byte: queueFiller operation (0 = MUL) * 32 + number of NOPs
;     1 byte: number of instruction bytes
;     N bytes: instructions


