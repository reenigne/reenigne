%include "../../defaults_bin.asm"

tempBuffer equ codeEnd
tempBufferSize equ 16
stackStart equ tempBuffer + tempBufferSize
stackSize equ 0x100
pspStart equ (15 + stackStart + stackSize) & 0xfff0
pspEnd equ 0x100 + pspStart
codeStart equ pspEnd

  ; Set up stack
  cli
  mov ax,cs
  mov ss,ax
  mov sp,stackStart + 0x100
  sti
  mov es,ax

  mov ax,0
  mov ds,ax

%macro setResidentInterrupt 2
  mov word [%1*4], %2
  mov [%1*4 + 2], cs
%endmacro

  mov ax,[0x10*4]
  mov [es:oldInterrupt10],ax
  mov ax,[0x10*4 + 2]
  mov [es:2+oldInterrupt10],ax

  setResidentInterrupt 0x10, int10Routine
  setResidentInterrupt 0x20, int20Routine
  setResidentInterrupt 0x21, int21Routine
  setResidentInterrupt 0x22, int22Routine
  setResidentInterrupt 0x23, int23Routine
  setResidentInterrupt 0x24, int24Routine
  setResidentInterrupt 0x25, int25Routine
  setResidentInterrupt 0x26, int26Routine
  setResidentInterrupt 0x27, int27Routine
  setResidentInterrupt 0x28, int28Routine
  setResidentInterrupt 0x29, int29Routine
  setResidentInterrupt 0x2e, int2eRoutine
  setResidentInterrupt 0x2f, int2fRoutine

  mov di,codeStart
  loadData




oldInterrupt10:
  dw 0, 0


int10Routine:
  cmp ah,0x0e
  jne .noIntercept
  push ax
  mov al,0x0a  ; OCW3 - no bit 5 action, no poll command issued, act on bit 0,
  out 0x20,al  ;  read Interrupt Request Register
  pop ax

  push ax
  push bx
  push cx
  push dx
  push si
  push di
  push ds
  call sendChar
  pop ds
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
.noIntercept:
  jmp far [cs:oldInterrupt10 - residentPortion]


int21Routine:
  push es
  push di
  push ax
  mov ax,cs
  mov es,ax
  mov di,tempBuffer
  cld
  mov al,0x05
  stosb             ; Host command
  mov al,0x21
  stosb             ; Interrupt number
  pop ax
  stosw
  push ax        ; Save sector count
  mov ax,bx
  stosw
  mov ax,cx
  stosw
  mov ax,dx
  stosw
  mov ax,bp
  stosw
  mov ax,si
  stosw
  mov ax,di
  stosw

  ; Do the actual send
  mov ax,cs
  mov ds,ax
  mov si,tempBuffer - residentPortion
  mov ah,0
  mov cx,19
  call sendLoop

  pop cx             ; ...in CL and CH respectively
  pop ax         ; Saved sector count from above
  mov ah,0
  add cl,7
  shl ax,cl      ; Return number of bytes to read/write in AX and sector count in CH
  pop bx
  pop es
  ret

codeEnd:



align 16

stackStart:
