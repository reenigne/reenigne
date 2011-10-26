org 0

  mov al,0x34
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  cli

  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov ax,0xb800
  mov es,ax
  mov sp,endCode + 8192

mainLoop:
  mov si,endCode
  mov di,0
;  mov di,endCode + 8192
  mov cx,2048

  mov al,0x70  ; Timer 1, write LSB+MSB, mode 0, binary
  out 0x43,al
  mov al,0
  out 0x41,al
  out 0x41,al

  ; Wait for any pending refresh to occur (unnecessary?)
  times 18 nop

  ; Reset timer 0 so that the CPU is in lockstep with timer 0
  mov al,0x34  ; Timer 0, write LSB+MSB, mode 2, binary
  out 0x43,al
  xor al,al
  out 0x40,al
  out 0x40,al

  in al,0x40
  mov ah,al
  in al,0x40
  xchg ax,dx

;  rep movsw

%rep 2048
;  lodsw
;  push ax
  stosw
  nop
  nop
  nop
  nop
  nop
  nop
%endrep

;  rep stosb

  in al,0x40
  mov ah,al
  in al,0x40
  mov bl,al

  mov al,0x54  ; Timer 1, write LSB, mode 2, binary
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  mov al,bl
  xchg ah,al
  xchg dh,dl
  sub ax,dx
  neg ax
  int 0x60
  mov al,10
  int 0x62

  jmp mainLoop

align 16
endCode:

; Want 16384 IOs
; => 8192 bytes moved



;%	000-00F  8237 DMA controller
;	000 Channel 0 address register
;	001 Channel 0 word count
;	002 Channel 1 address register
;	003 Channel 1 word count
;	004 Channel 2 address register
;	005 Channel 2 word count
;	006 Channel 3 address register
;	007 Channel 3 word count
;	008 Status/command register
;	009 Request register
;	00A Mask register
;	00B Mode register
;	00C Clear MSB/LSB flip flop
;	00D Master clear temp register
;	00E Clear mask register
;	00F Multiple mask register
;
;
;        MOV     AL,0FFH                 ; SET CNT OF 64K FOR REFRESH
;        OUT     DMA+1,AL
;        OUT     DMA+1,AL
;
;        MOV     AL,058H                 ; SET DMA MODE,CH 0,RD.,AUTOINT
;        OUT     DMA+0BH,AL              ; WRITE DMA MODE REG
;
;        MOV     AL,0                    ; ENABLE DMA CONTROLLER
;        MOV     CH,AL                   ; SET COUNT HIGH=00
;        OUT     DMA+8,AL                ;SETUP DMA COMMAND REG
;        OUT     DMA+10,AL               ; ENABLE DMA CH 0
;
;        MOV     AL,18                   ; START TIMER 1
;        OUT     TIMER+1,AL


