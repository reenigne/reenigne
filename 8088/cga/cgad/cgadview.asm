org 0x100
cpu 8086

  cli
  mov ax,cs
  mov ds,ax
  mov ss,ax
  mov sp,stackHigh-2
  sti

  mov si,0x81
searchLoop
  lodsb
  cmp al,0x20
  je foundEnd
  cmp al,0x0d
  jne searchLoop
foundEnd:
  dec si
  mov byte[si],0
  cmp si,0x81
  je error

  ; Load meta file

  mov ax,0x3d00
  mov dx,0x81
  int 0x21               ; open file
  jc error
  mov bx,ax
  mov ax,0x4202
  xor cx,cx
  xor dx,dx
  int 0x21               ; seek to end
  jc error
  cmp dx,0
  jne error
  mov [cgadSize],ax
  mov ax,0x4200          ; seek to start
  int 0x21
  jc error
  mov cx,[cgadSize]
  mov ah,0x3f
  mov dx,cgadStart
  int 0x21               ; read file
  jc error
  mov ah,0x3e
  int 0x21               ; close file
  jnc success
error:
  mov ah,9
  mov dx,errorMessage
  int 0x21
  ret

success:
  cmp word[cgadStart],0x4743    ; 'CG'
  jne error
  cmp word[cgadStart+2],0x4441  ; 'AD'
  jne error
  cmp word[cgadStart+4],0       ; version low
  jne error
  cmp word[cgadStart+6],0       ; version high
  jne error

  ; Assume we have a single time record containing the full 16kB of CGA VRAM
  ; and all registers. The CGAD files currently produced by CGAArt comply with
  ; this.

  mov ax,6
  int 0x10

  mov dx,0x3d8
  mov al,[cgadStart+40]
  out dx,al
  inc dx
  mov al,[cgadStart+41]
  out dx,al
  lea si,cgadStart+42
  mov dl,0xd4
  mov cx,16
  mov bl,0
crtcLoop:
  lodsb
  mov ah,al
  mov al,bl
  inc bx
  out dx,ax
  loop crtcLoop

  mov ax,0xb800
  mov es,ax
  xor di,di
  mov cx,0x2000
  rep movsw

  mov ah,0x00
  int 0x16

  mov ax,3
  int 0x10

  mov ax,0x4c00
  int 0x21

cgadSize: dw 0

errorMessage: db "File error$"

stackLow:
  times 128 dw 0
stackHigh:

cgadStart:
; DWord  format identifier ('CGAD')                                         0
; DWord  format version (0)                                                 4
; DWord  total number of hdots (usually 262*912)                            8
; DWord  horizontal PLL hdots (usually 910)                                12
; DWord  vertical PLL hdots (usually 262.5*910)                            16
; Time records follow. Each time record:
;   DWord  hdots from start of file                                        20
;   DWord  first address to modify                                         24
;   DWord  count of addresses to modify                                    28
;   Data bytes follow, padded with 0s to a multiple of 4
;
; Address 0 onwards are VRAM data
; Negative addresses are registers:
;  -26   registerLogCharactersPerBank,  // log(characters per bank)/log(2) 32
;  -25   registerScanlinesRepeat,                                          33
;  -24   registerHorizontalTotalHigh,                                      34
;  -23   registerHorizontalDisplayedHigh,                                  35
;  -22   registerHorizontalSyncPositionHigh,                               36
;  -21   registerVerticalTotalHigh,                                        37
;  -20   registerVerticalDisplayedHigh,                                    38
;  -19   registerVerticalSyncPositionHigh,                                 39
;  -18   registerMode,                        // port 0x3d8                40
;  -17   registerPalette,                     // port 0x3d9                41
;  -16   registerHorizontalTotal,             // CRTC register 0x00        42
;  -15   registerHorizontalDisplayed,         // CRTC register 0x01
;  -14   registerHorizontalSyncPosition,      // CRTC register 0x02
;  -13   registerHorizontalSyncWidth,         // CRTC register 0x03
;  -12   registerVerticalTotal,               // CRTC register 0x04
;  -11   registerVerticalTotalAdjust,         // CRTC register 0x05
;  -10   registerVerticalDisplayed,           // CRTC register 0x06
;   -9   registerVerticalSyncPosition,        // CRTC register 0x07
;   -8   registerInterlaceMode,               // CRTC register 0x08
;   -7   registerMaximumScanline,             // CRTC register 0x09
;   -6   registerCursorStart,                 // CRTC register 0x0a
;   -5   registerCursorEnd,                   // CRTC register 0x0b
;   -4   registerStartAddressHigh,            // CRTC register 0x0c
;   -3   registerStartAddressLow,             // CRTC register 0x0d
;   -2   registerCursorAddressHigh,           // CRTC register 0x0e
;   -1   registerCursorAddressLow             // CRTC register 0x0f
