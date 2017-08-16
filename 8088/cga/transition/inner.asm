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
  inc di                                             ; 1  total: 21 IOs, 96 cycles     633 iterations possible in active

; Update is a string of:
; es = 0xb800
  mov byte[1234],56  ; c6 06 1234 56                 ; 5+1 IOs  34.45 cycles   547 byte writes possible in inactive


; RGB cube:
; ds:si = pointer into wipeSequence (*2)
; es:di = pointer into updateBuffer
; ds:redGreen, blue = combined RGB values from both images, 4 bytes per character = 32000 bytes
; ds:redTable, greenTable, blueTable = 8*64*2*3 = 3072 bytes
; ds:rgb = 2048 bytes
  lodsw                                                        ; 3
  stosw                                                        ; 3
  xchg bx,ax                                                   ; 1
  mov dx,[bx+redGreen]  ; dx =  0rrrRRR0 0gggGGG0              ; 6
  mov bx,[bx+blue]      ; bx =  0bbbBBB0 00000000              ; 6
  mov al,[bx+blueTable + step*128]  ; al = 0bbb0000            ; 5
  mov bl,dh                                                    ; 2
  add al,[bx+greenTable + step*128] ; al = 0bbbggg0            ; 5
  mov bl,dl                                                    ; 2
  mov ah,[bx+redTable + step*128]   ; ax = 0bbbggg0 rrr00000   ; 5
  xchg bx,ax                                                   ; 1
  mov ax,[bx+rgb]                                              ; 6
  stosw                                                        ; 3
  inc di                                                       ; 1
  inc di                                                       ; 1  ; 226.3 cycles    268 iterations possible in active

; Update is a string of:
; es = 0xb800
  mov byte[1234],5678  ; c6 06 1234 5678               ; 6+2 IOs  51.43 cycles   366 word writes possible in inactive

;277.73 cycles total for both, 286 iterations maximum. With 8 steps per iteration, transition takes 224 frames = 3.7s
