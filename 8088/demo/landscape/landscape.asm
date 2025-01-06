;org 0x100
cpu 8086

  landscapeSegment equ ((textEnd - ..start) + 15) >> 4
;  landscapeSegment equ landscape
  speedCodeSegment equ landscapeSegment + 0x1000
  stackSegment equ speedCodeSegment + 0x1000
  perspectiveSegment equ stackSegment + 0x1000
  ;endSegment equ perspectiveSegment + ((64*99) >> 4)

  bytesPerIteration equ 12
  iterationsPerColumn equ 32
  bytesPerColumn equ 4 + bytesPerIteration*iterationsPerColumn + 14
  totalColumns equ 160
  columnsSize equ bytesPerColumn*totalColumns + 3
  fillingDSize equ 303

  ; Offsets in SS relative to initial SP

  skyPointerTable equ 0
  skyJumpTable equ skyPointerTable + 256*2
  skyDataTable equ skyJumpTable + 21*3
  diCorrectionTable equ skyDataTable + 160*10*3
  cameraU equ diCorrectionTable + 22*3
  cameraV equ cameraU + 2
  cameraAngle equ cameraV + 2
  columnStarts equ cameraAngle + 2
  savedSpeedCode equ columnStarts + 160*2
  savedSpeedCodeAddress equ savedSpeedCode + 1
  frameCounter equ savedSpeedCodeAddress + 2
  vramOffset equ frameCounter + 2
  vramOffsetLow equ vramOffset + 2


segment .text

..start:
  ; Set up stack. This must be computed at runtime because we don't know the
  ; segment we're loaded at assembly time.

  ; Example: desired inner SP = 0x1000 (landscapeSegment) - stackBaseSegment is 0x318c
  ; desired initial SP = 0x1004
  ; stackTop = 0x17f0 (relative to stackBaseSegment)
  ;   stack actually starts somewhere between 0x17e1 and 0x17f0 relative to stackBaseSegment
  ; adjustment = 0x07d0 bytes, 0x007d paragraphs
  ; SS = 0x3209

  ; Note that some data (in this case in diCorrectionTable) will be split across the segment boundary when accessed via SS

  mov ax,cs
  mov bx,ax
  mov cx,ax
  add bx,landscapeSegment + 4  ; this will be our initial SP value
  add ax,stackSegment
  add cx,speedCodeSegment
  mov ds,cx
  cli
  mov sp,bx
  mov ss,ax
  sti
  mov es,ax

  ; Init skyPointerTable and columnStarts

  mov ax,fillingDSize
  mov cx,160
  xor di,di
  mov bp,0x1fe
  lea dx,[bx+skyDataTable-(90*3-81)]
initSkyPointerTableLoop:
  mov [es:bx+di+columnStarts],ax
  add ax,bytesPerColumn
  inc di
  inc di
  mov si,ax
  and si,bp
  mov [es:bx+si+skyPointerTable],dx
  add dx,10*3
  loop initSkyPointerTableLoop

  ; Init skyJumpTable

  mov ax,fillingDSize+columnsSize+462
  mov dx,61
  mov si,4
  mov cx,7
  lea di,[bx+skyJumpTable]
initSkyJumpTableLoop:
  stosw
  inc di
  sub ax,dx
  sub dx,si
  loop initSkyJumpTableLoop
  inc ax
  stosw
  inc di
  sub ax,30
  stosw
  inc di
  sub ax,24
  stosw
  inc di
  sub ax,7
  stosw
  inc di
  sub ax,5
  stosw
  inc di
  mov cx,9
initSkyJumpTableLoop2:
  sub ax,6
  stosw
  inc di
  loop initSkyJumpTableLoop2


  ; Init skyDataTable

  mov cx,160*10
  xor si,si
initSkyDataLoop:
  movsw
  inc di
  loop initSkyDataLoop

  ; Init diCorrectionTable
  ; y=99 is the top of the screen, y=0 is the bottom

  mov cx,11
  mov ax,90*80+2
initDICorrectionLoop:
  stosw
  inc di
  add ax,80
  loop initDICorrectionLoop

  mov cx,11
  mov ax,90*80+1
initDICorrectionLoop2:
  stosw
  inc di
  add ax,80
  loop initDICorrectionLoop2

    ; Accommodate DOSBox brokenness (IP not wrapping around) - remove before party!
    xor di,di
    mov ax,0xfeeb
    stosw


  ; Expand sine table

  ; The expanded sine table is in the speedcode segment, after the main
  ; column code. It will get overwritten by the sky/ground fixup code
  ; but we'll have finished with it by then.
  sineTable equ 303 + columnsSize

  mov si,sineTableQuadrant
  mov ax,cs
  mov ds,ax
  add ax,speedCodeSegment
  mov es,ax
  xchg ax,bp
  mov di,sineTable
  mov cx,41
  lea bx,[di+160]
sineHighLoop:
  lodsw
  mov [es:bx],ax
  mov [es:di+320],ax
  stosw
  neg ax
  mov [es:bx+160],ax
  mov [es:di+158],ax
  dec bx
  dec bx
  loop sineHighLoop


  ; Fix up segments in landscape table

  lea dx,[bp+0x2000] ; perspectiveSegment
  sub bp,0x1000  ; landscapeSegment
  mov es,bp
  mov ds,bp
  mov cx,0x4000
  mov si,2
  mov di,si
fixupLoop:
  lodsw
  add ax,dx
  stosw
  inc di
  inc di
  inc si
  inc si
  loop fixupLoop


  ; Create the positive (non-drawing) part of the fillingD routine

  sub dh,0x20     ; speedcodeSegment
  mov es,dx
  mov ax,cs
  mov ds,ax
  dec di
  dec di         ; di = 0
  mov ax,0xea89  ; "mov dx,bp"
  stosw
  mov al,0xc3    ; "ret"
  stosb
  mov cx,100
buildFillingD1:
  mov ax,0xea01  ; "add dx,bp"
  stosw
  mov al,0xc3    ; "ret"
  stosb
  loop buildFillingD1

  ; Create the speedcode

  mov bx,sineTable
buildColumns:

  ; Pre-column

  mov ax,0xf631  ; "xor si,si"
  stosw
;  mov ax,0xd231  ; "xor dx,dx"
  mov ax,0xca89  ; "mov dx,cx"
  stosw

  mov si,distanceTable
buildColumn:
  mov ax,0xdc8e  ; "mov ds,sp"
  stosw
  mov ax,0xafc5  ; "lds bp,[bx+iw]"
  stosw

  ; Compute UV offset
  lodsb
  cbw
  mov bp,ax
  imul word[es:bx]
  mov ch,dl
  xchg bp,ax
  imul word[es:bx+40]
  add dl,dl
  mov cl,dl
  add cx,cx
  xchg ax,cx
  stosw

  mov ax,0x29ad  ; "lodsw", ...
  stosw
  mov ax,0x95c2  ; "sub dx,ax", "xchg bp,ax"
  stosw
  mov ax,0xd2ff  ; "call dx"
  stosw

  cmp si,distanceTable+32
  jne buildColumn

  ; Post-column

  mov si,postColumn
  movsw          ; "mov bp,dx"
  movsw          ; "sub bp,lastHWM"
  movsw          ;     cs:
  movsw          ; mov [fixup+2],dl
  lea ax,[di-4]  ; fixup+2
  stosw
  mov ax,0x96ff  ; "call [bp+iw]"
  stosw
  mov ax,sp
  add ax,skyJumpTable+10*3-256  ; offset in SS to zero point of jump table
  stosw

  inc bx
  inc bx
  cmp bx,sineTable+320
  jnz buildColumns

  ; Jump back to the start of the routine so we can start it anywhere

  mov al,0xe9
  stosb
  mov ax,0x10000-columnsSize  ; address of start of columns code relative to instruction after this one
  stosw          ; "jmp speedcode"

  ; Ground fixup

  mov cx,9
  mov bx,799
  mov dx,80
groundFixupLoop:
  mov ax,0xc626  ; "mov b[es:di+799],0xb0"
  stosw
  mov al,0x85
  stosb
  mov ax,bx
  stosw
  sub bx,dx
  mov al,0xb0
  stosb
  loop groundFixupLoop

  mov si,groundFixup1
  movsw  ; mov byte[es:di+79],0xb0
  movsw  ; fixup0:
  movsw  ;    mov bp,dx
  movsw  ;    add di,[bp+diCorrectionTable]
  movsb
  mov ax,sp
  add ax,diCorrectionTable + 81 - 270
  stosw

  movsw  ; ret skyFixup1: pop bp
  movsw  ; mov ax,bp
  movsw  ; and bp,0x1fe
  movsw
  movsw  ; mov si,[bp+skyPointerTable]
  add ax,skyPointerTable - (diCorrectionTable + 81 - 270)
  stosw
  movsw  ; add si,dx
  movsb    ; dec di
  movsw  ; ss: movsw
  movsw  ; mov bp,dx
  movsw  ; add di,[bp+diCorrectionTable]
  add ax,(diCorrectionTable + 81 + 33 - 270) - skyPointerTable
  stosw
  movsw  ; jmp ax
  mov si,skyFixup1

  movsw  ; skyFixup2: pop bp  mov ax,bp
  movsw  ;    and bp,0x1fe
  movsw
  movsw  ;    mov si,[bp+skyPointerTable]
  movsb
  add ax,skyPointerTable - (diCorrectionTable + 81 + 33 - 270)
  stosw
  movsw  ; add si,dx
  movsb    ; dec di
  movsw  ; ss: movsw
  mov si,skyFixup2Iter
  movsw  ; add di,cx
  movsw  ; dec di  inc si
  movsw  ; ss: movsw
  mov si,skyFixupEnd
  movsw  ; mov bp,dx
  movsw  ; add di,[bp+diCorrectionTable]
  add ax,(diCorrectionTable + 81 + 36 - 270) - skyPointerTable
  stosw
  movsw  ; jmp ax

  mov si,skyFixup1
  movsw  ; skyFixup3: pop bp  mov ax,bp
  movsw  ;    and bp,0x1fe
  movsw
  movsw  ;    mov si,[bp+skyPointerTable]
  movsb
  add ax,skyPointerTable - (diCorrectionTable + 81 + 36 - 270)
  stosw
  movsw  ; add si,dx
  movsb    ; dec di
  movsw  ; ss: movsw
  mov si,skyFixup2Iter
  movsw  ; add di,cx
  movsw  ; dec di  inc si
  movsw  ; ss: movsw
  mov si,skyFixup2Iter
  movsw  ; add di,cx
  movsw  ; dec di  inc si
  movsw  ; ss: movsw
  mov si,skyFixupEnd
  movsw  ; mov bp,dx
  movsw  ; add di,[bp+diCorrectionTable]
  add ax,(diCorrectionTable + 81 + 39 - 270) - skyPointerTable
  stosw
  movsw  ; jmp ax


  mov dx,2
  mov bp,diCorrectionTable + 81 + 39 - 270

skyFixupOuterLoop:
  mov si,skyFixup1
  movsw  ; skyFixup4: pop bp  mov ax,bp
  movsw  ;    and bp,0x1fe
  movsw
  movsw  ;    mov si,[bp+skyPointerTable]
  movsb
  add ax,skyPointerTable
  sub ax,bp
  stosw
  movsw  ; add si,dx
  mov si,skyFixup4ss
  movsw  ; mov bp,ss
  movsw  ; mov ds,bp
  movsw  ; dec cx  dec di
  movsw  ; movsw  add di,cx
  movsw  ;    inc si

  mov cx,dx
skyFixupInnerLoop:
  mov si,skyFixup4Iter
  movsw  ; movsw  add di,cx
  movsw  ;    inc si
  loop skyFixupInnerLoop

  mov si,skyFixup4End
  movsw  ; movsw  inc cx
  movsw  ; mov si,dx
  movsw  ; add di,[si+diCorrectionTable]
  add bp,3
  add ax,bp
  sub ax,skyPointerTable
  stosw
  movsw  ; jmp ax

  inc dx
  cmp dx,8
  jbe skyFixupOuterLoop



  ; Create the negative (drawing) part of the fillingD routine

  mov di,-300
  mov cx,100
buildFillingD2:
  mov al,0xaa    ; "stosb"
  stosb
  mov ax,0xcf01  ; "add di,cx"
  stosw
  loop buildFillingD2


  ; Create the perspective table
  mov ax,cs
  add ax,perspectiveSegment
  mov es,ax
  xor di,di
  xor cx,cx
perspectiveLoop:
  mov si,distanceTable
distanceLoop:
  lodsb
  mov ah,0
  xchg ax,bx      ; bx = rr
;  mov al,10
   mov al,40
  mul cl          ; ax = 10*h
;  sub ax,1000     ; ax = 10*h - 1000
   sub ax,4000
  neg ax          ; ax = 1000 - h*10
  ;xor dx,dx
  cwd
  idiv bx         ; ax = (1000 - h*10)/rr
  xor bx,bx
  cmp si,distanceTable+32
  jne noInfinity
;  xor ax,ax
  mov bx,90
noInfinity:
  sub ax,100       ; ax = (1000 - h*10)/rr - 100
  neg ax          ; ax = 100 - (1000 - h*10)/rr
  cmp ax,bx
  jge noClampLow
  mov ax,bx
noClampLow:
;  cmp ax,90 ;100
  cmp ax,100
  jle noClampHigh
;  mov ax,90 ;100
  mov ax,100
noClampHigh:
  mov bp,ax
  add ax,ax
  add ax,bp
  sub ax,81
  stosw

  cmp si,distanceTable+32
  jl distanceLoop
  inc cx
  cmp cx,99
  jl perspectiveLoop


  ; Set up video mode

  mov ax,1
  int 0x10

  mov dx,0x3d8
  mov al,8
  out dx,al

  mov dl,0xd4
  mov ax,0x7f04
  out dx,ax
  mov ax,0x6406
  out dx,ax
  mov ax,0x7007
  out dx,ax
  mov ax,0x0109
  out dx,ax

  mov ax,0xb800
  mov es,ax
  mov ax,0xb0
  mov cx,40*100
  xor di,di
  rep stosw

  mov bp,sp
  xor ax,ax
    mov word[bp+frameCounter],ax
  mov word[bp+cameraU],ax
  mov word[bp+cameraV],ax
  mov word[bp+cameraAngle],ax
  mov word[bp+vramOffset],ax
  mov byte[bp+vramOffsetLow],al

mainLoop:
  mov bp,sp
    inc word[bp+frameCounter]
    cmp word[bp+frameCounter],0x41
    jne noBreakPoint
    nop
    noBreakPoint:

  push cs
  mov ax,afterRender
  push ax

  ; Rotate camera
  mov cx,0x100  ; Amount by which to rotate
  mov ax,[bp+cameraAngle]
  add ax,cx
  jc cameraAngleUnderflow
  cmp ax,totalColumns*0x100
  jb noOverflow
  sub ax,totalColumns*0x100
  jmp noOverflow
cameraAngleUnderflow:
  add ax,totalColumns*0x100
noOverflow:
  mov [bp+cameraAngle],ax
  mov al,ah
  mov ah,0
  add ax,ax
  mov si,ax

  mov al,ch
  cbw
  add [bp+vramOffsetLow],cl
  mov bx,[bp+vramOffset]
  mov cx,bx
  adc bx,ax
  sub cx,bx
  neg cx
  and bx,0x1fff
  mov [bp+vramOffset],bx

  ; Update CGA start address
  mov ah,bh
  mov al,0x0c
  mov dx,0x3d4
  out dx,ax
  mov ah,bl
  inc ax
  out dx,ax

  xchg ax,cx
  cmp ax,0
  jge notNegativeScroll
  ; TODO
    cli
    hlt
notNegativeScroll:
  cmp ax,40
  jle notNewScene
  mov ax,40
notNewScene:
  mov dx,ax      ; Number of columns to clear
  lea di,[si+80]
  sub di,dx
  sub di,dx
  cmp di,160*2
  jb noOverflow4
  sub di,160*2
noOverflow4:     ; di is now number of first column to clear
  mov bx,[di+bp+columnStarts]
scrollLoop:
  mov ax,cs
  add ax,speedCodeSegment
  mov es,ax
  mov byte[es:bx+bytesPerColumn - 10],(100*3 - 81)&0xff  ; Top of screen
  sub bx,bytesPerColumn
  cmp bx,fillingDSize
  ja noScrollWrap
  add bx,bytesPerColumn*totalColumns
noScrollWrap:
  mov cx,0xb800
  mov es,cx
  mov cx,10
  mov al,0xb0
  mov di,[bp+vramOffset]
  sub di,dx
  add di,di
  add di,80
scrollInnerLoop:
  stosb
  add di,79
  loop scrollInnerLoop
  mov byte[es:di+89*80],al

  dec dx
  jnz scrollLoop


  ; Move camera
  mov ax,[bp+cameraU]
  add ax,0x100
  mov [bp+cameraU],ax
  and ah,0xfe
  mov bl,ah
  mov bh,0
  shl bx,1
  mov ax,[bp+cameraV]
  add ax,0x09f
  mov [bp+cameraV],ax
  and ax,0xfe00
  or bx,ax


  ; Address at which to start running the speed code

  mov ax,cs
  add ax,speedCodeSegment
  mov es,ax
  push ax
  push word[si+bp+columnStarts]

  add si,80
  cmp si,160*2
  jb noOverflow2
  sub si,160*2
noOverflow2:
  mov di,[si+bp+columnStarts]
  mov al,[es:di]
  mov [bp+savedSpeedCode],al
  mov [bp+savedSpeedCodeAddress],di
  mov al,0xcb ; "retf"
  stosb

  ; Set up variables for render
  mov cx,-81
  mov di,80*99+1
  mov ax,[bp+vramOffset]
  add di,ax
  add di,ax
  mov ax,0xb800
  mov es,ax
  retf
afterRender:
  mov bp,sp
  mov ax,cs
  add ax,speedCodeSegment
  mov es,ax
  mov bx,[bp+savedSpeedCodeAddress]
  mov al,[bp+savedSpeedCode]
  mov [es:bx],al

    cmp di,80*100+1
    je finalDIOk
    nop
    finalDIOk:

  jmp mainLoop


tearDown:
  mov ax,3
  int 0x10
  int 0x20






; 160 entry sine table scaled to 0x7fff
sineTableQuadrant:
  dw 0x0000, 0x0506, 0x0A0A, 0x0F0B, 0x1405, 0x18F8, 0x1DE1, 0x22BE
  dw 0x278D, 0x2C4D, 0x30FB, 0x3596, 0x3A1B, 0x3E8A, 0x42E0, 0x471C
  dw 0x4B3B, 0x4F3D, 0x5320, 0x56E2, 0x5A81, 0x5DFD, 0x6154, 0x6484
  dw 0x678D, 0x6A6C, 0x6D22, 0x6FAD, 0x720B, 0x743D, 0x7640, 0x7815
  dw 0x79BB, 0x7B30, 0x7C75, 0x7D89, 0x7E6B, 0x7F1B, 0x7F99, 0x7FE5
  dw 0x7FFF

; Our texture samples in each direction don't have to be evenly spaced.
; It makes sense to have those in the far distance further apart.
; Space them on a quadratic curve.
distanceTable:
  db  1,  2,  3,  4,  5,  7,  9, 11, 13, 15, 17, 20, 22, 25, 28, 31
  db 34, 38, 41, 45, 49, 53, 57, 61, 65, 70, 75, 80, 85, 90, 95,101

postColumn:
  mov bp,dx
;fixup
  sub bp,((100*3 - 81) & 0xff) - 0x100   ; initial state is ground right up to the top of the screen
  db 0x2e        ; cs:
  db 0x88,0x16   ; mov [iw],dl

groundFixup1:
  mov byte[es:di+79],0xb0

  mov bp,dx
  db 0x03, 0xbe  ; add di,[bp+iw]
  ret                                                            ; 1

skyFixup1:
  pop bp                                                         ; 1
  mov ax,bp                                                      ; 2
  and bp,0x1fe                                                   ; 4
  db 0x8b,0xb6  ; mov si,[bp+iw]  ;; sky pointer table           ; 4

  add si,dx                                                      ; 2
    dec di                                                         ; 1
  ss movsw                                                      ; 2
skyFixupEnd:
  mov bp,dx                                                      ; 2
  db 0x03,0xbe  ; add di,[bp+0x1334]  ; dx = 100 => di += 80*dx + 2              ; 4
  jmp ax                                                         ; 2

skyFixup2Iter:
  add di,cx                                                      ; 2
  dec di                                                         ; 1
  inc si                                                         ; 1
  ss movsw                                                      ; 2

skyFixup4ss:
  mov bp,ss                                                      ; 2
  mov ds,bp                                                      ; 2
  dec cx                                                         ; 1
    dec di                                                         ; 1
skyFixup4Iter:
  movsw                                                          ; 1
  add di,cx                                                      ; 2
  inc si                                                         ; 1
skyFixup4End:
  movsw                                                          ; 1
  inc cx                                                         ; 1
  mov si,dx                                                      ; 2
  db 0x03,0xbc  ; add di,[si+0x1634]  ; dx = 100 => di += 80*dx + 2              ; 4
  jmp ax                                                         ; 2



textEnd:

align 16

segment landscape

  incbin "gendata\landscape.dat"

; speedCodeSegment overlaps sky - sky is first expanded/copied to stackSegment

segment sky

  incbin "\voxelsky2.bin"

;segment data

;cameraU: resb 2
;cameraV: resb 2
;cameraAngle: resb 2
;speedcodeEntries: resb 160*2

;segment stack stack
;  resb 64
;stackTop:

