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

; Initializes freelist with all blocks in DS
initialize:
  mov word[freeList],0
  mov ax,ds
  mov es,ax
  mov di,(listSpace + 7) & -8
  mov si,6
  xor ax,8
  mov dx,ax
  mov cx,(0x10000 - ((listSpace + 7) & -8)) >> 3
.loop:
  stosw
  add di,si
  add ax,dx
  loop .loop
  ret

freeList: dw 0
listSpace:
