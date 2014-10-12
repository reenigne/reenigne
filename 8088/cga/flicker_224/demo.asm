org 0x100
cpu 8086

PERIOD EQU (76*262/4)

  ; Set up pointers for music

  mov ax,cs
  mov ds,ax
  mov ax,codeEnd
  add ax,15
  mov cl,4
  shr ax,cl
  mov bx,cs
  add ax,bx
  mov [musicPointer+2],ax

  ; Load music file

  mov ax,0x3d00
  mov dx,musicFileName
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
  mov [musicEnd],ax
  mov ax,0x4200          ; seek to start
  int 0x21
  jc error
  mov cx,[musicEnd]
  mov ah,0x3f
  mov ds,[musicPointer+2]
  xor dx,dx
  int 0x21               ; read file
  jc error
  mov ah,0x3e
  int 0x21               ; close file
  jc error

  ; Set up pointers for video

  mov ax,cs
  mov ds,ax
  mov ax,[musicPointer+2]
  mov bx,[musicEnd]
  add bx,15
  mov cl,4
  shr bx,cl
  add ax,bx
  mov [startSegment],ax

  ; Load video file

  mov ax,0x3d00
  mov dx,videoFileName
  int 0x21               ; open file
  jc error
  mov bx,ax
  mov ax,0x4202
  xor cx,cx
  xor dx,dx
  int 0x21               ; seek to end
  jnc noError
error:
  mov ah,9
  mov dx,errorMessage
  int 0x21
  ret
noError:
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  shr dx,1
  rcr ax,1
  mov di,ax
  mov si,[startSegment]
  add ax,si
  mov [endSegment],ax
  mov ds,ax
  cmp ax,0xa000
  jae error
  mov ax,0x4200
  xor dx,dx
  int 0x21               ; seek to start
  jc error
loadLoop:
  cmp di,0x800
  jae fullBlock
  cmp di,0
  je loadDone
  mov ah,0x3f
  mov cx,di
  shl cx,1
  shl cx,1
  shl cx,1
  shl cx,1
  mov ds,si
  xor dx,dx
  int 0x21
  jc error
loadedABlock:
  shr cx,1
  shr cx,1
  shr cx,1
  shr cx,1
  sub di,cx
  add si,cx
  jmp loadLoop
fullBlock:
  mov ah,0x3f
  mov cx,0x8000
  mov ds,si
  xor dx,dx
  int 0x21
  jc error
  jmp loadedABlock
loadDone:
  mov ah,0x3e
  int 0x21
  jc error


  cli

  ; Set up interrupt
  xor ax,ax
  mov ds,ax
  mov bx,[8*4]
  mov cx,[8*4+2]
  mov word[8*4],interrupt8
  mov [8*4+2],cs
  mov ax,cs
  mov ds,ax
  mov [oldInterrupt8],bx
  mov [oldInterrupt8+2],cx

  ; Set up PIT channel 0
  mov al,0x34
  out 0x43,al
  mov al,PERIOD & 0xff
  out 0x40,al
  mov al,PERIOD >> 8
  out 0x40,al

  ; Set up speaker
  in al,0x61
  mov [originalPortB],al
  or al,3
  out 0x61,al


  ; Set up CGA

  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x08
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

  mov dl,0xd4

  ;   0xff Horizontal Total                             38
  mov ax,0x3800
  out dx,ax

  ;   0xff Horizontal Displayed                         1c  (28 characters)
  mov ax,0x1c01
  out dx,ax

  ;   0xff Horizontal Sync Position                     27  (0x2d - (40 - 28)/2)
  mov ax,0x2702
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0d03
  out dx,ax

  ;   0x7f Vertical Total                               7e
  mov ax,0x7e04
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           7f
  mov ax,0x7f06
  out dx,ax

  ;   0x7f Vertical Sync Position                       6a
  mov ax,0x6a07
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
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

  ; Set up video meory segment
  mov ax,0xb800
  mov es,ax

  ; Set up video update segment
  mov ds,[startSegment]
  xor si,si

  sti

mainLoop:
  lodsw                     ; frame number for next update
waitForFrame:
  cmp ax,[cs:frameCounter]
  jge doUpdate
  ; Check keyboard
  push ax
  mov ah,1
  int 0x16
  cmp al,27
  je finish
  pop ax
  jmp waitForFrame
doUpdate:
  lodsw                     ; Video memory copy address
  mov di,ax
  lodsw                     ; Copy byte count and line count
  mov dx,ax
  mov bx,28  ; 28 words per scanline
  sub bl,dl  ; BX = number of words to skip each row
yLoop:
  mov cl,dl
  rep movsw
  add di,bx
  dec dh
  jnz yLoop
  lodsw                     ; CRTC start address
  mov [cs:crtcPointer],ax

  ; Segment correction
  mov ax,ds
  mov bx,di
  mov cl,4
  shr bx,cl
  add ax,bx
  mov ds,ax
  and di,0x0f
  cmp ax,[cs:endSegment]
  jne mainLoop
  mov ax,[cs:startSegment]
  mov word[cs:frameCounter],0
  jmp mainLoop


finish:
  ; Restore everything
  cli

  mov ax,3
  int 0x10

  mov bx,[oldInterrupt8]
  mov cx,[oldInterrupt8+2]
  xor ax,ax
  mov ds,ax
  mov [8*4],bx
  mov [8*4+2],cx
  mov ax,cs
  mov ds,ax

  mov al,0x36
  out 0x43,al
  mov al,0
  out 0x40,al
  out 0x40,al

  mov al,[originalPortB]
  out 0x61,al

  sti

  ret


interrupt8:
  push ax
  push ds

  ; Set up DS
  push bx
  mov bx,cs
  mov ds,bx

  ; Update audio
  push si
  lds si,[musicPointer]
  lodsw
  mov ds,bx
  cmp si,[musicEnd]
  jl noMusicEnd
  xor si,si
noMusicEnd:
  mov [musicPointer],si
  xchg bx,ax
  mov al,0xb6
  out 0x43,al
  mov al,bl
  out 0x42,al
  mov al,bh
  out 0x42,al

  ; Update video
  mov al,[crtcPointer]
  add al,5
  cmp al,40
  jne noResetCrtcPointer
  xor ax,ax
  inc word[frameCounter]
noResetCrtcPointer:
  mov [crtcPointer],al
  test al,1
  je noCrtcUpdate
  mov ah,0
  xchg si,ax
  add si,crtcTable
  lodsw
  add ax,[crtcStartAddress]
  push dx
  mov dx,0x3d4
  mov bx,ax
  mov al,0x0c
  mov ah,bh
  out dx,al   ; Start Address High
  inc ax
  mov ah,bl
  out dx,al   ; Start Address Low
  lodsw
  mov bx,ax
  lodsb
  mov ah,al
  mov al,5
  out dx,al   ; Vertical Total Address
  inc ax
  mov ah,bl
  out dx,al   ; Vertical Displayed
  inc ax
  mov ah,bh
  out dx,al   ; Vertical Sync Position
  pop dx
noCrtcUpdate:

  ; Restore and chain interrupt
  pop si
  pop bx
  add word[pitCount],PERIOD
  pop ds
  jc pitFallback
  mov al,0x20
  out 0x20,al
  pop ax
  iret
pitFallback:
  pop ax
  jmp far [cs:oldInterrupt8]

oldInterrupt8: dw 0,0
pitCount: dw 0
musicPointer: dw 0,0
musicEnd: dw 0
crtcPointer: db 0
crtcStartAddress: dw 0
frameCounter: dw 0
originalPortB: db 0
startSegment: dw 0
endSegment: dw 0

crtcTable:
  ;   SAL   SAH    VD   VSP   VTA
  db 0xe4, 0x0d, 0x7f, 0x7f, 0x00  ; 0x0de4 = 28*127
  db 0xe4, 0x0d, 0x7f, 0x7f, 0x00
  db 0x50, 0x0f, 0x0d, 0x43, 0x08  ; 0x0f50 = 28*140
  db 0x50, 0x0f, 0x0d, 0x43, 0x08
  db 0x34, 0x1d, 0x7f, 0x7f, 0x00  ; 0x1d34 = 28*(127+140)
  db 0x34, 0x1d, 0x7f, 0x7f, 0x00
  db 0x00, 0x00, 0x0d, 0x43, 0x08  ; 0x0000 = 28*0
  db 0x00, 0x00, 0x0d, 0x43, 0x08

musicFileName: db "music.dat",0
videoFileName: db "video.dat",0
errorMessage: db "File error$"

codeEnd:
