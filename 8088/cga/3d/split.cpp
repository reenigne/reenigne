  push ax
  "a" bits are in 0,5,10 already
  Compute "b" in 1,6,11
  Compute "e" in 3,8,13
  Compute "d" in 2,7,12
  push ds
  mov ds,[0]
  call nextLevelTL
  pop ds
  "a" bits no longer needed (mask off 0,5,10)
  Do a lookup to move "b" bits from 1,6,11 to 0,5,10
                      "e" bits from 3,8,13 to 2,7,12
                      "d" bits from 2,7,12 to 4,9,14
  Move "c" bits from 1,6,11 on stack to 1,6,11
  Compute "f" in 3,8,13
  push ds
  mov ds,[2]
  call nextLevelTR
  pop ds
  "b" and "c" bits no longer needed (mask off 0,5,10 and 1,6,11)
  Do a lookup to move "e" bits from 2,7,12 to 0,5,10
                      "f" bits from 3,8,13 to 1,6,11
  Move "i" bits from 3,8,13 on stack to 3,8,13
  Compute "h" in 2,7,12
  "d" bits still in 4,9,14
  push ds
  mov ds,[6]
  call nextLevelBR
  pop ds
  "f" and "i" bits no longer needed (mask off 1,6,11 and 3,8,13)
  Do a lookup to move "e" bits from 0,5,10 to 1,6,11              bits used for lookup: 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14  15 bits so would need 512kB of space. Not reasonable!
                      "h" bits from 2,7,12 to 3,8,13
                      "d" bits from 4,9,14 to 0,5,10
  Move "g" bits from 2,7,12 on stack to 2,7,12

