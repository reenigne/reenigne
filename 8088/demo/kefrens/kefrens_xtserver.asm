%include "../../defaults_bin.asm"

frames equ 838
positions equ 154
scanlines equ 200
footerSize equ (footerEnd-footer)
headerSize equ (headerEnd-header)

  startAudio

  mov ax,cs
  mov es,ax
  mov ds,ax
  cli
  add ax,((unrolledCode + headerSize + 34*scanlines + footerSize + 15)>>4)
  mov ss,ax
  mov sp,0xfffe
  sti

  mov di,unrolledCode

  ; Copy header

  mov cx,headerSize
  mov si,header
  cld
  rep movsb

  ; Unroll

  mov cx,scanlines
  mov bp,ax
  add bp,0x1000
unroll:
  mov si,kefrensScanline
  movsb
  mov ax,bp
  stosw
  mov dx,cx
  mov cx,15
  rep movsw
  movsb
  mov cx,dx
  add bp,(frames*2 + 15)>>4     ; Increase raster segment
  loop unroll

  ; Copy footer

  mov cx,footerSize
  mov si,footer
  rep movsb


  ; Create pixel table

  mov ax,ss
  mov es,ax
  xor dx,dx
  xor di,di
pixelTableOuterLoop:

  mov cx,positions/2
  xor bx,bx
pixelTable3Loop:
  mov si,pixelTable3
  mov ax,bx
  stosw
  times 4 movsw          ; Even y, even x
  mov ax,dx
  or al,0x30
  stosw
  mov ax,bx
  stosw
  times 4 movsw          ; Even y, odd x
  mov ax,dx
  or al,0x30
  stosw
  inc bx
  loop pixelTable3Loop

  mov cx,positions/2
  mov bx,80
pixelTable1Loop:
  mov si,pixelTable1
  mov ax,bx
  stosw
  times 4 movsw          ; Odd y, even x
  mov ax,dx
  or al,0x10
  stosw
  mov ax,bx
  stosw
  times 4 movsw          ; Odd y, odd x
  mov ax,dx
  or al,0x10
  stosw
  inc bx
  loop pixelTable1Loop

  inc dx
  cmp dx,16
  jne pixelTableOuterLoop


  ; Find raster bars segment

  mov dx,es
  add dx,0x1000
  mov es,dx


  ; Create raster bars table

  mov cx,scanlines/2
  mov ax,0
rasterLoopY:
  mov bx,cx

  mov cx,frames
  xor di,di
  rep stosw
  add dx,(frames*2 + 15) >> 4
  mov es,dx

  add ax,positions*6*2*3

  mov cx,frames
  xor di,di
  rep stosw
  add dx,(frames*2 + 15) >> 4
  mov es,dx

  add ax,positions*6*2
  cmp ax,positions*6*2*32
  jb noWrap
  mov ax,0
noWrap:

  mov cx,bx
  loop rasterLoopY


  ; Create sample table

  mov [sampleSeg],dx
  mov es,dx
  xor di,di
  mov si,waveInfoTable
  mov cx,waveInfoLength
waveInfoLoop:
  push cx
  lodsw
  mov bp,ax
  lodsw
  mov cx,ax
  lodsw
  push si
  mov si,ax
  xor dx,dx
sampleLoop:
  mov bl,dh
  xor bh,bh
  mov al,[bx+sineTable]
  add dx,bp
  mov bx,si
  imul bl
  add ax,256*38   ; (-128*75 + 256*38)/256 + 1 = 1, (127*75 + 256*38)/256 + 1 = 76
  mov al,ah
  inc ax
  stosb
  loop sampleLoop
  pop si
  pop cx
  loop waveInfoLoop

  mov ax,0x0101
  mov cx,262/2
  rep stosw
  sub di,262
  mov bx,cs
  mov es,bx
  mov ax,di
  mov di,songTable
  mov cx,songLength
  rep stosw

;  captureScreen

  ; Go into lockstep and reduce DRAM refresh frequency to 4 refreshes per scanline

  lockstep2
  mov al,TIMER1 | LSB | MODE2 | BINARY
  out 0x43,al
  mov al,19
  out 0x41,al  ; Timer 1 rate


  ; Video layout:
  ;   We don't want to have to use the second 8kB so use Max Scan Line Address = 0
  ; scanlines 260..261 startup - set Vertical Total to 1
  ;             0..199 active video
  ;           200..201 set Vertical Total to 59              0.. 1
  ;           202..223 overscan bottom                       2..23
  ;           224..239 vertical sync                        24..39
  ;           240..259 overscan top                         40..59


  ; Set video mode to 2bpp

  mov dx,0x3d8
  mov al,0x0a
  out dx,al

  ; Palette
  ;      1 +OVERSCAN B
  ;      2 +OVERSCAN G
  ;      4 +OVERSCAN R
  ;      8 +OVERSCAN I
  ;   0x10 +BACKGROUND I
  ;   0x20 +COLOR SEL
  inc dx
  mov al,0x30
  out dx,al

  mov dl,0xd4

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

  ;   0x7f Vertical Total                               01
  mov ax,0x0104
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0206
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x1807
  out dx,ax

  ;   0x03 Interlace Mode                               02
  mov ax,0x0208
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0009
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x070b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  inc ax
  out dx,ax

  ;   0x3f Cursor (H)                                   03  0x3c0 == 40*24 == start of last line
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

;  times 38 nop


  ; Set up speaker

  in al,0x61
  or al,3
  out 0x61,al


  ; Set up timer 2 for PWM

  mov al,TIMER2 | LSB | MODE0 | BINARY
  out 0x43,al
  mov al,0x01
  out 0x42,al  ; Counter 2 count = 1 - terminate count quickly


  ; Send read interrupt request command to PIC

  mov al,0x0a
  out 0x20,al


  ; Setup initial registers and clear screen

  mov ax,0xb800
  mov es,ax
  mov cx,80
  xor ax,ax
  xor di,di
  rep stosw
  mov dx,0x03d4
  mov bp,[sampleSeg]
  mov bx,0
  mov word[songPosition],songTable
  mov si,[songTable]


  ; Do the actual effect

  call unrolledCode


  ; Turn off speaker

  in al,0x61
  and al,0xfc
  out 0x61,al


  ; Finish up

  sti
  refreshOn
  mov ax,3
  int 0x10
  stopAudio
  int 0x67


  ; This is the inner loop code. It's not called directory, but instead used
  ; as source data for the unroller, which creates 200 copies with the XXs
  ; filled in.

kefrensScanline:
  db 0xb8              ; mov ax,XXXX
  mov ds,ax            ; 2 0
  mov sp,[bx]          ; 2 2

  pop di               ; 1 2
  mov al,[es:di]       ; 3 1 +WS
  pop cx               ; 1 2
  and ax,cx            ; 2 0
  pop cx               ; 1 2
  or ax,cx             ; 2 0
  stosw                ; 1 2 +WS +WS
  pop ax               ; 1 2
  and ah,[es:di+1]     ; 4 1 +WS
  pop cx               ; 1 2
  or ax,cx             ; 2 0
  stosw                ; 1 2 +WS +WS

  pop ax               ; 1 2
  out dx,al            ; 1 1
  mov ds,bp            ; 2 0
  lodsb                ; 1 1
  out 0x42,al          ; 2 1        Total 1 (2) 31 = 34 bytes

;       AND     OR      AND     OR       AND     OR      AND     OR
;      cdab    cdab    g.ef    g.ef     bc.a    bc.a    fgde    fgde       abcdefg = 5679abd
pixelTable3:
  dw 0x0000, 0x7956, 0x0f00, 0xd0ab,  0x00f0, 0x6705, 0x0000, 0xbd9a
pixelTable1:
  dw 0x0000, 0x7956, 0x0f00, 0xd0ab,  0x00f0, 0x6706, 0x0000, 0xbd9a


%macro audioOnlyScanline 0
  times 56 nop
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al
%endmacro


header:
  mov [cs:savedSP],sp
frameLoop:
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al

  ; Scanline 260: set Vertical Total to 1 and update audio pointer
  mov ax,0x0104
  out dx,ax
  mov dl,0xd9
  mov di,[cs:songPosition]
  mov si,[cs:di]
  inc di
  inc di
  cmp di,songTable+songLength*2
  jne noRestartSong
  mov di,songTable
noRestartSong:
  mov [cs:songPosition],di
  times 15 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al

  ; Scanline 261: audio only
  audioOnlyScanline

headerEnd:

  ; Scanlines 0-199 go in here - actual effect

footer:
  ; Scanline 200: set Vertical Total to 59
  mov dl,0xd4
  mov ax,0x3b04
  out dx,ax
  times 45 nop
  mov ax,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al

  ; Scanline 201: audio only - video still active
  audioOnlyScanline

  ; Scanline 202: clear addresses 0-15
  xor di,di
  xor ax,ax
  nop
  nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al

%rep 9
  ; Scanlines 203-211: clear addresses 18-159
  xor ax,ax
  times 4 nop
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  stosw
  mov al,[es:di]
  mov ds,bp
  lodsb
  out 0x42,al
%endrep

  ; Scanlines 212-258: audio only
%rep 47
  audioOnlyScanline
%endrep

  ; Start of scanline 259: check for ending and update

  times 2 nop
  ; See if the keyboard has sent a byte
  in al,0x20
  and al,2
  jz noKey
  ; Read byte from keyboard
  in al,0x60
  mov ah,al
  ; Acknowledge keyboard
  in al,0x61
  or al,0x80
  out 0x61,al
  and al,0x7f
  out 0x61,al
  ; Check for Escape
  cmp ah,1
  je effectComplete
  jmp doneKey
noKey:
  times 28 nop
doneKey:
  inc bx
  inc bx
  cmp bx,frames*2
  jne noNewLoop
  mov bx,0
noNewLoop:
  jmp frameLoop-34*scanlines
effectComplete:
  mov sp,[cs:savedSP]
  ret

savedSP: dw 0

footerEnd:
songPosition: dw 0
sampleSeg: dw 0
unrollPointer: dw 0, 0

