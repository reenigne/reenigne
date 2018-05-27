; Allocate a block, return in bx
allocate:
  mov bx,[freeList]
  mov ax,[bx]
  mov [freeList],ax
  ret

; Deallocate leaf block bx
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

; Unsplits split leaf block bx
unsplitLeaf:
  mov si,bx
  lodsw
  xchg ax,bx
  call deallocate
  lodsw
  xchg ax,bx
  call deallocate
  lodsw
  xchg ax,bx
  call deallocate
  lodsw
  xchg ax,bx
  call deallocate
  ret

; Recursively unsplits block bx
unsplit:
  push si
  mov si,bx
  lodsw
  test al,1
  jz .noUnsplit
  xchg ax,bx
  call unsplit
  lodsw
  xchg ax,bx
  call unsplit
  lodsw
  xchg ax,bx
  call unsplit
  lodsw
  xchg ax,bx
  call unsplit
  pop si
  ret

freeList: dw 0

