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
testLoop:
  mov si,[testCasePointer]
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
  je success




testCasePointer: dw testCases

testCases:

; Format of testCases:
;   2 bytes: total length of testCases data excluding length field
;   For each testcase:
;     1 byte: cycle count
;     1 byte: queueFiller operation (0 = MUL) * 32 + number of NOPs
;     1 byte: number of instruction bytes
;     N bytes: instructions


