; Assume fadeFrames > fadeSteps
; Unroll step (i) loop
; Unroll space (s) loop and skip either 0, 1 or 2 iterations

; Attribute fade:
; ds:si = pointer into wipeSequence (*2)
; es:di = pointer into updateBuffer
; ds:oldImage = old image
; ds:newImage = new image
; ds:fadeAttribute = fade attribute table (256*_fadeSteps entries)
  lodsw                                              ; 3
  stosw                                              ; 3
  xchg bx,ax                                         ; 1
  mov al,[bx+1+oldImage]   ; Attribute to change     ; 5
  mov bx,step*256+fadeAttribute                      ; 3
  xlatb                                              ; 2
  stosb                                              ; 2
  inc di                                             ; 1
  inc di                                             ; 1  total: 21 IOs, 96 cycles     633 iterations possible in active     14 bytes

; Update is a string of:
; es = 0xb800
  mov byte[1234],56  ; c6 06 1234 56                 ; 5+1 IOs  34.45 cycles   547 byte writes possible in inactive           5 bytes    (14+5)*547 = 10393 bytes unrolled code

; Suppose we want the wipe to take 174 frames. Then each frame we can do 11 steps.


; RGB cube:
; ds:si = pointer into wipeSequence (*2)
; es:di = pointer into updateBuffer
; ds:0 (or 0x4000) = redGreen (red even bytes, green odd bytes) combined RGB values from both images, 2 bytes per character = 16000 bytes
; es:0 (or 0x4000) = blue = (even bytes, 0 odd bytes) combined RGB values from both images, 2 bytes per character = 16000 bytes
; ds:redTable, greenTable, blueTable = 8*64*3 = 1536 bytes
; ds:rgb (at 00000000 000TABLE) = 2048 bytes, 256 byte aligned
  lodsw                                                        ; 3
  stosw                                                        ; 3
  xchg bx,ax                                                   ; 1
  mov dx,[bx]     ; dx =  rrrRRR00 gggGGG00                    ; 4
  mov bx,[es:bx]  ; bx =  bbbBBB00 00000000                    ; 5
  mov al,[bx+blueTable + step*64]   ; al = 0bbb0000            ; 5
  mov bl,dh                                                    ; 2
  add al,[bx+greenTable + step*64]  ; al = 0bbbggg0            ; 5
  mov bl,dl                                                    ; 2
  mov ah,[bx+redTable + step*64]    ; ax = 0bbbggg0 rrrTABLE   ; 5
  xchg ax,si                                                   ; 1
  movsw                                                        ; 5
  xchg ax,si                                                   ; 1
  inc di                                                       ; 1
  inc di                                                       ; 1  ; 201.61 cycles    301 iterations possible in active             29 bytes

; Update is a string of:
; es = 0xb800
  mov word[1234],5678  ; c7 06 1234 5678               ; 6+2 IOs  51.43 cycles   366 word writes possible in inactive                 6 bytes   (29+6)*314 = 10990 bytes unrolled code

;253.04 cycles total for both, 314 iterations maximum. With 8 steps per iteration, wipe takes 203 frames = 3.4s
