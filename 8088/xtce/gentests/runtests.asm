%include "../../defaults_bin.asm"

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

  ; Enable PIT channel 2. We'll use this for timing so that tests can do IRQ0s.
  in al,0x61
  and al,0xfc
  or al,1
  out 0x61,al

  ; Enable auto-EOI
  mov al,0x13  ; ICW4 needed, not cascaded, call address interval 8, edge triggered
  out 0x20,al  ; Set ICW1
  mov al,0x08  ; Interrupt vector address
  out 0x21,al  ; Set ICW2
  mov al,0x0f  ; 8086/8088 mode, auto-EOI, buffered mode/master, not special fully nested mode
  out 0x21,al  ; Set ICW4
  mov al,0xbc  ; Enable IRQs 0 (timer), 1 (keyboard) and 6 (floppy disk).
  out 0x21,al  ; Leave disabled 2 (EGA/VGA/slave 8259) 3 (COM2/COM4), 4 (COM1/COM3), 5 (hard drive, LPT2) and 7 (LPT1)

  xor ax,ax
  mov ds,ax
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
  mov ax,si
  sub ax,testCases+2
  cmp ax,[testCases]
  jb notDone

  mov si,passMessage
  mov cx,5
  outputString
  mov ax,[testCaseIndex]
  call outputDecimal
  outputCharacter 10

  complete
notDone:
;    mov ax,[testCaseIndex]
;    call outputDecimal
;    outputCharacter ' '

  call doMeasurement
  mov ax,bx
  neg ax
  mov si,[testCaseOffset]
  cmp byte[si+3],0
  je adjustNoStub
  sub ax,4725-92 +12 - 30 + 2 - 8  ; Recalculate this whenever we change the code between ***TIMING START***  and ***TIMING END***
  jmp doneAdjust
adjustNoStub:
  sub ax,4615 + 2095 ; Recalculate this whenever we change the code between ***TIMING START***  and ***TIMING END***
doneAdjust:
  cmp ax,[si]
  jne testFailed

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
  push ax

;  cmp byte[si+3],0
;  jne noStubAdjustCount
;  add ax,212
;noStubAdjustCount:
  mov [countedCycles],ax

  mov ax,[testCaseIndex]
  outputHex
  outputCharacter ' '

  outputCharacter 'o'
  pop ax
  call outputDecimal
  outputCharacter ' '
  outputCharacter 'e'
  mov si,[testCaseOffset]
  mov ax,[si]
  call outputDecimal
  outputCharacter 10

  mov si,failMessage
  mov cx,5
  outputString
  mov ax,[testCaseIndex]
  call outputDecimal
  outputCharacter ','
  mov ax,[countedCycles]
  call outputDecimal

  outputCharacter 10
  outputCharacter 'x'

  mov word[sniffer],0x8000

  outputCharacter 6

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
  mov ax,0x00f6  ; 'test byte[bx+si],0'
  stosw
  mov al,0
  stosb
doneNops:

  mov cl,[si]                 ; instruction bytes
  inc si
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
;  cmp word[sniffer],0x8000
;  jne .noDump
    push di
    push si
    push ds
    push cx
    push ax

    mov si,0
    mov ax,es
    mov ds,ax
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
;  .noDump:
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

  cmp dl,0
  je snifferAdjustNoStub
  mov word[cs:patchSnifferInitialWait+1],6 + 773*3
  jmp snifferDoneAdjust
snifferAdjustNoStub:
  mov word[cs:patchSnifferInitialWait+1],6 + (773 + 858 + 1 + 1)*3
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

  mov ax,cs
  mov ds,ax
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


failMessage: db "FAIL "
passMessage: db "PASS "

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


