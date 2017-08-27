  add si,5*2-2
  and si,0x3fe
  lodsw

  add bx,40*2
  and bx,0x3fe
  add ax,[bx]

  add ax,dx

  xchg ax,bx
  mov bh,(gradientTable >> 8)
  mov bl,[bx]
  xchg ax,bx

  stosb                         ; 144 cycles == 422 iterations during active


; Update:

  movsb
  inc di   ; 28.795 cycles  == 654 updates during inactive


; Want 452 plasma iterations == 214 scanlines
; 452 updates == 42.8 scanlines
; total == 256 scanlines - 6 to spare

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
