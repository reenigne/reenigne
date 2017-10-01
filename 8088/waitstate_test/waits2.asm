org 0x100
cpu 8086

  mov dx,0x213
  mov al,0
  out dx,al

  mov cx,12
  xor ax,ax
mainLoop:
  push ax
  push cx
  call doTest
  pop cx
  pop ax
  add ax,0x1000
  loop mainLoop
  mov ax,0xb800
  call doTest
  mov ax,0xc000
  call doTest
  mov ax,0xd000
  call doTest
  mov ax,0xe000
  call doTest
  mov ax,0xf000
  call doTest

  mov dx,0x213
  mov al,1
  out dx,al

  ret

printNybble:
  and al,0xf
  cmp al,10
  jge .letters
  add al,'0'
  jmp printCharacter
.letters:
  add al,'A'-10

printCharacter:
  mov dl,al
  mov ah,2
  int 0x21
  ret

printHex:
  push ax
  mov al,ah
  mov cl,4
  shr al,cl
  call printNybble
  pop ax
  push ax
  mov al,ah
  call printNybble
  pop ax
  push ax
  mov cl,4
  shr al,cl
  call printNybble
  pop ax
  call printNybble
  ret

doTest:
  push ax
  call printHex
  mov al,':'
  call printCharacter
  mov al,' '
  call printCharacter
  pop ax

  push ax
  mov bx,test1
  call doMeasurement
  pop bx
  xchg bx,ax
  push bx
  mov bx,test2
  call doMeasurement
  pop bx
  sub ax,bx
  sub ax,0x600  ; (1 byte for "POP AX" opcode + 2 bytes for popped data)*0x200

;
;  call printNybble
  call printHex
  mov al,10
  call printCharacter
  mov al,13
  call printCharacter
  ret

  ; 8253 PIT Mode control (port 0x43) values

  TIMER0 EQU 0x00
  TIMER1 EQU 0x40
  TIMER2 EQU 0x80

  LATCH  EQU 0x00
  LSB    EQU 0x10
  MSB    EQU 0x20
  BOTH   EQU 0x30 ; LSB first, then MSB

  MODE0  EQU 0x00 ; Interrupt on terminal count: low during countdown then high                            (useful for PWM)
  MODE1  EQU 0x02 ; Programmable one shot      : low from gate rising to end of countdown
  MODE2  EQU 0x04 ; Rate generator             : output low for one cycle out of N                         (useful for timing things)
  MODE3  EQU 0x06 ; Square wave generator      : high for ceil(n/2) and low for floor(n/2)                 (useful for beepy sounds)
  MODE4  EQU 0x08 ; Software triggered strobe  : high during countdown then low for one cycle
  MODE5  EQU 0x0a ; Hardware triggered strobe  : wait for gate rising, then high during countdown, then low for one cycle

  BINARY EQU 0x00
  BCD    EQU 0x01

%macro refreshOff 0
  mov al,TIMER1 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01  ; Count = 0x0001 so we'll stop almost immediately
  out 0x41,al
%endmacro

%macro refreshOn 0
  refreshOn 18
%endmacro

%macro refreshOn 1
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,%1
  out 0x41,al  ; Timer 1 rate
%endmacro

%macro ensureRefresh 0
  cli
  cld

  xor ax,ax
  mov ds,ax
  mov si,ax

  ; Delay for enough time to refresh 512 columns
  mov cx,256

  ; Increase refresh frequency to ensure all DRAM is refreshed before turning
  ; off refresh.
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,2
  out 0x41,al  ; Timer 1 rate

  rep lodsw
%endmacro

%macro safeRefreshOff 0
  ensureRefresh
  ; We now have about 1.5ms during which refresh can be off
  refreshOff
%endmacro

%macro safeRefreshOn 0
  ensureRefresh
  refreshOn
%endmacro

%macro safeRefreshOn 1
  ensureRefresh
  refreshOn %1
%endmacro

doMeasurement:
  push ax
  safeRefreshOff

  pop ax
  mov [cs:savedStack],sp
  mov [cs:savedStack + 2],ss
  mov ss,ax
  xor sp,sp

  mov al,TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0x00
  out 0x40,al
  out 0x40,al

  jmp bx

test1:
  times 0x200 pop ax
;  pop ax
;  pop ax
;  pop ax
;  pop ax
test2:
  times 0x200 pop ax
;  pop ax
;  pop ax
;  pop ax
;  pop ax

  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al

  safeRefreshOn

  mov sp,[cs:savedStack]
  mov ss,[cs:savedStack + 2]

  sti

  xchg bx,ax
  ret

savedStack:
