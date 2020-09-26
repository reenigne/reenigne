%include "../defaults_com.asm"

LENGTH EQU 2048
moveListCount equ 42

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

; Check command line. If number specified, go single test routine with specified number.
; Loop over tests
;   Disable interrupts
;   Backup contents of segment 0x10a8 to buffer
;   Copy test code to segment 0x10a8
;   Safe refresh off
;   Read timer
;   Execute test
;   Read timer
;   Subtract timer
;   Safe refresh on
;   Restore stuff from buffer to segment 0x10a8
;   Compare to expected
;   If not equal
;     Print failing test number
;     Copy an instance of test code
;     Execute under trace

main:
  mov si,bannerMessage
  mov cx,passedMessage - bannerMessage
  outputString

  xor bx,bx
  xor di,di
  mov si,0x81
findInitialSpaces:
  lodsb
  cmp al,0x20
  je findInitialSpaces
  jmp startSearch

searchLoop:
  lodsb
  cmp al,0x20
  je foundEnd
startSearch:
  cmp al,'0'
  jl notNumber
  cmp al,'9'
  jg notNumber
  inc di
  sub al,'0'
  push ax
  mov ax,10
  mul bx
  pop cx
  add ax,cx
  mov bx,ax
notNumber:
  cmp al,0x0d
  jne searchLoop
foundEnd:
  test di,di
  jz notSingleTest
  mov [singleTest],bx
notSingleTest:

  mov ax,0x20a8
  mov bx,cs
  add bx,0x1000
  cmp bx,ax
  jae not20a8
  mov bx,ax
not20a8:
  xor ax,ax
  mov es,ax
  mov ax,[es:0x413]
  mov cl,6
  shl ax,cl
  sub ax,0x1000
  cmp ax,bx
  jae enoughRAM

  mov si,memoryMessage
  mov cx,memoryMessageEnd
  outputString
  outputCharacter 13
  outputCharacter 10
  mov ax,0x4c01
  int 0x21

enoughRAM:
  mov es,bx
  mov ax,cs
  mov ds,ax
  mov cx,0x8000
  xor si,si
  xor di,di
  rep movsw

  push es
  mov ax,doneReloc
  push ax
  retf
doneReloc:

  mov ax,0xf000
  mov es,ax
  cmp byte[0xff70],0xcc
  je biosOk
  mov byte[badBIOS],1
biosOk:

  ; Enable PIT channel 2. We'll use this for timing so that tests can do IRQ0s.
  in al,0x61
  and al,0xfc
  or al,1
  out 0x61,al

  in al,0x21
  mov [imr],al


  ; Enable auto-EOI
  mov al,0x13  ; ICW4 needed, not cascaded, call address interval 8, edge triggered
  out 0x20,al  ; Set ICW1
  mov al,0x08  ; Interrupt vector address
  out 0x21,al  ; Set ICW2
  mov al,0x0f  ; 8086/8088 mode, auto-EOI, buffered mode/master, not special fully nested mode
  out 0x21,al  ; Set ICW4
  mov al,0xfe ;bc  ; Enable IRQs 0 (timer). Leave disabled 1 (keyboard) and 6 (floppy disk).
  out 0x21,al  ; Leave disabled 2 (EGA/VGA/slave 8259) 3 (COM2/COM4), 4 (COM1/COM3), 5 (hard drive, LPT2) and 7 (LPT1)

  xor ax,ax
  mov ds,ax
  mov ax,[8*4]
  mov [cs:oldInt8],ax
  mov ax,[8*4+2]
  mov [cs:oldInt8+2],ax
  mov ax,[3*4]
  mov [cs:oldInt3],ax
  mov ax,[3*4+2]
  mov [cs:oldInt3+2],ax
  mov ax,[0xfe*4]
  mov [cs:oldIntFE],ax
  mov ax,[0xfe*4+2]
  mov [cs:oldIntFE+2],ax
  mov ax,[0xff*4]
  mov [cs:oldIntFF],ax
  mov ax,[0xff*4+2]
  mov [cs:oldIntFF+2],ax

  mov word[8*4],irq0
  mov [8*4+2],cs
  mov word[0xff*4],interruptFF
  mov [0xff*4+2],cs
  mov word[0xfe*4],interruptFE
  mov [0xfe*4+2],cs
  mov word[3*4],int3handler
  mov [3*4+2],cs

  mov ax,cs
  mov ds,ax

  cli
  mov ss,ax
  xor sp,sp
  sti
  mov si,testCases+2
  mov [testCaseOffset],si
testLoop:

;   mov ax,[testCaseIndex]
;   outputHex
;   outputCharacter ' '

  mov ax,si
  sub ax,testCases+2
  cmp ax,[testCases]
  jae cleanup
  jmp notDone

cleanup:
  xor ax,ax
  mov ds,ax
  mov ax,[cs:oldInt8]
  mov [8*4],ax
  mov ax,[cs:oldInt8+2]
  mov [8*4+2],ax
  mov ax,[cs:oldInt3]
  mov [3*4],ax
  mov ax,[cs:oldInt3+2]
  mov [3*4+2],ax
  mov ax,[cs:oldIntFE]
  mov [0xfe*4],ax
  mov ax,[cs:oldIntFE+2]
  mov [0xfe*4+2],ax
  mov ax,[cs:oldIntFF]
  mov [0xff*4],ax
  mov ax,[cs:oldIntFF+2]
  mov [0xff*4+2],ax
  mov ax,cs
  mov ds,ax

  ; Disable auto-EOI
  mov al,0x13  ; ICW4 needed, not cascaded, call address interval 8, edge triggered
  out 0x20,al  ; Set ICW1
  mov al,0x08  ; Interrupt vector address
  out 0x21,al  ; Set ICW2
  mov al,0x0d  ; 8086/8088 mode, normal EOI, buffered mode/master, not special fully nested mode
  out 0x21,al  ; Set ICW4
  mov al,[imr]
  out 0x21,al

  cmp word[singleTest],-1
  jne noFails

  mov si,passedMessage
  mov cx,failureMessage1 - passedMessage
  outputString
  mov ax,[passed]
  call outputDecimal
  outputCharacter '/'
  mov ax,[testCaseIndex]
  call outputDecimal
  outputCharacter 13
  outputCharacter 10
  mov ax,[firstFail]
  cmp ax,-1
  je noFails
  push ax
  mov si,failureMessage1
  mov cx,failureMessage2 - failureMessage1
  outputString
  pop ax
  call outputDecimal
  mov si,failureMessage2
  mov cx,failureMessage3 - failureMessage2
  outputString
  mov ax,[firstFailObserved]
  call outputDecimal
  mov si,failureMessage3
  mov cx,failureMessage4 - failureMessage3
  outputString
  mov ax,[firstFailExpected]
  call outputDecimal
  mov si,failureMessage4
  mov cx,failureMessageEnd - failureMessage4
  outputString
noFails:
  disconnect
  mov ax,0x4c00
  int 0x21
notDone:
  mov ax,[singleTest]
  cmp ax,-1
  je .notSingleTest
  cmp ax,[testCaseIndex]
  jne nextTestCase

;    mov ax,[testCaseIndex]
;    call outputDecimal
;    outputCharacter ' '
.notSingleTest:
;   outputCharacter 'm'
  call doMeasurement
;   outputCharacter 'c'
  mov ax,bx
  neg ax
  mov si,[testCaseOffset]
  cmp byte[si+3],0
  je adjustNoStub
  sub ax,4725-92 +12 - 30 + 2 - 8 ;-25  ; Recalculate this whenever we change the code between ***TIMING START***  and ***TIMING END***
  jmp doneAdjust
adjustNoStub:
  sub ax,4615 + 2095 ;-25 ; Recalculate this whenever we change the code between ***TIMING START***  and ***TIMING END***
doneAdjust:
  cmp word[singleTest],-1
  jne doSniffer
  cmp ax,[si]
  jne testFailed

  inc word[passed]

nextTestCase:
  inc word[testCaseIndex]
  mov bl,[si+5]      ; Number of preamble bytes
  mov bh,0
  lea si,[si+bx+6]   ; Points to instruction bytes count
  mov bl,[si]        ; Number of instruction bytes
  inc si             ; Points to first instruction byte
  add si,bx          ; Points to fixup count
  mov bl,[si]        ; Number of fixups
  inc si             ; Points to first fixup
  add si,bx
  mov [testCaseOffset],si

  jmp testLoop

testFailed:
  cmp word[firstFail],-1
  jne notFirstFail

  push ax
  mov ax,[testCaseIndex]
  mov [firstFail],ax
  pop ax
  add ax,210
  mov [firstFailObserved],ax
  mov si,[testCaseOffset]
  mov ax,[si]
  add ax,210
  mov [firstFailExpected],ax
notFirstFail:
  ;cmp word[singleTest],-1
  ;je nextTestCase
  jmp nextTestCase

doSniffer:
  outputCharacter 8  ; Sniffer to direct mode

  mov word[sniffer],0x8000

  outputCharacter 6  ; Start recording sniffer data

;  mov ax,[countedCycles]
;  add ax,210
;  mov si,[testCaseOffset]
;  cmp byte[si+3],0
;  jne noAdjustNoStub
;  sub ax,212
;noAdjustNoStub:
  mov bx,[si]
  cmp ax,bx
  jae noSatLow
  mov ax,bx
noSatLow:
  cmp ax,2047
  jb noSatHigh
  mov ax,2047
noSatHigh:
  mov [countedCycles],ax

  mov cx,16
loopTop:
  mov [savedCX],cx
;    mov word[countedCycles],2047
  call doMeasurement

  mov cx,2
outerFlush:
  push cx

  mov ax,[countedCycles]
  mov dx,25
  mul dx
  mov cx,ax
flushLoop2:
  loop flushLoop2

  pop cx
  loop outerFlush

  mov cx,[savedCX]
  loop loopTop2
  outputCharacter 7    ; Stop recording sniffer data
  jmp cleanup
loopTop2:
  jmp loopTop

doMeasurement:
  xor ax,ax
  mov ds,ax
  mov word[3*4],int3handler
  mov [3*4+2],cs
  mov bx,[0xffdf]

  mov ax,0x10a8
  mov ds,ax
  mov ax,cs
  mov es,ax
  mov cx,moveListCount
  mov si,moveList
  mov di,backup
backupLoop:
  cs lodsw
  xchg ax,si
  movsw
  xchg ax,si
  loop backupLoop
  xchg ax,bx
  stosw

  ; Patch this because changing the code would require recalculating the timings
  mov ax,cs
  mov ds,ax
  sub ax,0x10a8
  neg ax
  mov [cs:patchSegment+1],ax

  mov ax,0x10a8
  mov es,ax
  xor di,di
  mov si,[testCaseOffset]
  mov bl,[si+2]               ; nops/queuefiller

  mov bp,di
  mov dx,[si+3]               ; refresh
  mov cl,[si+5]               ; preamble bytes
  push si
  add si,6
  rep movsb

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
  cmp al,0x20
  jne notQueueFiller1
  mov ax,0x10b1  ; 'mov cl,16'
  stosw
  mov ax,0xe9d2  ; 'shl cl,cl'
  stosw
  jmp doneQueueFiller
notQueueFiller1:
  cmp al,0x40
  jne notQueueFiller2
  jmp doneQueueFiller
notQueueFiller2:
  jmp testFailed
doneQueueFiller:
  mov cl,bl
  and cl,0x1f
  mov al,0x90
  cmp cl,11
  jge irregularNops
regularNops:
  rep stosb
  jmp doneNops
irregularNops:
  cmp cl,11
  jne notCHNops
  stosb
  mov ax,0x2802  ; 'add ch,[bx+si]'
  stosw
  jmp doneNops
notCHNops:
  cmp cl,12
  jne notDLNops
  stosb
  mov ax,0x1002  ; 'add dl,[bx+si]'
  stosw
  jmp doneNops
notDLNops:
  cmp cl,13
  jne notAAANops
  stosb
  stosb
  stosb
  mov al,0x37    ; 'aaa'
  stosb
  jmp doneNops
notAAANops:
  cmp cl,14
  jne notLESNops
  mov ax,0x10c4  ; 'les ax,[bx+si]'
  stosw
  jmp doneNops
notLESNops:
  cmp cl,15
  jne notCMPNops
  mov ax,0x3880  ; 'cmp byte[bx+si],0'
  stosw
  mov al,0
  stosb
  jmp doneNops
notCMPNops:
  cmp cl,16
  jne notTESTNops
  mov ax,0x00f6  ; 'test byte[bx+si],0'
  stosw
  mov al,0
  stosb
  jmp doneNops
notTESTNops:
  mov al,0x90
  cmp cl,17
  jg regularNops
  mov cl,11
  jmp regularNops
doneNops:

  mov cl,[si]                 ; instruction bytes
  inc si
  cmp byte[badBIOS],0
  je .biosOK
  cmp word[si],0xd8fe
  je .skipTest
  cmp word[si],0xe8fe
  jne .biosOK
.skipTest:
  mov word[si],0
.biosOK:
  push bx
  mov bx,di
  rep movsb
  mov ax,0x00eb  ; 'jmp ip+0'
  stosw

  push di
  mov cl,[si]                 ; fixups
  inc si
  jcxz .overLoop
.loopTop:
  lodsb
  test al,0x80
  jnz .fixupMain
  ; fixup preamble
  cbw
  mov di,ax
  add word[es:di+bp],bx
  jmp .doneFixup
.fixupMain:
  and al,0x7f
  cbw
  mov di,ax
  add word[es:di+bx],bx
.doneFixup:
  loop .loopTop
.overLoop:
  pop di
  pop bx
  pop si

  mov ax,0xffcd  ; 'int 0xff'
  stosw
  xor ax,ax
  stosw
  stosw

%if 0
    push di
    push si
    push ds
    push cx
    push ax

    mov cx,22
.dump:
    lodsw
    outputHex
    loop .dump

    pop ax
    pop cx
    pop ds
    pop si
    pop di
%endif

%if 0
  cmp word[testCaseIndex],365
  jne .noDump
;  cmp word[sniffer],0x8000
;  jne .noDump
    push di
    push si
    push ds
    push cx
    push ax

    mov si,0
    mov ax,es
    mov ds,si ;ax
    mov cx,22
.dump:
    lodsw
    outputHex
    loop .dump

    pop ax
    pop cx
    pop ds
    pop si
    pop di
  .noDump:
%endif


;    push di
;    push si
;    push ds
;    push cx
;    push ax
;
;    mov si,-14
;    mov ax,es
;    mov ds,ax
;    mov cx,7
;.dumpStack:
;    lodsw
;    outputHex
;    loop .dumpStack
;
;    pop ax
;    pop cx
;    pop ds
;    pop si
;    pop di

;  xor ax,ax
;  mov ax,0x10
;  push ax
;  popf
;   outputCharacter 'a'

  cmp dl,0
  je snifferAdjustNoStub
  mov word[cs:patchSnifferInitialWait+1],6 + 774*3 ; 773*3
  jmp snifferDoneAdjust
snifferAdjustNoStub:
  mov word[cs:patchSnifferInitialWait+1],6 + 1634*3 ; (773 + 858 + 1 + 1)*3
snifferDoneAdjust:

  safeRefreshOff
  writePIT16 0, 2, 2    ; Ensure an IRQ0 is pending
  writePIT16 0, 2, 100  ; Queue an IRQ0 to execute from HLT
  sti
  hlt                   ; ACK first IRQ0
  hlt                   ; wait for second IRQ0
  writePIT16 0, 2, 0 ; Queue an IRQ0 for after the test in case of crash
  writePIT16 2, 2, 0        ; ***TIMING START***

  in al,0xe0

  push dx
;  cli
  xor ax,ax
  mov ds,ax
  mov word[0x20],irq0a
  mov [0x22],cs

  mov ds,[cs:sniffer]
  mov ax,[0]      ; Trigger: Start of command load sequence
  times 10 nop
  mov dl,16
  mov cx,[cs:savedCX]
  sub dl,cl

  outputByte
  mov si,[cs:testCaseOffset]
  mov dx,[cs:countedCycles]
  outputByte
  outputByte
patchSnifferInitialWait:
  mov dx,9999
  outputByte
  outputByte
  pop dx

  times 4 nop

  ; Clear prefetch queue to improve sniffer output
  jmp $+2

  ; Start of emulated code

  ; Set up and save some registers for test
  mov ax,cs
  mov ds,ax
  mov es,ax
  mov [savedSP],sp
  mov [savedSS],ss
  mov ax,cs
patchSegment:
  add ax,0x1000
  mov ss,ax
  mov word[testBuffer],0
  mov [testBuffer+2],ax

  ; Set up programmable delay
  mov bl,dh
  mov bh,0
  add bx,bx
  mov si,[delayTable + bx]
  mov di,patch+1
  mov cx,10
patchLoop:
  movsb
  add di,3
  loop patchLoop

  ; Set up some more registers
  mov ax,ss
  mov ds,ax
  mov es,ax
  xor bx,bx
  mov cx,bx
  mov si,bx
  mov di,bx
  mov bp,bx
  mov sp,bx

  ; Start refresh at requested rate
  mov al,(1 << 6) | BOTH | (2 << 1)
  out 0x43,al
  jmp $+2  ; Reset queue for sniffer decoder
  mov al,dl
  out 0x41,al
  mov al,0
  out 0x41,al

  ; Programmable delay 0-80 cycles (plus a constant)
  mov dl,1
patch:
  %rep 10
    mov al,0
    mul dl
  %endrep

  ; Set up very last registers and start test
  mov ax,bx
  mov dx,bx
  jmp far [cs:testBuffer]

int3handler:
  add sp,4
  popf
  retf

irq0:
  iret

irq0a:

interruptFF:
  mov al,0x70
  out 0x43,al
  mov al,0
  out 0x41,al
  out 0x41,al
  times 4 nop

  ; end of emulated code

  mov al,0x80
  out 0x43,al               ; ***TIMING END***
  in al,0x42
  mov bl,al
  in al,0x42
  mov bh,al

  mov al,0x80
  out 0x43,al
  in al,0x42
  mov cl,al
  in al,0x42
  mov ch,al

  mov al,0x80
  out 0x43,al
  in al,0x42
  mov dl,al
  in al,0x42
  mov dh,al

  mov al,0x80
  out 0x43,al
  in al,0x42
  mov ah,al
  in al,0x42
  xchg ah,al

  add bx,ax
  add bx,cx
  add bx,dx

doneTimer:
  xor ax,ax
  mov ds,ax
  mov word[0x20],irq0
  mov [0x22],cs
  writePIT16 0, 2, 0

  mov sp,[cs:savedSP]
  mov ss,[cs:savedSS]

  safeRefreshOn

; Restore the saved bytes
  mov ax,0x10a8
  mov es,ax
  mov ax,cs
  mov ds,ax
  mov cx,moveListCount
  mov si,moveList
  mov dx,backup
restoreLoop:
  lodsw
  xchg di,ax    ; di = restore dest
  xchg si,dx
  movsw
  xchg si,dx
  loop restoreLoop
  xchg si,dx
  xor ax,ax
  mov es,ax
  mov di,0xffdf
  movsw

  ret

interruptFE:
  xor bx,bx
  jmp doneTimer


outputDecimal:
  cmp ax,10000
  jae .d5
  cmp ax,1000
  jae .d4
  cmp ax,100
  jae .d3
  cmp ax,10
  jae .d2
  jmp .d1
.d5:
  mov bx,10000
  xor dx,dx
  div bx
  add al,'0'
  push dx
  outputCharacter
  pop ax
.d4:
  mov bx,1000
  xor dx,dx
  div bx
  add al,'0'
  push dx
  outputCharacter
  pop ax
.d3:
  mov bx,100
  xor dx,dx
  div bx
  add al,'0'
  push dx
  outputCharacter
  pop ax
.d2:
  mov bl,10
  div bl
  add al,'0'
  push ax
  outputCharacter
  pop ax
  mov al,ah
.d1:
  add al,'0'
  outputCharacter
  ret

bannerMessage: db "Acid88 v1.0 - https://github.com/reenigne/reenigne/tree/master/8088/acid88",13,10
passedMessage: db "Passed: "
failureMessage1: db "First failing test: "
failureMessage2: db ". Took "
failureMessage3: db " cycles, expected "
failureMessage4: db " cycles."
failureMessageEnd:
memoryMessage: db "Not enough memory"
memoryMessageEnd:

; The tests were designed to be run from segment 0x10a8, which (in a DOS
; program) we can't guarantee that we own. To avoid breaking TSRs etc.
; save the values from addresses in this segment before running tests and
; restore them afterwards. Interrupts (other than those we control) need
; to be off while these values are modified. The list below is all the
; addresses that the tests modify, as discovered by instrumenting XTCE.
moveList:
  dw 0,2,4,8,0xa,0xc,0xe,0x10,0x12,0x14,0x16,0x18,0x1a,0x1c,0x1e,0x20,0x22
  dw 0x24,0x26,0x28,0x2a,0x2c,0x2e,0x30,0x32,0x34
  dw 0x4000,0x4002,0x6000,0x6002,0x6004,0xa8f9,0xa8fb,0xa8fd
  dw 0xff00,0xffc0,0xfff4,0xfff6,0xfff8,0xfffa,0xfffc,0xfffe

imr: db 0
oldInt3: dw 0,0
oldInt8: dw 0,0
oldIntFE: dw 0,0
oldIntFF: dw 0,0
passed: dw 0
firstFail: dw -1
firstFailExpected: dw 0
firstFailObserved: dw 0
singleTest: dw -1
badBIOS: db 0

testCaseIndex: dw 0
testCaseOffset: dw 0
testBuffer: dw 0, 0
savedSP: dw 0
savedSS: dw 0
savedCX: dw 0
lut: db 0x88,8
sniffer: dw 0x7000
countedCycles: dw 1
testSP: dw 0

backup: times (moveListCount+1) dw 0

delayData:
%assign i 0
%rep 81
  %assign k i
  %assign j 0
  %rep 10
    %if k <= 0
      db 0
    %elif k >= 8
      db 0xff
    %else
      db 0xff >> (8-k)
    %endif
    %assign k k-8
    %assign j j+1
  %endrep
  %assign i i+1
%endrep

delayTable:
%assign i 0
%rep 81
  dw delayData + i*10
  %assign i i+1
%endrep

testCases:

; Format of testCases:
;   2 bytes: total length of testCases data excluding length field
;   For each testcase:
;     2 bytes: cycle count
;     1 byte: queueFiller operation (0 = MUL, 1 = shift, 2 = nothing) * 32 + number of NOPs
;     1 byte: refresh period
;     1 byte: refresh phase
;     1 byte: number of preamble bytes
;     N bytes: preamble
;     1 byte: number of instruction bytes
;     N bytes: instructions
;     1 byte: number of fixups
;     N entries:
;       1 byte: offset of address to fix up relative to start of preamble/main. Offset of main is added to value at this address + 128*(0 = address in preamble, 1 = address in main code)


