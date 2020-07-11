  org 0

%include "../../defaults_common.asm"

%define bin

%ifdef bin
%macro outputCharacter 0
  int 0x65
%endmacro
%else
%macro outputCharacter 0
  call doPrintCharacter
%endmacro
%endif

..start:
;  mov ax,0x40
;  mov ds,ax
;checkMotorShutoff:
;  cmp byte[0x40],0
;  je noMotorShutoff
;  mov byte[0x40],1
;  jmp checkMotorShutoff
;noMotorShutoff:

;  mov dx,0x3b8
;  mov al,29
;  out dx,al
;
;  mov dl,0xb4
;  ;   0xff Horizontal Total                             38  38  71  71  38  38  38  61
;  mov ax,0x6100
;  out dx,ax
;
;  ;   0xff Horizontal Displayed                         28  28  50  50  28  28  28  50
;  mov ax,0x5001
;  out dx,ax
;
;  ;   0xff Horizontal Sync Position                     2d  2d  5a  5a  2d  2d  2d  52
;  mov ax,0x5202
;  out dx,ax
;
;  ;   0x0f Horizontal Sync Width                        0a  0a  0a  0a  0a  0a  0a  0f
;  mov ax,0x0f03
;  out dx,ax
;
;  ;   0x7f Vertical Total                               1f  1f  1f  1f  7f  7f  7f  19
;  mov ax,0x1904
;  out dx,ax
;
;  ;   0x1f Vertical Total Adjust                        06  06  06  06  06  06  06  06
;  mov ax,0x0605
;  out dx,ax
;
;  ;   0x7f Vertical Displayed                           19  19  19  19  64  64  64  19
;  mov ax,0x1906
;  out dx,ax
;
;  ;   0x7f Vertical Sync Position                       1c  1c  1c  1c  70  70  70  19
;  mov ax,0x1907
;  out dx,ax
;
;  ;   0x03 Interlace Mode                               02  02  02  02  02  02  02  02
;  mov ax,0x0208
;  out dx,ax
;
;  ;   0x1f Max Scan Line Address                        07  07  07  07  01  01  01  0d
;  mov ax,0x0d09
;  out dx,ax
;
;  ; Cursor Start                                        06  06  06  06  06  06  06  0b
;  ;   0x1f Cursor Start                                  6   6   6   6   6   6   6  0b
;  ;   0x60 Cursor Mode                                   0   0   0   0   0   0   0   0
;  mov ax,0x0b0a
;  out dx,ax
;
;  ;   0x1f Cursor End                                   07  07  07  07  07  07  07  0c
;  mov ax,0x0c0b
;  out dx,ax
;
;  ;   0x3f Start Address (H)                            00  00  00  00  00  00  00  00
;  mov ax,0x000c
;  out dx,ax
;
;  ;   0xff Start Address (L)                            00  00  00  00  00  00  00  00
;  mov ax,0x000d
;  out dx,ax
;
;  ;   0x3f Cursor (H)                                   00  00  00  00  00  00  00  00
;  mov ax,0x000e
;  out dx,ax
;
;  ;   0xff Cursor (L)                                   00  00  00  00  00  00  00  00
;  mov ax,0x000f
;  out dx,ax
;
; ; mov ax,0xb000
;;  mov es,ax
;;  xor di,di
;;  mov cx,80*25
;;  rep stosw
;
;
;  in al,0x61
;  or al,0x80
;  mov [cs:port61high+1],al
;  and al,0x7f
;  mov [cs:port61low+1],al
;
;  xor ax,ax
;  mov ds,ax
;  mov ax,[0x20]
;  mov [cs:oldInterrupt8],ax
;  mov ax,[0x22]
;  mov [cs:oldInterrupt8+2],ax
;
;  in al,0x21
;  mov [cs:imr],al
;  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
;  out 0x21,al
;
  ; Determine phase
  lockstep 1
  mov ax,cs
  mov es,ax
  mov ds,ax
  mov ss,ax
  mov sp,stackTop
  mov di,data2

  in al,0x61
  or al,3
  out 0x61,al

  mov al,TIMER2 | BOTH | MODE2 | BINARY
  out 0x43,al
  mov dx,0x42
  mov al,0
  out dx,al
  out dx,al

  %rep 5
    readPIT16 2
    stosw
  %endrep

  refreshOn

  mov ax,'0'
  mov di,[data2+8]
  mov si,[data2+6]
  mov bx,[data2+4]
  mov cx,[data2+2]
  mov dx,[data2]
  sub dx,cx
  sub dx,20
  jnz notPhase0
  add ax,1
notPhase0:
  sub cx,bx
  sub cx,20
  jnz notPhase1
  add ax,2
notPhase1:
  sub bx,si
  sub bx,20
  jnz notPhase2
  add ax,4
notPhase2:
  sub si,di
  sub si,20
  jnz notPhase3
  add ax,8
notPhase3:
  mov [phase],al


  mov di,startAddresses
  mov ax,cs
  mov es,ax
  mov ax,-1
  mov cx,102
initAddressesLoopTop:
  stosw
  add ax,40
  loop initAddressesLoopTop
  mov cx,101
initAddressesLoopTop2:
  sub ax,40
  stosw
  loop initAddressesLoopTop2
  mov cx,101
  inc ax
initAddressesLoopTop3:
  stosw
  add ax,40
  loop initAddressesLoopTop3
  mov cx,101
initAddressesLoopTop4:
  sub ax,40
  stosw
  loop initAddressesLoopTop4


;  mov di,rasterData
;  xor ax,ax
;  mov cx,405
;initRastersLoopTop:
;;  stosb
;;  inc ax
;  loop initRastersLoopTop
;
  call copyImageData



; ISAV code starts here.

startISAV:
  push ds

  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x0a ;0x09
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0
  out dx,al

  mov dx,0x3d4

  ;   0xff Horizontal Total                             38
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         28
  mov ax,0x2801
  out dx,ax

  ;   0xff Horizontal Sync Position                     2d
  mov ax,0x2d02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0a
  mov ax,0x0a03
  out dx,ax

  ;   0x7f Vertical Total                               01  vertical total = 2 rows
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00  vertical total adjust = 0
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           01  vertical displayed = 1
  mov ax,0x0106
  out dx,ax

  ;   0x7f Vertical Sync Position                       1c  vertical sync position = 28 rows
  mov ax,0x1707
  out dx,ax

  ;   0x03 Interlace Mode                               00   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0008
  out dx,ax

  ;   0x1f Max Scan Line Address                        00  scanlines per row = 1
  mov ax,0x0009
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   08
  mov ax,0x080b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

  mov dl,0xda
  cli

  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate

  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov al,76*2 + 1
  out 0x40,al
  mov al,0
  out 0x40,al

  xor ax,ax
  mov ds,ax
  mov ax,[0x20]
  mov [cs:originalInterrupt8],ax
  mov ax,[0x22]
  mov [cs:originalInterrupt8+2],ax
  mov word[0x20],int8_oe0
  mov [0x22],cs

  in al,0x21
  mov [cs:originalIMR],al
  mov al,0xfe
  out 0x21,al

  sti
setupLoop:
  hlt
  jmp setupLoop


  ; Step 0 - don't do anything (we've just completed wait for CRTC stabilization)
int8_oe0:
  mov word[0x20],int8_oe1

  mov al,0x20
  out 0x20,al
  iret


  ; Step 1, wait until display is disabled, then change interrupts
int8_oe1:
  in al,dx
  test al,1
  jz .noInterruptChange   ; jump if not -DISPEN, finish if -DISPEN

  mov word[0x20],int8_oe2

.noInterruptChange:

  mov al,0x20
  out 0x20,al
  iret


  ; Step 2, wait until display is enabled - then we'll be at the start of the active area
int8_oe2:
  in al,dx
  test al,1
  jnz .noInterruptChange  ; jump if -DISPEN, finish if +DISPEN

  mov word[0x20],int8_oe3
  mov cx,2

.noInterruptChange:

  mov al,0x20
  out 0x20,al
  iret


  ; Step 3 - this interrupt occurs one timer cycle into the active area.
  ; The pattern of scanlines on the screen is +-+-- As the interrupt runs every other scanline, the pattern of scanlines in terms of what is seen from the interrupt is ++---.
int8_oe3:
  mov dl,0xd4
  mov ax,0x0308  ; Set interlace mode to ISAV
  out dx,ax
  mov dl,0xda

  loop .noInterruptChange
  mov word[0x20],int8_oe4
.noInterruptChange:

  mov al,76*2
  out 0x40,al
  mov al,0
  out 0x40,al

  mov al,0x20
  out 0x20,al
  iret


  ; Step 4 - this interrupt occurs two timer cycles into the active area.
int8_oe4:
  in al,dx
  test al,1
  jnz .noInterruptChange  ; jump if -DISPEN, finish if +DISPEN

  mov word[0x20],int8_oe5

.noInterruptChange:

  mov al,0x20
  out 0x20,al
  iret


  ; Step 5
int8_oe5:
  in al,dx
  test al,1
  jz .noInterruptChange   ; jump if not -DISPEN, finish if -DISPEN (i.e. scanline 4)

  mov word[0x20],int8_oe6

  mov al,76*2 - 3
  out 0x40,al
  mov al,0
  out 0x40,al

.noInterruptChange:

  mov al,0x20
  out 0x20,al
  iret


  ; Step 6. This occurs on scanline 1. The next interrupt will be on scanline 3.
int8_oe6:
  mov word[0x20],int8_oe7

  mov al,76*2
  out 0x40,al
  mov al,0
  out 0x40,al

  mov al,0x20
  out 0x20,al
  iret


  ; Step 7. This occurs on scanline 3 (one timer cycle before the active area starts). The next interrupt will be on scanline 0.
int8_oe7:
  mov word[0x20],int8_oe8

  mov al,0x20
  out 0x20,al
  iret


  ; Step 8 - scanline 0, next interrupt on scanline 2
int8_oe8:
  mov al,(20*76) & 0xff
  out 0x40,al
  mov al,(20*76) >> 8
  out 0x40,al

  mov word[0x20],int8_oe9

  mov dl,0xd4

  mov al,0x20
  out 0x20,al
  add sp,6
  sti
  pop ds
  ret


  ; Step 9 - initial short (odd) field
int8_oe9:
  push ax
  push dx
  mov dx,0x3d4
  mov ax,0x0309    ; Scanlines per row = 4  (2 on each field)
  out dx,ax
  mov ax,0x0106    ; Vertical displayed = 1 row (actually 2)
  out dx,ax
  mov ax,0x0304    ; Vertical total = 4 rows
  out dx,ax
  mov ax,0x0305    ; Vertical total adjust = 3
  out dx,ax

  pop dx

  mov al,(224*76 - 20) & 0xff
  out 0x40,al
  mov al,(224*76 - 20) >> 8
  out 0x40,al

;  mov al,[cs:originalIMR]
;  out 0x21,al

  push ds
  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_oe10
  pop ds

  mov al,0x20
  out 0x20,al
  pop ax
  iret


  ; Step 10 - set up CRTC registers for full screen - scanline 0
int8_oe10:
  push ax
  push dx
  mov dx,0x3d4
  mov ax,0x0709  ; Scanlines per row = 8 (4 on each field)
  out dx,ax
  mov ax,0x1906  ; Vertical displayed = 25 rows (actually 50)
  out dx,ax
  mov ax,0x1f04  ; Vertical total = 32 rows (actually 64)
  out dx,ax
  mov ax,0x0605  ; Vertical total adjust = 6
  out dx,ax

  mov al,(76) & 0xff
  out 0x40,al
  mov al,(76) >> 8
  out 0x40,al

  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_isav1

  mov al,0x20
  out 0x20,al
  sti
  hlt


;  ; Final 0 - scanline 224
;int8_isav0:
;  push ax
;  push dx
;  mov dx,0x3d4
;
;  mov al,0xfe
;  out 0x21,al
;
;  mov al,(76) & 0xff
;  out 0x40,al
;  mov al,(76) >> 8
;  out 0x40,al
;
;  push ds
;  xor ax,ax
;  mov ds,ax
;  mov word[0x20],int8_isav1
;
;  mov al,0x20
;  out 0x20,al
;  sti
;  hlt


  ; Final 1 - scanline 225
int8_isav1:
isav1_patch:
  mov ax,0x1102  ; Horizontal sync position early
  out dx,ax

  mov word[0x20],int8_isav2

  mov al,0x20
  out 0x20,al
  sti
  hlt


  ; Final 2 - scanline 226
int8_isav2:
isav2_patch:
  mov ax,0x2d02  ; Horizontal sync position normal
  out dx,ax

  mov word[0x20],int8_isav3

  mov al,0x20
  out 0x20,al
  sti
  hlt


  ; Final 3 - scanline 227
int8_isav3:
  mov word[0x20],int8_isav4

  mov al,0x20
  out 0x20,al
  sti
  hlt


  ; Final 4 - scanline 228
int8_isav4:
isav4_patch:
  mov ax,0x1102  ; Horizontal sync position early
  out dx,ax

  mov al,(33*76) & 0xff
  out 0x40,al
  mov al,(33*76) >> 8
  out 0x40,al

  mov word[0x20],int8_isav5

  mov al,0x20
  out 0x20,al
  sti
  hlt


  ; Final 5 - scanline 229
int8_isav5:
isav5_patch:
  mov ax,0x2d02  ; Horizontal sync position normal
  out dx,ax

  mov al,(264*76) & 0xff
  out 0x40,al
  mov al,(264*76) >> 8
  out 0x40,al

  mov word[0x20],int8_field1

  mov al,0x20
  out 0x20,al
  sti
  hlt


  ; Field 1: 2 rows per frame, 1 scanline per row
int8_field1:
  mov ax,cs
  mov ds,ax
  mov ss,ax

  mov sp,startAddresses+203*2
  mov dx,0x3d4
;  mov ax,0x0009 ; 1 scanline per row
;  out dx,ax
  mov bp,0x2801 ;0x5001 (left horizontal displayed = 40)
  mov di,0x0c00 ;0x1900 (right horizontal total = 13)
  mov ax,0x2b02 ;0x5702 (left horizontal sync position = 43)
  mov si,sampleData+203
  mov bx,rasterData-sampleData
  mov es,ax

  ; Scanlines -1..200

%macro scanline 2
  mov al,0x00
  out dx,ax        ; e  Horizontal Total         left  0x2b00  44    0x5700  88

  mov ax,0x0102 ;0x0202
  out dx,ax        ; f  Horizontal Sync Position right 0x0102   1    0x0202   2

  pop cx
  mov al,0x0c
  mov ah,ch
  out dx,ax
  inc ax
  mov ah,cl
  out dx,ax

  lodsb
  out 0xe0,al

  %if %1 == -1
    mov ax,(%2 << 8) + 0x04
    out dx,ax      ;    Vertical Total
    times 3 nop
  %elif %1 == 201-%2
;    %if %2 == 1
      mov ax,0x3d04  ;  Vertical Total                 0x3d04  62  (1 for scanlines -1 and 200, 60 for scanlines 201-260)
;    %else
;      mov ax,0x1d04  ;  Vertical Total                 0x1d04  64  (1 for scanlines -1 and 200, 62 for scanlines 201-260)
;    %endif
    out dx,ax
    times 3 nop
  %else
    mov al,[bx+si]
    mov dl,0xd9
    out dx,al
    mov dl,0xd4
  %endif

  mov ax,0x0101
  out dx,ax        ; b  Horizontal Displayed     right 0x0101   1     0x0101   1
  xchg ax,di
  out dx,ax        ; a  Horizontal Total         right 0x0c00  13     0x1900  26
  xchg ax,di
  xchg ax,bp
  out dx,ax        ; d  Horizontal Displayed     left  0x2801  40     0x5001  80
  xchg ax,bp
  mov ax,es
  out dx,ax        ; c  Horizontal Sync Position left  0x2b02  43     0x5702  87
%endmacro
%assign i -1
%rep 202
  scanline i,1
  %assign i i+1
%endrep

  ; Scanline 201

  mov ax,0x3800 ;0x7100
  out dx,ax        ; e  Horizontal Total         left  0x3800  57     0x7100 114
  mov ax,0x2d02 ;0x5a02
  out dx,ax        ; f  Horizontal Sync Position right 0x2d02  45     0x5a02  90
 ; mov ax,0x3d04  ; Vertical total
;  out dx,ax
;  mov ax,0x1707  ; Vertical sync position
;  out dx,ax

  mov sp,stackTop

  ; TODO: We are now free to do per-frame vertical-overscan stuff
  ; with no special timing requirements except:
  ;   HLT before overscan is over
  ;   Sound (if in use)

;  mov bp,cs
;  mov es,bp
;  mov di,startAddresses

  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_field0
  mov ax,cs
  mov ds,ax

;  inc word[frameCount]
;  jnz noFrameCountCarry
;  inc word[frameCount+2]
;noFrameCountCarry:

 mov ax,[cs:adjustPeriod]
 mov bx,[cs:lastAdjustPeriod]
 mov [cs:lastAdjustPeriod],ax
 sub ax,bx
 add ax,224*76
 mov [cs:patchDriftLow+1],al
 mov [cs:patchDriftHigh+1],ah

patchDriftLow:
  mov al,(224*76) & 0xff
  out 0x40,al
patchDriftHigh:
  mov al,(224*76) >> 8
  out 0x40,al

  mov dx,0x3d9
  mov al,2
  out dx,al

  call doKeyboard
  mov al,0x20
  out 0x20,al
  sti
  hlt


int8_field0:
  mov ax,cs
  mov ds,ax
  mov ss,ax

  mov sp,startAddresses
  mov dx,0x3d4
  mov ax,0x0109  ; 2 scanlines per row
  out dx,ax
  mov bp,0x2801 ;0x5001 (left horizontal displayed = 40)
  mov di,0x0c00 ;0x1900 (right horizontal total = 13)
  mov ax,0x2b02 ;0x5702 (left horizontal sync position = 43)
  mov si,sampleData
  mov bx,rasterData-sampleData
  mov es,ax

  ; Scanlines -1..201

%assign i -1
%rep 203
  scanline i,0
  %assign i i+1
%endrep

  ; Scanline 202

  mov ax,0x3800 ;0x7100
  out dx,ax        ; e  Horizontal Total         left  0x3800  57     0x7100 114
  mov ax,0x2d02 ;0x5a02
  out dx,ax        ; f  Horizontal Sync Position right 0x2d02  45     0x5a02  90
  mov ax,0x0009  ; 1 scanline per row
  out dx,ax
;  mov ax,0x3d04  ; Vertical total
;  out dx,ax
;  mov ax,0x1707  ; Vertical sync position
;  out dx,ax

  mov sp,stackTop

  ; TODO: We are now free to do per-frame vertical-overscan stuff
  ; with no special timing requirements except:
  ;   HLT before overscan is over
  ;   Sound (if in use)

  mov bp,cs
  mov es,bp
  mov di,startAddresses

  xor ax,ax
  mov ds,ax
  mov word[0x20],int8_isav1

;  inc word[frameCount]
;  jnz noFrameCountCarry
;  inc word[frameCount+2]
;noFrameCountCarry:

  mov al,(76) & 0xff
  out 0x40,al
  mov al,(76) >> 8
  out 0x40,al

  mov dx,0x3d9
  mov al,1
  out dx,al


;  call doKeyboard

  xor ax,ax
  mov ds,ax
  mov dx,0x3d4

  mov al,0x20
  out 0x20,al
  sti
  hlt


;  jmp doneFrame

;restart:


;  call startISAV


;  lockstep 1
;;  safeRefreshOff
;
;  ; Mode
;  ;      1 +HRES
;  ;      2 +GRPH
;  ;      4 +BW
;  ;      8 +VIDEO ENABLE
;  ;   0x10 +1BPP
;  ;   0x20 +ENABLE BLINK
;  mov dx,0x3d8
;  mov al,0x1a ; 0x1b
;  out dx,al
;
;  ; Palette
;  ;      1 +OVERSCAN B
;  ;      2 +OVERSCAN G
;  ;      4 +OVERSCAN R
;  ;      8 +OVERSCAN I
;  ;   0x10 +BACKGROUND I
;  ;   0x20 +COLOR SEL
;  inc dx
;  mov al,0x0f
;  out dx,al
;
;  mov dl,0xd4
;  mov ax,0x3800  ; Horizontal total          0x7100
;  out dx,ax
;  mov ax,0x2801  ; Horizontal displayed      0x5001
;  out dx,ax
;  mov ax,0x2d02  ; Horizontal sync position  0x5a02
;  out dx,ax
;  mov ax,0x0a03  ; Horizontal sync width     0x0f03
;  out dx,ax
;  mov ax,0x1f04  ; Vertical total
;  out dx,ax
;  mov ax,0x0005  ; Vertical total adjust
;  out dx,ax
;  mov ax,0x0106  ; Vertical displayed
;  out dx,ax
;  mov ax,0x0c07  ; Vertical sync position
;  out dx,ax
;  mov ax,0x0008  ; Interlace mode
;  out dx,ax
;  mov ax,0x0109  ; Maximum scanline address
;  out dx,ax
;  mov ax,0x060a
;  out dx,ax
;  mov ax,0x070b
;  out dx,ax
;  mov ax,0x000c
;  out dx,ax
;  inc ax
;  out dx,ax
;  mov ax,0x3f0e
;  out dx,ax
;  mov ax,0xff0f
;  out dx,ax
;  mov dl,0xda
;  waitForNoVerticalSync
;  waitForVerticalSync
;;  waitForDisplayEnable
;  mov ax,0x0104
;  mov dl,0xd4
;  out dx,ax
;
;  writePIT16 0, 2, 2   ; Ensure IRQ0 pending
;
;  xor ax,ax
;  mov ds,ax
;  mov word[0x20],interrupt8h0
;  mov [0x22],cs
;
;;   mov dl,0xd9
;;   mov al,0x0e
;;   out dx,al
;
;  mov dl,0xda
;  waitForDisplayDisable
;  waitForDisplayEnable
;
;;   mov dl,0xd9
;;   mov al,0x0d
;;   out dx,al
;
;  cmp byte[cs:cgaCrtcPhase],1
;  jne noSwitchPhase
;  mov dl,0xd4
;  mov ax,0x3900 ;0x7200
;  out dx,ax
;  mov dl,0xda
;  waitForDisplayDisable
;  waitForDisplayEnable
;  mov dl,0xd4
;  mov ax,0x3800 ;0x7100
;  out dx,ax
;  mov dl,0xda
;  waitForDisplayDisable
;  waitForDisplayEnable
;noSwitchPhase:
;
;;   mov dl,0xd9
;;   mov al,0x0c
;;   out dx,al
;
;  waitForDisplayDisable
;  waitForDisplayEnable
;
;;   mov dl,0xd9
;;   mov al,1
;;   out dx,al
;
;
;  writePIT16 0, 2, 31
;
;  sti
;  hlt
;interrupt8h0:
;;   mov al,2
;;   out dx,al
;
;  mov al,75                 ; Now counting down from 31
;  out 0x40,al
;  mov al,0
;  out 0x40,al
;  mov word[0x20],interrupt8h1
;  mov al,0x20
;  out 0x20,al
;  sti
;  hlt
;
;interrupt8h1:
;;   mov al,3
;;   out dx,al
;;   inc dx
;
;  in al,dx                  ; Now counting down from 75
;  test al,1
;  jz .noInterruptChange  ; jump if +DISPEN, finish if -DISPEN
;  mov word[0x20],interrupt8h2
;.noInterruptChange:
;  mov al,0x20
;  out 0x20,al
;  mov sp,stackTop
;  sti
;  hlt
;
;interrupt8h2:
;;   dec dx
;;   mov al,4
;;   out dx,al
;
;  mov ax,[cs:refreshPhase]     ; We're still counting down from 75
;  out 0x40,al
;  mov al,ah
;  out 0x40,al
;  mov word[0x20],interrupt8h3
;  mov al,0x20
;  out 0x20,al
;  mov sp,stackTop
;  sti
;  hlt
;
;interrupt8h3:
;;   mov al,5
;;   out dx,al
;
;  mov word[0x20],interrupt8h4  ; We're still counting down from refreshPhase
;  mov al,0x20
;  out 0x20,al
;  mov sp,stackTop
;  sti
;  hlt
;
;interrupt8h4:
;;   mov al,6
;;   out dx,al
;
;  refreshOn 19                 ; refreshPhase has happened, restart refresh
;  mov al,0x20
;  out 0x20,al
;  mov sp,stackTop
;
;  mov dl,0xd4
;  mov ax,0x3f04
;  out dx,ax
;
;  mov dl,0xda
;  waitForNoVerticalSync
;  waitForVerticalSync
;
;  waitForDisplayEnable
;
;  writePIT16 0, 2, 76*64 - 1  ; Start counting down after display enable starts
;
;  mov word[0x20],interrupt8a
;
;  sti
;  hlt
;interrupt8a:
;;   dec dx
;;   mov al,7
;;   out dx,al
;;   inc dx
;
;  in al,dx
;  test al,1
;  jz .noInterruptChange  ; jump if +DISPEN, finish if -DISPEN
;  mov word[0x20],interrupt8b
;.noInterruptChange:
;  mov al,0x20
;  out 0x20,al
;  mov sp,stackTop
;  sti
;  hlt
;
;interrupt8b:
;;   dec dx
;;   mov al,8
;;   out dx,al
;
;  mov ax,[cs:adjustPeriod]     ; We're still counting down from 76*64 - 1
;  out 0x40,al
;  mov al,ah
;  out 0x40,al
;  mov word[0x20],interrupt8c
;  mov al,0x20
;  out 0x20,al
;  mov sp,stackTop
;  sti
;  hlt
;
;interrupt8c:
;;   mov al,9
;;   out dx,al
;
;  mov ax,(76*262) & 0xff        ; We're still counting down from adjustPeriod
;  out 0x40,al
;  mov al,(76*262) >> 8
;  out 0x40,al
;  cmp byte[cs:stableImage],0
;  je .notStableImage
;  mov word[0x20],interrupt8stable
;  jmp .doneImageSelect
;.notStableImage:
;  mov word[0x20],interrupt8
;.doneImageSelect:
;  mov al,0x20
;  out 0x20,al
;  mov sp,stackTop
;  sti
;  hlt
;
;interrupt8:

;interrupt8stable:
;  initCGA 0x0a
;  mov dl,0xd9
;  %rep 3800
;    out dx,al
;    inc ax
;  %endrep
;interrupt8numbers:
;  mov ax,cs
;  mov ds,ax
;  jmp endOfFrame


doKeyboard:

  in al,0x60
  xchg ax,bx
  ; Acknowledge the previous byte
port61high:
  mov al,0xcf
  out 0x61,al
port61low:
  mov al,0x4f
  out 0x61,al
  cmp bl,0x4b               ; left
  je moveLeft
  cmp bl,0x4d               ; right
  je moveRight
  cmp bl,0x48               ; up
  je moveUp
  cmp bl,0x50               ; down
  je moveDown
  cmp bl,0x4a               ; keypad-
  je decreaseRefreshPhase
  cmp bl,0x2c               ; z
  je decreaseRefreshPhase
  cmp bl,0x4e               ; keypad+
  je increaseRefreshPhase
  cmp bl,0x2d               ; x
  je increaseRefreshPhase
;  cmp bl,0x39               ; space
;  je switchCgaCrtcPhase
;  cmp bl,0x1f               ; s
;  je switchImage
;  cmp bl,0x31               ; n
;  je toggleNumbersScreen
  cmp bl,1                  ; esc
  je tearDown2
  ret

tearDown2:
  jmp tearDown
moveLeft:
  dec word[adjustPeriod]
  jmp doneFrame
moveRight:
  inc word[adjustPeriod]
  jmp doneFrame
moveUp:
  sub word[adjustPeriod],76
  jmp doneFrame
moveDown:
  add word[adjustPeriod],76
  jmp doneFrame
decreaseRefreshPhase:
  dec word[refreshPhase]
  cmp word[refreshPhase],64-1
  jne .done
  mov word[refreshPhase],64+18
.done:
  jmp doneFrame
increaseRefreshPhase:
  inc word[refreshPhase]
  cmp word[refreshPhase],64+19
  jne .done
  mov word[refreshPhase],64+0
.done:
  jmp doneFrame
;switchCgaCrtcPhase:
;  xor byte[cgaCrtcPhase],1
;  jmp doneFrame
;switchImage:
;  xor byte[stableImage],1
;  cmp byte[numbersMode],0
;  jne .noCopyImageData
;  call copyImageData
;.noCopyImageData:
;  jmp doneFrame
;toggleNumbersScreen:
;  xor byte[numbersMode],1
;  cmp byte[numbersMode],0
;  je leavingNumbersMode
;
;  initCGA 9
;  call copyImageData
;
;  jmp doneFrame
;leavingNumbersMode:
;  call copyImageData
doneFrame:

;  mov ax,0xb000
;  call printNumbers

%ifdef bin
;  mov al,0xfc  ; Enable IRQ0 (timer), disable all others
;  out 0x21,al
  mov ax,[cs:phase]
  outputHex
  mov al,13
  outputCharacter
  mov al,10
  outputCharacter
  mov ax,[cs:adjustPeriod]
  outputHex
  mov al,13
  outputCharacter
  mov al,10
  outputCharacter
  mov ax,[cs:refreshPhase]
  outputHex
  mov al,13
  outputCharacter
  mov al,10
  outputCharacter
;  mov ax,[cs:cgaCrtcPhase]
;  outputHex
;  mov al,13
;  outputCharacter
;  mov al,10
;  outputCharacter

;  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
;  out 0x21,al
%endif


;  cmp byte[numbersMode],0
;  jne doNumbersMode
  ; Not numbers mode, update numbers on MDA only

;  jmp restart
;doNumbersMode:

;  mov ax,0xb800
;  call printNumbers
;  xor ax,ax
;  mov ds,ax
;  mov word[0x20],interrupt8numbers
  ret


tearDown:
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,18
  out 0x41,al  ; Timer 1 rate

  xor ax,ax
  mov ds,ax
  mov ax,[cs:originalInterrupt8]
  mov [0x20],ax
  mov ax,[cs:originalInterrupt8+2]
  mov [0x22],ax

  in al,0x61
  and al,0xfc
  out 0x61,al

  mov ax,cs
  mov ds,ax
  mov al,[originalIMR]
  out 0x21,al

  writePIT16 0, 2, 0

  mov ax,3
  int 0x10

  sti
  mov ax,cs
  mov ds,ax
  mov al,[phase]
  outputCharacter

;  mov ax,19912
;  mul word[frameCount]
;  mov cx,dx
;  mov ax,19912
;  mul word[frameCount+2]
;  add ax,cx
;  adc dx,0
;  mov cx,0x40
;  mov ds,cx
;  add [0x6c],ax
;  adc [0x6e],dx
;dateLoop:
;  cmp word[0x6c],0x18
;  jb doneDateLoop
;  cmp word[0x6e],0xb0
;  jb doneDateLoop
;  mov byte[0x70],1
;  sub word[0x6c],0xb0
;  sbb word[0x6e],0x18
;  jmp dateLoop
;doneDateLoop:
exit:
  mov ax,0x4c00
  int 0x21


copyImageData:
  mov ax,0xb800
  mov es,ax
  mov ax,cs
  mov ds,ax
  xor di,di
  cld

;  cmp byte[stableImage],0
;  jne clearVRAM
;  cmp byte[numbersMode],0
;  jne clearVRAM

  mov si,vramData
  mov cx,4096+4080
  rep movsw
  ret

;clearVRAM:
;  xor ax,ax
;  mov cx,8192
;  rep stosw
;  ret

;printNybble:
;  and al,0xf
;  cmp al,10
;  jge .letters
;  add al,'0'
;  jmp printCharacter
;.letters:
;  add al,'A'-10
;
;printCharacter:
;  mov ah,7
;  stosw
;  ret
;
;printHex:
;  push ax
;  mov al,ah
;  mov cl,4
;  shr al,cl
;  call printNybble
;  pop ax
;  push ax
;  mov al,ah
;  call printNybble
;  pop ax
;  push ax
;  mov cl,4
;  shr al,cl
;  call printNybble
;  pop ax
;  call printNybble
;  ret
;
;printNumbers:
;  mov es,ax
;  xor di,di
;  mov ax,[phase]
;  call printNybble
;  mov di,160
;  mov ax,[adjustPeriod]
;  call printHex
;  mov di,320
;  mov ax,[refreshPhase]
;  call printHex
;  mov di,480
;  mov ax,[cgaCrtcPhase]
;  call printNybble
;  ret
;
;dummyInterrupt8:
;  push ax
;  mov al,0x20
;  out 0x20,al
;  pop ax
;  iret
;
;doPrintCharacter:
;  push ax
;  push bx
;  push cx
;  push dx
;  push si
;  push di
;  push bp
;  mov dl,al
;  mov ah,2
;  int 0x21
;  pop bp
;  pop di
;  pop si
;  pop dx
;  pop cx
;  pop bx
;  pop ax
;  ret




; Returns the CGA to normal mode
stopISAV:
  cli
  xor ax,ax
  mov ds,ax
  mov ax,[cs:originalInterrupt8]
  mov [8*4],ax
  mov ax,[cs:originalInterrupt8+2]
  mov [8*4+2],ax
  mov al,0x34
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al
  sti

  ; Set the CGA back to a normal mode so we don't risk breaking anything
  mov ax,3
  int 0x10
  ret




;frameCount: dw 0, 0
;oldInterrupt8: dw 0, 0
;imr: db 0
phase: dw 0
lastAdjustPeriod: dw 0x142a
adjustPeriod: dw 0x142a
refreshPhase: dw 0x0045
;cgaCrtcPhase: dw 0
;numbersMode: dw 0
;stableImage: dw 0
originalInterrupt8:
  dw 0, 0
originalIMR:
  db 0
timerCount:
  dw 0

startAddresses:
  times 405 dw 0
rasterData:
  times 405 db 0x0f
sampleData:
  times 405 db 38
data2:
  times 5 dw 0
vramData:
  incbin "..\..\..\..\Pictures\reenigne\cga2ntsc\mandala-4241838_203_out.dat"

;segment stack stack
  times 128 dw 0
stackTop:


;0 is default
;
; 0- 1   broken sync
; 2- 4   ok (even lines = bank 0)
; 5-11   broken sync
;12-14   broken stability
;15-17   hardly anything, unstable
;18-20   broken sync
;21-25   hardly anything, unstable
;26-33   broken sync
;34-36   ok (even lines = bank 0)
;37-39   advance 1 character per scanline
;40-42   ok (even lines = bank 0)
;43-52   broken sync
;53-55   hardly anything, unstable
;56-56   broken sync
;57-61   hardly anything, unstable
;62-68   broken sync
;69-71   1 column, broken sync
;72-72   ok (even lines = bank 0)
;73-74   hardly anything, unstable
;75-


;142c   odd lines



;Field 0: (102 VRAM lines used, 203 scanlines)
;  0
;  1
;  ...
;  100
;  101
;  100
;  ...
;  1
;  0
;
;Field 1: (101 VRAM lines used, 202 scanlines)
;  0.5
;  1.5
;  ...
;  100.5
;  100.5
;  ...
;  1.5
;  0.5
;
;Total 405 scanlines



;    Frame structure
;      hsync left on vsync start
;      hsync right on vsync start
;      hsync left on vsync end
;      hsync right on vsync end
;      field 1
;      field 0

