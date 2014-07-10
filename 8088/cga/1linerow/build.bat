rem ..\greyscale\release\greyscale \bunny_320.raw > picture.asm
copy 1linerow.asm + picture.asm 1.asm
yasm 1.asm -o 1.bin
