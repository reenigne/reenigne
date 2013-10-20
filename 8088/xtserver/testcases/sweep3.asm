org 0
cpu 8086


  int 0x61


  mov di,0xffff

  mov bx,di
  mov cx,di

  in al,0x61
  or al,3
  out 0x61,al

  mov al,TIMER2 | BOTH | MODE3 | BINARY
  out 0x43,al

loopTop:

  mov ax,bx  ; high word                         ffff
  mul di     ;                                  *ffff = fffe:0001
  mov bx,dx  ; new high word                     fffe
  xchg cx,ax ; low word <-> new low word              0001 <> ffff
  mul di     ;                                  *ffff = fffe:0001
  add cx,dx  ;                                               ffff
  adc bx,0   ;                                          fffe

;  mov ax,bx
;  int 0x63
;  mov ax,cx
;  int 0x63
;  mov al,10
;  int 0x65

  mov al,bl
  out 0x42,al
  mov al,bh
  out 0x42,al

  cmp bx,0
  jne loopTop
;  cmp cx,0
;  jne loopTop

  in al,0x61
  and al,0xfc
  out 0x61,al


  int 0x62


  int 0x67




;c = d*2^(-b*t)
;
;
;t = 0 : c(0) = d = 65536
;t = 1 : c(1) = 65536*2^(-b*1) = 65535  2^(-b) = 65535/65536  -b = log(65535/65536)  b = log_2(65536/65535) = 0.00002201394726395550201651307832236
;
;
;   d = FFFF:FFFF
;2^-b = 0000:FFFF



;d = bx:cx




; ffff ffff
; fffe ffff
; fffe 0000
; fffd 0002
; fffc 0004
;
; 0001 0000
; 0000 ffff
;      fffe
;      fffd
;      fffc
;      fff
;
;
; fffe 0001
;      fffe
;
; fffe ffff
;
;
; fffe:ffff * ffff =
;
;      fffd 0002
;           fffe 0001
;
;
; fffe:0001 * ffff =
;
;      fffd 0002
;           0000 ffff
;
;
; fffd:0002 * ffff =
;
;      fffc 0003
;           0001 fffe
;
;
; 0001:0000 * ffff =
;
;      0000 ffff
;           0000 ffff
