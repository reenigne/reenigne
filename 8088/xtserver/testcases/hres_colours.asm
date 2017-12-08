  %include "../../defaults_bin.asm"

  lockstep
  refreshOff

  mov ax,0xb800
  mov ds,ax
  mov cx,1000
  rep lodsw

  mov ax,0
  mov ds,ax
  setInterrupt 8, interrupt8
  writePIT16 0, 0, 1

  mov dx,0x3d8
  mov al,9
  out dx,al
  inc dx
  mov al,6
  out dx,al

  mov dx,0x3d4

  mov ax,((114 - 1) << 8) | 0x00       ;Horizontal total
  out dx,ax
  mov ax,(18 << 8) | 0x01              ;Horizontal displayed
  out dx,ax
  mov ax,(90 << 8) | 0x02              ;Horizontal sync position
  out dx,ax
  mov ax,0x0003                        ;Horizontal sync width
  out dx,ax
;  mov ax,(((32 + 75) - 1) << 8) | 0x04 ;Vertical total
  mov ax,((114 - 1) << 8) | 0x04       ;Vertical total
  out dx,ax
  mov ax,0x0605                        ;Vertical adjust
  out dx,ax
  mov ax,(100 << 8) | 0x06             ;Vertical displayed
  out dx,ax
;  mov ax,((28 + 75) << 8) | 0x07       ;Vertical sync position
  mov ax,(106 << 8) | 0x07             ;Vertical sync position
  out dx,ax
  mov ax,0x0208                        ;Interlace mode
  out dx,ax
;  mov ax,0x0709                        ;Maximum scanline
  mov ax,0x0309                        ;Maximum scanline

  out dx,ax
  mov ax,0x000c                        ;Start address
  out dx,ax
  inc ax
  out dx,ax

  mov ax,0xb800
  mov es,ax
  xor di,di
  cld

  mov cl,0
loopTop:
  mov ah,cl

  mov al,0x4d
  stosw
  stosw
  mov al,0xb0
  stosw
  stosw
  mov al,0x55
  stosw
  stosw
  mov al,0xb1
  stosw
  stosw
  mov al,0x13
  stosw
  stosw
  mov al,0x14
  stosw
  mov al,0
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  or ah,al
  mov al,9
  stosw

  mov ah,cl
  inc ah

  mov al,0x4d
  stosw
  stosw
  mov al,0xb0
  stosw
  stosw
  mov al,0x55
  stosw
  stosw


;  mov ah,cl
;
;  mov al,0x4d
;  stosw
;  stosw
;  mov al,0xb0
;  stosw
;  stosw
;  mov al,0x55
;  stosw
;  stosw
;  mov al,0xb1
;  stosw
;  stosw
;  mov al,0x13
;  stosw
;  stosw
;  mov al,0x14
;  stosw
;  mov al,0
;  shr ax,1
;  shr ax,1
;  shr ax,1
;  shr ax,1
;  or ah,al
;  mov al,9
;  stosw
;
;  mov ah,cl
;  inc ah
;
;  mov al,0x4d
;  stosw
;  stosw
;  mov al,0xb0
;  stosw
;  stosw
;  mov al,0x55
;  stosw
;  stosw




  mov al,0xb1
  stosw
  stosw
  mov al,0x13
  stosw
  stosw
  mov al,0x14
  stosw
  mov al,0
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  or ah,al
  mov al,9
  stosw

  mov ah,cl
  inc ah
  inc ah

  mov al,0x4d
  stosw
  stosw
  mov al,0xb0
  stosw
  stosw
  mov al,0x55
  stosw
  stosw
  mov al,0xb1
  stosw
  stosw
  mov al,0x13
  stosw
  stosw
  mov al,0x14
  stosw
  mov al,0
  shr ax,1
  shr ax,1
  shr ax,1
  shr ax,1
  or ah,al
  mov al,9
  stosw


;  mov ah,cl
;  inc ah
;
;  mov al,0xb1
;  stosw
;  stosw
;  mov al,0x13
;  stosw
;  stosw
;  mov al,0x14
;  stosw
;  mov al,0
;  shr ax,1
;  shr ax,1
;  shr ax,1
;  shr ax,1
;  or ah,al
;  mov al,9
;  stosw
;
;  mov ah,cl
;  inc ah
;  inc ah

;  mov al,0x4d
;  stosw
;  stosw
;  mov al,0xb0
;  stosw
;  stosw
;  mov al,0x55
;  stosw
;  stosw
;  mov al,0xb1
;  stosw
;  stosw
;  mov al,0x13
;  stosw
;  stosw
;  mov al,0x14
;  stosw
;  mov al,0
;  shr ax,1
;  shr ax,1
;  shr ax,1
;  shr ax,1
;  or ah,al
;  mov al,9
;  stosw


  inc cx
  inc cx
  inc cx
  cmp cx,256
  jge drawComplete
  jmp loopTop
drawComplete:

  mov ax,0
  mov cx,0x4000
  sub cx,di
  shr cx,1
  rep stosw


  mov dx,0x3da
  waitForVerticalSync
  waitForNoVerticalSync

  times 21454 - 76*180 nop

  writePIT16 0, 2, 76*262

  mov dx,0x3d4
  sti
waitLoop:
  hlt
  jmp waitLoop


drawBottom EQU 0


interrupt8:
  mov dl,0xd9
  mov al,0x0f
  out dx,al
  mov al,6
  out dx,al
  times 26 nop
  mov al,0x0a
  out dx,al
  mov al,6
  out dx,al
  mov dl,0xd4
  times 27 nop

  mov ax,((20 - 1) << 8) | 0x00        ;Horizontal total
  out dx,ax

;  %if drawBottom != 0
;    mov ax,0x150c
;    out dx,ax
;    mov ax,0x180d
;    out dx,ax
;  %else
    mov ax,0x070c
    out dx,ax
    mov ax,0x080d
    out dx,ax
;  %endif

  mov ax,((100 - 1) << 8) | 0x04       ;Vertical total
  out dx,ax
  times 11 nop
  mov ax,((54 - 1) << 8) | 0x00        ;Horizontal total
  out dx,ax
  mov ax,(18 << 8) | 0x01              ;Horizontal displayed
  out dx,ax
  mov ax,(30 << 8) | 0x02              ;Horizontal sync position
  out dx,ax
  mov ax,0x0005                        ;Vertical adjust
  out dx,ax
  times 9 nop

  %rep 99
    mov ax,((20 - 1) << 8) | 0x00        ;Horizontal total
    out dx,ax
    mov ax,(18 << 8) | 0x01              ;Horizontal displayed
    out dx,ax
    mov ax,(30 << 8) | 0x02              ;Horizontal sync position
    out dx,ax
    mov ax,0x0003                        ;Horizontal sync width
    out dx,ax
    times 12 nop
    mov ax,((54 - 1) << 8) | 0x00        ;Horizontal total
    out dx,ax
    mov ax,(18 << 8) | 0x01              ;Horizontal displayed
    out dx,ax
    mov ax,(30 << 8) | 0x02              ;Horizontal sync position
    out dx,ax
    mov ax,0x0003                        ;Horizontal sync width
    out dx,ax
    times 8 nop
  %endrep

  mov ax,((20 - 1) << 8) | 0x00        ;Horizontal total
  out dx,ax

;  %if drawBottom != 0
;    mov ax,0x0e0c
;    out dx,ax
;    mov ax,0x100d
;    out dx,ax
;  %else
    mov ax,0x000c
    out dx,ax
    mov ax,0x000d
    out dx,ax
;  %endif

  mov ax,((114 - 1) << 8) | 0x04       ;Vertical total
  out dx,ax
  times 12 nop
  mov ax,((54 - 1) << 8) | 0x00        ;Horizontal total
  out dx,ax
  mov ax,(18 << 8) | 0x01              ;Horizontal displayed
  out dx,ax
  mov ax,(30 << 8) | 0x02              ;Horizontal sync position
  out dx,ax
  mov ax,0x0605                        ;Vertical adjust
  out dx,ax
  times 8 nop

  %rep 99
    mov ax,((20 - 1) << 8) | 0x00        ;Horizontal total
    out dx,ax
    mov ax,(18 << 8) | 0x01              ;Horizontal displayed
    out dx,ax
    mov ax,(30 << 8) | 0x02              ;Horizontal sync position
    out dx,ax
    mov ax,0x0003                        ;Horizontal sync width
    out dx,ax
    times 12 nop
    mov ax,((54 - 1) << 8) | 0x00        ;Horizontal total
    out dx,ax
    mov ax,(18 << 8) | 0x01              ;Horizontal displayed
    out dx,ax
    mov ax,(30 << 8) | 0x02              ;Horizontal sync position
    out dx,ax
    mov ax,0x0003                        ;Horizontal sync width
    out dx,ax
    times 8 nop
  %endrep


  mov ax,((114 - 1) << 8) | 0x00  ;Horizontal total
  out dx,ax
  mov ax,(90 << 8) | 0x02         ;Horizontal sync position
  out dx,ax
  mov ax,0x0003                   ;Horizontal sync width
  out dx,ax

  times 4500 nop

  mov al,0x20
  out 0x20,al
  iret

; There is about enough time for 10 word-sized register writes per scanline
