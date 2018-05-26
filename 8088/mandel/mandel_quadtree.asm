; Allocate a block, return in bx
allocate:
  mov bx,[freeList]
  mov ax,[bx]
  mov [freeList],ax
  ret

; Deallocate block bx
deallocate:
  mov ax,[freeList]
  mov [bx],ax
  mov [freeList],bx
  ret

; Splits unsplit leaf block bx
split:
  mov cl,[bx]
  mov di,bx
  call allocate
  mov [bx],cl
  xchg ax,bx
  stosw
  call allocate
  xchg ax,bx
  stosw
  call allocate
  xchg ax,bx
  stosw
  call allocate
  xchg ax,bx
  stosw
  ret

; Unsplits split block bx
unsplit:
  mov si,bx



freeList: dw 0

