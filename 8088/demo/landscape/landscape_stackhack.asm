;org 0x100
cpu 8086

segment .text

..start:
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
  mov ah,0xb0
  mov cx,40*100
  rep stosw


  ; Expand sine table

  sineTable equ 303 + 160*32*10 + 1  ; In speedcode segment

  mov si,sineTableQuadrant
  mov ax,landscape + 0x1000
  mov di,sineTable
  mov cx,41
  lea bx,[si+160]
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

  mov ax,landscape
  mov es,ax
  mov ds,ax
  mov cx,0x4000
  mov si,2
  mov di,si
  mov dx,ax
  add dh,0x20   ;  lea dx,[bx+0x2000]
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

  sub dx,0x1000
  mov es,dx       ; ES = speedcode segment
  mov ax,cs
  mov ds,ax
  dec di
  dec di
  mov cx,100
  mov ax,0xca89  ; "mov dx,cx"
  stosw
  mov al,0xc3    ; "ret"
  stosb
buildFillingD1:
  mov ax,0xca01  ; "add dx,cx"
  stosw
  mov al,0xc3    ; "ret"
  stosb
  loop buildFillingD1

  ; Create the speedcode

  mov bx,sineTable
buildColumns:
  mov si,distanceTable
buildColumn:
  mov ax,0x8ec5  ; "lds cx,[bp+iw]"
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
  mov ax,0x91c2  ; "sub dx,ax", "xchg cx,ax"
  stosw
  mov ax,0xd2ff  ; "call dx"
  stosw

  cmp si,distanceTable+32
  jne buildColumn

  inc bx
  inc bx
  cmp bx,sineTable+320
  jnz buildColumns

  mov al,0xe9
  stosb
  mov ax,0x10000-160*32*10
  stosw          ; "jmp speedcode"

  ; Create the negative (drawing) part of the fillingD routine

  mov di,-300
  mov cx,100
buildFillingD2:
  mov al,0xaa    ; "stosb"
  stosb
  mov ax,0xdf01  ; "add di,bx"
  stosw
  loop buildFillingD2


  ; Create the perspective table
  mov ax,landscape + 0x2000
  mov es,ax
  xor di,di
  xor cx,cx
  xor dx,dx
perspectiveLoop:
  mov bx,distanceTable
distanceLoop:
  mov al,10
  mul cl
  sub ax,1000
  neg ax
  idiv word[bx]
  inc bx
  add ax,10
  cmp ax,0
  jge noClampLow
  xor ax,ax
noClampLow:
  cmp ax,99
  jle noClampHigh
  mov ax,99
noClampHigh:
  mov bp,ax
  add ax,ax
  add ax,bp
  stosw

  cmp bx,distanceTable+32
  jl distanceLoop
  inc cx
  cmp cx,99
  jl perspectiveLoop


  ; Save 6 bytes of landscape for interrupts
  mov ax,landscape
  mov es,ax
  xor bx,bx
  mov ax,[es:bx]
  mov [savedLandscape],ax
  mov ax,[es:bx+2]
  mov [savedLandscape+2],ax
  mov ax,[es:bx+4]
  mov [savedLandscape+4],ax

  in al,0x21
  mov [imr],al
  mov al,0xfe  ; Enable IRQ0 (timer), disable all others
  out 0x21,al
  xor bx,bx
  mov es,bx
  mov ax,[es:bx+0x20]
  mov [oldInterrupt8],ax
  mov ax,[es:bx+0x22]
  mov [oldInterrupt8+2],ax
  cli
  mov word[es:bx+0x20],interrupt8
  mov word[es:bx+0x22],cs
  sti


mainLoop:
  ; Move camera
  mov ax,[cameraU]
  add ax,0x100
  mov [cameraU],ax
  mov bl,ah
  mov ax,[cameraV]
  add ax,0x09f
  mov [cameraV],ax
  mov bh,ah
  mov bp,bx
  ; Rotate camera
  mov ax,[cameraAngle]
  add ax,0x100
  mov [cameraAngle],ax

  ; Set up variables for render
  mov bx,-81
  mov ax,0xb800
  mov es,ax



tearDown:
  xor bx,bx
  mov es,bx
  cli
  mov ax,[cs:oldInterrupt8]
  mov [es:bx+0x20],ax
  mov ax,[cs:oldInterrupt8+2]
  mov [es:bx+0x22],ax
  sti

  mov al,[imr]
  out 0x21,al
  mov ax,3
  int 0x10
  int 0x20


interrupt8:
  mov sp,cs
  mov ss,sp
  mov sp,stackTop

  push ds
  push ax
  push bx
  mov ax,landscape
  mov ds,ax
  xor bx,bx

  ; If the segment part of the return address isn't in the speedcode area,
  ; it means the interrupt happened between the "sti" and the "jmp far" of a
  ; previous interrupt, and we should throw away that return address because
  ; we don't actually care about it.

  cmp word[bx-4],landscape + 0x1000
  jne returnAddressOk
  mov ax,[bx-6]
  mov [cs:interruptReturnAddress],ax
returnAddressOk:
  mov ax,[bx-2]                 ; Saved flags
  and ah,0xfd                   ; Clear interrupt flag
  push ax

  mov ax,[cs:savedLandscape]
  mov [bx-6],ax
  mov ax,[cs:savedLandscape+2]
  mov [bx-4],ax
  mov ax,[cs:savedLandscape+4]
  mov [bx-2],ax

  pushf
  call far[oldInterrupt8]  ; This is an interrupt routine so it will pop flags on exit

  popf  ; Restore interrupted flags, except IF
  pop bx
  pop ax
  pop ds

enterSpeedcode:
  mov sp,landscape + 0x1000
  mov ss,sp
  xor sp,sp
  sti
  jmp far[cs:interruptReturnAddress]




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

savedLandscape: resb 6
interruptReturnAddress: resb 4
imr: resb 1
oldInterrupt8: resb 2
cameraU: resb 2
cameraV: resb 2
cameraAngle: resb 2
speedcodeEntries: resb 160*2

segment stack stack
  resb 64
stackTop:

segment landscape

  incbin "gendata\landscape.dat"
