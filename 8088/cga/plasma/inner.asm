  add si,5-1
  and si,0x1ff
  lodsb

  add bp,40
  and bp,0x1ff
  add al,[bp+0]

  add al,dl
;  xchg ax,bx
;;  mov bh,(gradientTable >> 8)
;  mov bl,[bx]
;  xchg ax,bx
  xlatb
  stosb                         ; 118.265 cycles



  add si,5-1
  lodsb
  add bx,40
  add al,[bx]
  xchg ax,si
  mov si,[bp+si]
  xchg ax,si
  stosw                        ; 90 cycles


  add si,cx
  lodsb
  add bx,dx
  add al,[bx]



; Update:

  movsb
  inc di   ; 28.795 cycles  == 654 updates during inactive


; Want 446 plasma iterations == 173.5 scanlines
; 446 updates == 42.2 scanlines
; total == 215.7 scanlines - 46.3 to spare

; With movsw, 446 updates takes ~56.3 scanlines
; Leaves 205.7 scanlines for plasma, ~140 cycles per iteration

       CLL  CLR  CRL  CRR
L 39   45   51   73   87
  40   41   57   71   91
  41   39   59   69   95
  42   39   59   67   95
  43   37   61   67   97
  44   37   61   67   97
  45   37   61   67   97
  46   37   61   67   99
  47   37   61   69   99
  48   37   59   69   99
  49   37   59   69   99
  50   37   59   71   99
  51   37   57   71   99
  52   37   57   71   99
  53   39   57   73   99
  54   39   55   73   99
  55   39   55   75   97
  56   41   53   77   97
  57   41   53   79   95
  58   43   51   81   95
  59   45   49   83   93

00b1 08b0 08b1 80b0
88b1 84b0 84b1 48b0
44b1 45b0 45b1 54b0
55b1 59b0 59b1 95b0
99b1 93b0 93b1 39b0
33b1 3ab0 3ab1 a3b0
aab1 aeb0 aeb1 eab0
eeb1 efb0 efb1 feb0
ffb1 fbb0 fbb1 bfb0
bbb1 bdb0 bdb1 dbb0
ddb1 d6b0 d6b1 6db0
66b1 64b0 64b1 46b0
44b1

00b0 00b0 08b0 08b0
08b1 08b1 08b1 04b1
04b1 04b1 04b1 05b1
05b1 05b1 05b1 09b1
09b1 09b1 09b1 03b1
03b1 03b1 03b1 0ab1
0ab1 0ab1 0ab1 0eb1
0eb1 0eb1 0eb1 0fb1
0fb1 0fb1 0bb1 0bb1
0bb1 0bb1 0db1 0db1
0db1 0db1 06b1 06b1
06b1 06b1 04b1 04b1
04b1

