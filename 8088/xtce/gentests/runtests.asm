  %include "../../defaults_bin.asm"

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

testLoop:
  mov ax,cs
  mov ds,ax
  mov si,[testCasePointer]
  cmp si,[testCases]
  jb notDone
  complete
notDone:




testCasePointer: dw testCases

testCases:

; Format of testCases:
;   2 bytes: total length of testCases data excluding length field
;   For each testcase:
;     1 byte: cycle count
;     1 byte: queueFiller operation (0 = MUL) * 32 + number of NOPs
;     1 byte: number of instruction bytes
;     N bytes: instructions


