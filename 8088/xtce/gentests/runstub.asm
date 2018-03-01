org 0
cpu 8086

  xor ax,ax
  mov ds,ax
  mov word[0x20],irq0a
  mov [0x22],cs
  mov word[0xff*4],interruptFF
  mov [0xff*4+2],cs
  mov word[0xfe*4],interruptFF
  mov [0xfe*4+2],cs
  mov word[3*4],int3handler
  mov [3*4+2],cs

  ; Enable auto-EOI
  mov al,0x13  ; ICW4 needed, not cascaded, call address interval 8, edge triggered
  out 0x20,al  ; Set ICW1
  mov al,0x08  ; Interrupt vector address
  out 0x21,al  ; Set ICW2
  mov al,0x0f  ; 8086/8088 mode, auto-EOI, buffered mode/master, not special fully nested mode
  out 0x21,al  ; Set ICW4
  mov al,0xbc  ; Enable IRQs 0 (timer), 1 (keyboard) and 6 (floppy disk).
  out 0x21,al  ; Leave disabled 2 (EGA/VGA/slave 8259) 3 (COM2/COM4), 4 (COM1/COM3), 5 (hard drive, LPT2) and 7 (LPT1)


  ; Code executed on real hardware starts here

  jmp $+2

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
  mov al,(1 << 6) | 0x30 | (2 << 1)
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

  ; Code executed on real hardware ends here

savedSP: dw 0
savedSS: dw 0
testBuffer: dw 0, 0

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

