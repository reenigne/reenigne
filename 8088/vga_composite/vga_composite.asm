  %include "../defaults_bin.asm"

  stopScreen
;  outputCharacter 'A'

  mov dx,0xc000
romScanLoop:
  mov ds,dx
  cmp word[0],0xaa55
  jnz noRom

  push dx
;  outputCharacter 'e'

  mov ax,0x40
  mov es,ax
  mov word[es:0x67],3
  mov [es:0x69],ds
  call far [es:0x67]      ; Init ROM

;  outputCharacter 'f'
  pop dx

noRom:
  add dx,0x80
  cmp dx,0xf600
  jne romScanLoop

;  outputCharacter 'B'

;  xor ax,ax
;  mov es,ax
;  mov ax,[es:0x10*4]
;  outputHex
;  mov ax,[es:0x10*4+2]
;  outputHex

;  mov ax,0
;  mov ds,ax
;  mov si,0
;  mov cx,0
;  mov dl,1
;  sendFile

;  outputCharacter 4
;  outputCharacter 0
;  outputCharacter 0
;  outputCharacter 1
;  mov ax,0
;  mov ds,ax
;  mov si,0
;  mov cx,0x8000
;sendLoopTop:
;  lodsb
;  outputCharacter
;  lodsb
;  outputCharacter
;  loop sendLoopTop


  mov ax,0x40
  mov es,ax
  mov byte[es:0x10],0x6d
  mov byte[es:0x49],0x03
  mov word[es:0x63],0x03d4
  mov word[es:0x87],0x0960
  mov word[es:0x89],0x0b11

  mov ax,0x13
  int 0x10

;  mov ax,0x40
;  mov es,ax
;  mov ax,[es:0x49]  ; AefB 5007 0B62 0D17 CDE
;  outputHex         ; AefB 2813 0960 0B11 CDE
;  mov ax,[es:0x87]
;  outputHex
;  mov ax,[es:0x89]
;  outputHex
;
;  outputCharacter 'C'


; 28.322MHz 900 dots by 262 scanlines
; 900 dots == 227.5 cycles, 360 is 91,  90 pixels is 91 hdots
; Each cycle is


; BIOS mode                  13
; Bytes per CRTC character    4
;
; Horizontal displayed       80
; Horizontal start blank     80
; Horizontal start retrace   84
; Horizontal end retrace     96
; Horizontal end blank       98
; Horizontal total          100   200
;
; Vertical displayed        400
; Vertical start blank      441
; Vertical start retrace    412
; Vertical end retrace      414
; Vertical end blank        441
; Vertical total            449   262
;
; Horizontal dots           320
;
; Colours on screen         100
;
; total horizontal cycles   800  1800




  mov dx,0x3c4
  ; Reset Register  0x01
  ;      1 Fast Reset Command                               0 = reset.  1 = no reset.
  ;      2 Safe Reset Command                               0 = reset.  2 = no reset.
  mov ax,0x0100
  out dx,ax   ; Reset sequencer

  ; Clocking Mode Register                              01
  ;      1 Character Dot Clock Select                    1  0 = 9 pixels per character.  1 = 8 pixels per character.
  ;      4 Shift Load Two Control                        0  0 = Reload the video shift registers on each character clock.  1 = Reload the video shift registers on every second character clock permitting use of slow image memory chips.
  ;      8 Dot Clock Divide by Two Enable                0  0 = Normal pixel width.  1 = Double pixel width.
  ;   0x10 Shift Load Four Control                       0  0 = Reload the video shift registers as per bit 1.  1 = Reload the video shift registers on every fourth character clock permitting use of slow image memory chips.
  ;   0x20 Screen Inhibit                                0  0 = Normal.  1 = Blank screen for faster memory access.
   mov ax,0x0101
   out dx,ax

  ; Sequencer Memory Mode Register                      0e
  ;      2 Memory Size Specification                     2  0 = 64K image memory available.  1 = More than 64K memory available.
  ;      4 Processor Image Memory Access Mode Select     4  0 = Odd/even memory access mode.  1 = Normal memory access mode.
  ;      8 Processor Image Memory Access Mapping Select  8  0 = Memory planes accessed determined by Map Mask Register.  1 = Memory plane accessed determined using low 2 bits of address.
  mov ax,0x0604
  out dx,ax

  ; Miscellaneous Output Register                       63
  ;      1 Input Output Address Select                   1  0 = Use ports 3bx.  1 = Use ports 3dx.
  ;      2 Display Memory Access Enable                  2  0 = Disable processor access to display image memory.  1 = Enable processor access to display image memory.
  ;   0x0c Dot Clock Frequency Select                    0  0 = 25.175 MHz clock for 640 total Horizontal Pixels.  1 = 28.322 MHz clock for 720 total Horizontal Pixels.  2/3 = External clock up to a maximum of 65.000 MHz.
  ;   0x20 Address Extension Bit                        20  0 = Select the low 64K page.  1 = Select the high 64K page for the Text Memory Access Mode.
  ;   0xc0 Synchronisation Signal Polarity              c0  0 = 200 lines.  1 = 400 lines.  2 = 350 lines.  3 = 480 lines.
  mov dl,0xc2
  mov al,0x67
  out dx,al
  mov dl,0xc4
  mov ax,0x0300
  out dx,ax   ; Unreset sequencer
  mov dl,0xd4  ; CRTC port address low byte             d4  Corresponds to Input Output Address Select
  mov ax,0x0e11  ; Unprotect
  out dx,ax

  ;   0xff Horizontal Total Register                    5f  Horizontal Total - 5
   mov ax,0xdc00
   out dx,ax

  ;   0xff Horizontal Display Enable End Register       4f  Horizontal Display End - 1
   mov ax,0xcc01
   out dx,ax

  ;   0xff Start Horizontal Blanking Register           50  Start Horizontal Blanking
   mov ax,0xcd02
;   mov ax,0xff02
   out dx,ax

  ; End Horizontal Blanking Register                    82
  ;   0x1f End Horizontal Blanking                       2  End Horizontal Blanking bits 4..0
  ;   0x60 Display Enable Skew                           0  character clock delay
  ;   0x80 Light Pen Register Enable                    80  0 = Enable Lightpen Registers.  1 = Normal Operation.
   mov ax,0x9f03
;   mov ax,0x8003
   out dx,ax

  ;   0xff Start Horizontal Retrace Register            54  Start Horizontal Retrace Pulse
   mov ax,0xd104
;   mov ax,0xff04
   out dx,ax

  ; End Horizontal Retrace Register                     80
  ;   0x1f End Horizontal Retrace                        0  End Horizontal Retrace  (only bottom 4 bits used?)
  ;   0x60 Horizontal Retrace Skew                       0  Horizontal Retrace Skew
  ;   0x80 End Horizontal Blanking Register bit 5       80  End Horizontal Blanking bit 5
   mov ax,0x1d05
;   mov ax,0x0005
   out dx,ax

  ;   0xff Vertical Total Register                      bf  Vertical Total - 2 bits 7..0
   mov ax,0x0406
   out dx,ax

  ; CRTC Overflow Register                              1f
  ;      1 Vertical Total bit 8                          1  Vertical Total bit 8
  ;      2 Vertical Display End bit 8                    2  Vertical Display End bit 8
  ;      4 Vertical Retrace Start bit 8                  4  Vertical Retrace Start bit 8
  ;      8 Start Vertical Blanking bit 8                 8  Start Vertical Blanking bit 8
  ;   0x10 Line Compare bit 8                           10  Line Compare bit 8
  ;   0x20 Vertical Total bit 9                          0  Vertical Total bit 9
  ;   0x40 Vertical Display End bit 9                    0  Vertical Display End bit 9
  ;   0x80 Vertical Retrace Start bit 9                  0  Vertical Retrace Start bit 9
   mov ax,0xdf07
   out dx,ax

  ; Maximum Scan Line Register                          41
  ;   0x1f Maximum Scan Lines                            1  Height of a text mode character.
  ;   0x20 Start Vertical Blanking bit 9                 0  Start Vertical Blanking bit 9
  ;   0x40 Line Compare Register bit 9                  40  Line Compare bit 9
  ;   0x80 Double Scan Line Display                      0  0 = Normal pixel height.  1 = Double pixel height.
   mov ax,0x4009
   out dx,ax

  ;   0xff Vertical Retrace Start Register              9c  Vertical Retrace Start bits 7..0
   mov ax,0xff10
   out dx,ax

  ; Vertical Retrace End Register                       8e
  ;   0x0f Vertical Retrace End                         0e  Vertical Retrace End
  ;   0x10 Clear Vertical Interrupt                      0  0 = Clear the vertical interrupt state.  1 = Normal vertical interrupt state operation.
  ;   0x20 Enable Vertical Interrupt                     0  0 = Enable the vertical interrupt.  1 = Disable the vertical interrupt.
  ;   0x40 Refresh Bandwidth Select                      0  0 = Generate 3 image memory refresh cycles per horizontal scan line.  1 = Generate 5.
  ;   0x80 CRTC Register Protect                        80  0 = Enable writes to CRTC Regs at indexes 0..7.  1 = Disable writes to CRTC Regs at indexes 0..7.
   mov ax,0x4e11
   out dx,ax

  ;   0xff Vertical Display Enable End Register         8f  Vertical Display Enable End bits 7..0
   mov ax,0x0612
   out dx,ax

  ;   0xff Offset Register                              28  Display Image Offset. Image data width.
   mov ax,0x7113
   out dx,ax

  ; Underline Location Register                         40
  ;   0x1f Underline Location                            0  Underline scan line for monochrome attribute mode.
  ;   0x20 Display Address Dwell Count 4                 0
  ;   0x40 Display Memory Address Unit 4                40
  mov ax,0x0014
  out dx,ax

  ;   0xff Start Vertical Blanking Register             96  Start Vertical Blanking bits 7..0
   mov ax,0xff15
   out dx,ax

  ;   0xff End Vertical Blanking Register               b9  End Vertical Blanking
   mov ax,0x0016
   out dx,ax

  ; CRTC Mode Control Register                          a3
  ;      1 Display Address 13 Remap Row 0                1  0 = Emulate CGA interlacing.  1 = No interlacing.
  ;      2 Display Address 14 Remap Row 1                2  0 = Emulate Hercules interlacing.  1 = No interlacing.
  ;      4 Horizontal Retrace Divisor                    0  0 = Normal operation.  1 = Double number of scan lines.
  ;      8 Display Address Dwell Count 2                 0
  ;   0x20 Display Address Unit 2 Remap                 20
  ;   0x40 Display Memory Address Unit 2                40
  ;   0x80 Horizontal/Vertical Retrace Reset            80  0 = Reset the horizontal and vertical retrace signals.  1 = Generate horizontal and vertical retrace signals.
  mov ax,0xe317
  out dx,ax


  ; rows 3 to 255 inclusive
  ; columns 59 to 826 inclusive
  ; 21-52
;  mov dx,3
;  mov ax,cs
;  mov es,ax
;  mov di,screenData
;  mov si,0
;yLoopTop:
;
;  mov ax,di
;  mov cl,4
;  shr ax,cl
;  and di,15
;  mov cx,es
;  add cx,ax
;  mov es,cx
;
;  inc dx
;  cmp dx,256
;  jne yLoopTop



  mov cx,64
  mov al,0
  mov dx,0x3c8
dacLoop:
  out dx,al
  inc dx
  out dx,al
  out dx,al
  out dx,al
  inc ax
  dec dx
  loop dacLoop

  outputCharacter 'D'

  mov ax,0xa000
  mov es,ax
  mov bx,cs
  mov ds,bx
  mov si,screenData
  mov dx,0x3c4
  xor di,di
  mov cx,(904*262 + 15)/16
  mov bp,4
blitLoop:
  mov ax,0x0102
  out dx,ax

  mov al,[si]
  stosb
  mov al,[si+4]
  stosb
  mov al,[si+8]
  stosb
  mov al,[si+12]
  stosb
  sub di,bp

  mov ax,0x0202
  out dx,ax

  mov al,[si+1]
  stosb
  mov al,[si+5]
  stosb
  mov al,[si+9]
  stosb
  mov al,[si+13]
  stosb
  sub di,bp

  mov ax,0x0402
  out dx,ax

  mov al,[si+2]
  stosb
  mov al,[si+6]
  stosb
  mov al,[si+10]
  stosb
  mov al,[si+14]
  stosb
  sub di,bp

  mov ax,0x0802
  out dx,ax

  mov al,[si+3]
  stosb
  mov al,[si+7]
  stosb
  mov al,[si+11]
  stosb
  mov al,[si+15]
  stosb

  inc bx
  mov ds,bx
  loop blitLoop

;  outputCharacter 'E'
  mov ax,cs
  mov ds,ax

  mov bx,endHorizontalDisplay
  jmp doneKey

adjustLoop:
  mov ah,0
  int 0x16
  cmp ah,0x48
  jne notUp
  sub bx,2
  cmp bx,endHorizontalDisplay
  jge doneKey
  mov bx,endHorizontalDisplay
  jmp doneKey
notUp:
  cmp ah,0x50
  jne notDown
  add bx,2
  cmp bx,endVerticalBlanking
  jle doneKey
  mov bx,endVerticalBlanking
  jmp doneKey
notDown:
  cmp ah,0x4b
  jne notLeft
  dec word[bx]
  jmp doneKey
notLeft:
  cmp ah,0x4d
  jne doneKey
  inc word[bx]
doneKey:

  mov dx,0x3d4

  mov ah,[endHorizontalDisplay]
  mov al,1
  out dx,ax

  mov ah,[startHorizontalBlanking]
  mov al,2
  out dx,ax

  mov ah,[endHorizontalBlanking]
  and ah,0x1f
  or ah,0x80
  mov al,3
  out dx,ax

  mov ah,[startHorizontalRetrace]
  mov al,4
  out dx,ax

  mov ah,[endHorizontalRetrace]
  mov al,[endHorizontalBlanking]
  shl al,1
  shl al,1
  and al,0x80
  and ah,0x1f
  or ah,al
  mov al,5
  out dx,ax

  mov ah,[endVerticalDisplay+1]
  mov al,ah
  shl ah,1
  and ah,2
  mov cl,5
  shl al,cl
  and al,0x40
  or ah,al
  mov al,[startVerticalRetrace+1]
  shl al,1
  shl al,1
  and al,4
  or ah,al
  mov al,[startVerticalBlanking+1]
  mov cl,3
  shl al,cl
  and al,8
  or ah,al
  mov al,[startVerticalRetrace+1]
  mov cl,6
  shl al,cl
  and al,0x80
  or ah,al
  or ah,0x11
  mov al,7
  out dx,ax

  mov ah,[startVerticalBlanking+1]
  mov cl,4
  shl ah,cl
  and ah,0x20
  or ah,0x40
  mov al,9
  out dx,ax

  mov ah,[startVerticalRetrace]
  mov al,0x10
  out dx,ax

  mov ah,[endVerticalRetrace]
  and ah,0x0f
  or ah,0x40
  mov al,0x11
  out dx,ax

  mov ah,[endVerticalDisplay]
  mov al,0x12
  out dx,ax

  mov ah,[startVerticalBlanking]
  mov al,0x15
  out dx,ax

  mov ah,[endVerticalBlanking]
  mov al,0x16
  out dx,ax

  mov ax,[endHorizontalDisplay]
  outputHex
  mov ax,[startHorizontalBlanking]
  outputHex
  mov ax,[startHorizontalRetrace]
  outputHex
  mov ax,[endHorizontalRetrace]
  outputHex
  mov ax,[endHorizontalBlanking]
  outputHex
  outputCharacter ' '
  mov ax,[endVerticalDisplay]
  outputHex
  mov ax,[startVerticalBlanking]
  outputHex
  mov ax,[startVerticalRetrace]
  outputHex
  mov ax,[endVerticalRetrace]
  outputHex
  mov ax,[endVerticalBlanking]
  outputHex
  outputCharacter 10

  jmp adjustLoop

;endHorizontalDisplay: dw 0xcc
;startHorizontalBlanking: dw 0xcd
;startHorizontalRetrace: dw 0xd1
;endHorizontalRetrace: dw 0xdd
;endHorizontalBlanking: dw 0xdf
;endVerticalDisplay: dw 0xd4       ; 18f 399 212
;startVerticalBlanking: dw 0xdb    ; 196 406 219
;startVerticalRetrace: dw 0xe1     ; 19c 412 225
;endVerticalRetrace: dw 0xe3       ; 19e 414 227
;endVerticalBlanking: dw 0xfe      ; 1b9 441 254

endHorizontalDisplay: dw 0xdb
startHorizontalBlanking: dw 0xd9
startHorizontalRetrace: dw 0xdb
endHorizontalRetrace: dw 0xdd
endHorizontalBlanking: dw 0xdf
endVerticalDisplay: dw 0x101      ; 18f 399 212
startVerticalBlanking: dw 0x1ff   ; 196 406 219
startVerticalRetrace: dw 0x103    ; 19c 412 225
endVerticalRetrace: dw 0x104      ; 19e 414 227
endVerticalBlanking: dw 0x105     ; 1b9 441 254


sineTable:
  db 37, 37, 37, 38, 38, 38, 39, 39, 40, 40, 40, 41, 41, 42, 42, 42
  db 43, 43, 43, 44, 44, 44, 45, 45, 45, 46, 46, 46, 47, 47, 47, 48
  db 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 50, 51, 51, 51, 51
  db 51, 51, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52
  db 53, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 51
  db 51, 51, 51, 51, 51, 50, 50, 50, 50, 50, 49, 49, 49, 49, 48, 48
  db 48, 48, 47, 47, 47, 46, 46, 46, 45, 45, 45, 44, 44, 44, 43, 43
  db 43, 42, 42, 42, 41, 41, 40, 40, 40, 39, 39, 38, 38, 38, 37, 37
  db 37, 36, 36, 35, 35, 35, 34, 34, 33, 33, 33, 32, 32, 31, 31, 31
  db 30, 30, 30, 29, 29, 29, 28, 28, 28, 27, 27, 27, 26, 26, 26, 25
  db 25, 25, 25, 24, 24, 24, 24, 23, 23, 23, 23, 23, 22, 22, 22, 22
  db 22, 22, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21
  db 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22
  db 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25
  db 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 30, 30
  db 30, 31, 31, 31, 32, 32, 33, 33, 33, 34, 34, 35, 35, 35, 36, 36

screenData:
