cpu 8086
org 0

; Fastest sample rate

; Cycles per sample: 19
; Bytes per sample: 3
; Sample rate: 251.196KHz
; Maximum sound length: 0.87s
; Effective bits per sample: 2.25
  mov al,n
  out dx,al


; Fastest sample rate with samples not in code

; Cycles per sample: 41
; Bytes per sample: 1
; Sample rate: 116.408KHz
; Maximum sound length: 5.63s
; Effective bits per sample: 3.36
loopTop:
  lodsb
  out dx,al
  loop loopTop


; Fastest sample rate with 4-bit compression
; Unrolling might be necessary for DRAM refresh

; Cycles per sample pair: 106
; Cycles per sample: 53
; Bytes per sample: 0.5
; Sample rate: 90.051KHz
; Maximum sound length: 14.56s
; Effective bits per sample: 3.73

loopTop:
  lodsb
  mov ah,al
  and al,0x0f
  out dx,al
  shr ah,1
  shr ah,1
  shr ah,1
  shr ah,1
  mov al,ah
  out dx,al
  loop loopTop


; Compressed to as many bits as possible with 44.1KHz sample rate
; Cycles per 8 samples: 504
; Cycles per sample: 63
; Spare cycles per sample: 45
; Spare cycles per 8 samples: 360
; Bits per sample: 5
; Maximum sound length: 23.78s
; Effective bits per sample: 4.75

  mov cl,5      ;5
loopTop:
  mov bx,[si]   ;nnnnnnnnnnnSSSSS
  inc si
  inc si
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000nnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  mov cl,3      ;3
  shr bx,cl     ;00000000nSSSSSpp
  mov bh,[si]   ;nnnnnnnnnSSSSSpp
  inc si
  dec cx        ;2
  shr bx,cl     ;00nnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  mov cl,5      ;5
  shr bx,cl     ;0000000nnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,1      ;00000000SSSSpppp
  mov bh,[si]   ;nnnnnnnSSSSSpppp
  inc si
  dec cx        ;4
  shr bx,cl     ;0000nnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000000nnSSSSSp
  mov bh,[si]   ;nnnnnnnnnnSSSSSp
  inc si
  shr bx,1      ;0nnnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  inc cx        ;5
  shr bx,cl     ;000000nnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000000000SSSSS
  xchg ax,bx
  out dx,al
  add di,1
  jnz loopTop


  ; 481 cycles

  mov cl,5      ;5
loopTop:

  pop bx        ;nnnnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000nnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;0000000000nSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  dec sp
  pop bx        ;nnnnSSSSSppppppp
  shl bx,1      ;nnnSSSSSppppppp0
  mov al,bh
  and al,0x1f
  out dx,al
  dec sp
  pop bx        ;nnnnnnSSSSSppppp
  shr bx,cl     ;00000nnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;0000000000nSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  dec sp
  pop bx        ;nnnnnnnnnnSSSSSp
  shr bx,1      ;0nnnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;000000nnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000000000SSSSS
  mov al,bl
  out dx,al
  add di,1
  jnz loopTop


  ; 477 cycles - both SI and SP point at data

  mov cl,5      ;5
.l:
  pop bx        ;nnnnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000nnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;0000000000nSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  lodsb
  add si,4
  shr al,1
  shr al,1
  and al,0x1f
  out dx,al
  pop bx        ;nnnnnnSSSSSppppp
  shr bx,cl     ;00000nnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;0000000000nSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  dec sp
  pop bx        ;nnnnnnnnnnSSSSSp
  shr bx,1      ;0nnnnnnnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;000000nnnnnSSSSS
  mov al,bl
  and al,0x1f
  out dx,al
  shr bx,cl     ;00000000000SSSSS
  mov al,bl
  out dx,al
  add di,1
  jnz .l
.endCode

