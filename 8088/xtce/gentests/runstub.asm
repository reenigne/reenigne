org 0
cpu 8086

  xor ax,ax
  mov ds,ax
  mov word[0x20],irq0a
  mov [0x22],cs
  mov word[0xff*4],interruptFF
  mov [0xff*4+2],cs
  mov word[3*4],int3handler
  mov [3*4+2],cs

  ; Code executed on real hardware starts here

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

int3handler:
  add sp,4
  popf
  retf

irq0a:

interruptFF:
  refreshOff
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

