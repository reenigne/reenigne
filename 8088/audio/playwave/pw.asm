cpu 8086
segment _TEXT public class=CODE

interrupt9:
  in al,0x60
  cmp al,1
  jne noEnd
  mov cx,di
  xor si,si
noEnd:
  jmp far [cs:int9save]

global _playWithTimer
_playWithTimer:
  push bp
  mov bp,sp
  push si
  push di
  pushf
  push es
  push ds
  push bp

  mov ax,cs
  add ax,(opTable - $$) >> 4
  mov es,ax

  cli

  in al,0x61
  or al,3
  out 0x61,al

  mov al,0x90  ; TIMER2 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01
  out 0x42,al  ; Counter 2 count = 1 - terminate count quickly

  mov al,0x14  ; TIMER0 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,[bp+8]   ; p2 = Counter value
  out 0x40,al  ; Counter 0 count

  xor ax,ax
  mov [cs:canary],ax
  mov ds,ax
  mov ax,[8*4]
  mov [cs:int8save],ax
  mov ax,[8*4+2]
  mov [cs:int8save+2],ax
  mov ax,[9*4]
  mov [cs:int9save],ax
  mov ax,[9*4+2]
  mov [cs:int9save+2],ax
  mov ax,[bp+10]  ; p3 = interrupt vector
  mov word[8*4],ax  ;interrupt8
  mov [8*4+2],cs
  mov word[9*4],interrupt9
  mov [9*4+2],cs

  mov dx,0x20
  mov bx,0x0f
  mov cx,[bp+6]  ; p1 = Last data para +1
  xor si,si
  mov ax,[bp+4]  ; p0 = Initial data para
  mov ds,ax
  mov di,ax
  mov ah,0
  mov bp,sp

playSample:
  sti
  inc word[cs:canary]
  hlt
global _interrupt8test
_interrupt8test:
  lodsb
  out 0x42,al
  mov al,[es:si]
  add di,ax
  mov ds,di
  and si,bx
  mov al,dl
  out dx,al
  mov sp,bp
  cmp di,cx
  jne playSample

playSampleDone:
  mov sp,bp
  xor ax,ax
  mov ds,ax
  mov ax,[8*4]
  mov [cs:int8save],ax
  mov ax,[8*4+2]
  mov [cs:int8save+2],ax
  mov ax,[9*4]
  mov [cs:int9save],ax
  mov ax,[9*4+2]
  mov [cs:int9save+2],ax

  pop bp
  mov cx,[bp+12]  ; p4 = number of times to catch up
  jcxz noCatchUp
  mov al,0x10  ; TIMER0 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,1

  sti
catchUpLoop:
  out 0x40,al
  loop catchUpLoop

noCatchUp:
  mov al,0x34  ; TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov ax,[cs:canary]
  pop ds
  pop es
  popf
  pop di
  pop si
  pop bp
  ret


playSample2:
  sti
  hlt
global _interrupt8play
_interrupt8play:
  lodsb
  out 0x42,al
  mov al,[es:si]
  add di,ax
  mov ds,di
  and si,bx
  mov al,dl
  out dx,al
  mov sp,bp
  cmp di,cx
  jne playSample2
  jmp playSampleDone


align 16

; ES points here

opTable:
  db 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1
int8save:
  dw 0,0
int9save:
  dw 0,0
canary:
  dw 0


;global _readTimer0
;_readTimer0:
;  in al,0x40
;  mov ah,al
;  in al,0x40
;  xchg ah,al
;  ret
;
;
;global _resetTimer0
;_resetTimer0:
;  mov al,0x34  ;TIMER0 | BOTH | MODE2 | BINARY
;  out 0x43,al
;  xor al,al
;  out 0x40,al
;  out 0x40,al
;  ret


global _maxNops
_maxNops:
  mov ax,playCalibratedLoopEnd - playCalibratedLoopNops
  ret


global _playCalibrated
_playCalibrated:
  push bp
  mov bp,sp
  push si
  push di
  push es
  push ds

  mov ax,cs
  add ax,(opTable - $$) >> 4
  mov es,ax

  in al,0x61
  or al,3
  out 0x61,al

  cli
  xor ax,ax
  mov ds,ax
  mov ax,[8*4]
  mov [cs:int8save],ax
  mov ax,[8*4+2]
  mov [cs:int8save+2],ax
  mov word[8*4],interrupt8
  mov [8*4+2],cs
  mov ax,[9*4]
  mov [cs:int9save],ax
  mov ax,[9*4+2]
  mov [cs:int9save+2],ax
  mov word[9*4],interrupt9
  mov [9*4+2],cs

  mov al,0x90  ; TIMER2 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01
  out 0x42,al  ; Counter 2 count = 1 - terminate count quickly

  mov al,0x10  ; TIMER0 | BOTH | MODE0 | BINARY
  out 0x43,al
  mov al,0x10
  out 0x40,al
  mov al,0
  out 0x40,al  ; Counter 0 count = 16 - terminate count quickly

  hlt
interrupt8:
  mov al,0x20
  out 0x20,al
  add sp,6

  mov bx,[bp+8]  ; p2 = number of NOPs
  mov ax,bx
  add ax,playCalibratedLoopNops + 2 - playCalibratedLoopTop
  mov ah,al
  mov al,0x75
  mov [cs:playCalibratedLoopNops + bx],ax

  mov dx,0x42
  mov bx,0x0f
  mov cx,[bp+6]  ; p1 = Last data para +1
  xor si,si
  mov ax,[bp+4]  ; p0 = Initial data para
  mov ds,ax
  mov di,ax
  mov ah,0
  sti

playCalibratedLoopTop:
  lodsb                ; 2
  out dx,al            ; 2  dx == 0x42
  mov al,[es:si]       ; 4  es:0 points at opTable
  add di,ax            ; 2
  and si,bx            ; 2  bx == 0x0f
  mov ds,di            ; 2
  cmp di,cx            ; 2
playCalibratedLoopNops:
  times 126-(playCalibratedLoopNops-playCalibratedLoopTop) cld
playCalibratedLoopEnd:
  jne playCalibratedLoopTop

  cli

  in al,0x40
  mov bl,al
  in al,0x40
  mov bh,al
  neg bx

  xor ax,ax
  mov ds,ax
  mov ax,[8*4]
  mov [cs:int8save],ax
  mov ax,[8*4+2]
  mov [cs:int8save+2],ax
  mov ax,[9*4]
  mov [cs:int9save],ax
  mov ax,[9*4+2]
  mov [cs:int9save+2],ax
  sti

  mov cx,[bp+10]  ; p3 = number of times to catch up
  jcxz noCatchUpC
  mov al,0x10  ; TIMER0 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,1

catchUpLoopC:
  out 0x40,al
  loop catchUpLoopC

noCatchUpC:
  mov al,0x34  ; TIMER0 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov di,[bp+8]
  mov word[cs:playCalibratedLoopNops + di],0xfcfc  ; cld cld

  mov ax,bx

  pop ds
  pop es
  pop di
  pop si
  pop bp
  ret
global _playCalibratedEnd
_playCalibratedEnd:


global _muld
_muld:
  push bp
  mov bp,sp
  mov ax,[bp+4]
  mul word[bp+6]
  div word[bp+8]
  pop bp
  ret


global _mulh
_mulh:
  push bp
  mov bp,sp
  mov ax,[bp+4]
  mul word[bp+6]
  mov ax,dx
  pop bp
  ret


global _openHandle
_openHandle:
  push bp
  mov bp,sp
  mov word[errorCode],0
  mov ax,0x3d00
  mov dx,[bp+4]
  int 0x21
  jnc .noError
  mov [errorCode],ax
.noError:
  pop bp
  ret


global _closeHandle
_closeHandle:
  push bp
  mov bp,sp
  mov ah,0x3e
  mov bx,[bp+4]
  pop bp
  ret


global _readHandle
_readHandle:
  push bp
  mov bp,sp
  push ds
  mov word[errorCode],0
  mov bx,[bp+4]
  mov cx,[bp+6]
  mov dx,[bp+8]
  mov ds,[bp+10]
  int 0x21
  jnc .noError
  mov [errorCode],ax
.noError:
  pop ds
  pop bp
  ret


global _osError
_osError:
  mov ax,[errorCode]
  ret


global _dosAlloc
_dosAlloc:
  push bp
  mov bp,sp
  mov word[errorCode],0
  mov ah,0x48
  mov bx,[bp+4]
  int 0x21
  jnc .noError
  mov [errorCode],ax
.noError:
  pop bp
  ret


global _dosFree
_dosFree:
  push bp
  mov bp,sp
  mov ah,0x49
  mov es,[bp+4]
  int 0x21
  pop bp
  ret


global _totalMemory
_totalMemory:
  int 0x12
  ret


segment _DATA public class=DATA

global errorCode
errorCode:
  dw 0


;playKernelUnrolled:
;  lodsb              ; 2      }
;  out dx,al          ; 2      } x15
;
;  lodsb                     ; 2
;  out dx,al                 ; 2
;  inc di                    ; 1
;  mov ds,di                 ; 2
;  loop playKernelUnrolled   ; 2
;
;
; Calibrating unrolled loops:
;  N   iterations of loop unrolled to M      = a
;  N+P iterations of loop unrolled to M      = b
;  N   iterations of loop unrolled to M+Q    = c
;  N+P iterations of loop unrolled to M+Q    = d
;
;  1 unroll step = (c-a)/Q
;  1 iteration = (b-a)/P
;  1 non-iteration step = (b-a)/P - M*(c-a)/Q

