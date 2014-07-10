rem greyscale \bunny.raw > greyscale.asm
copy g.asm + greyscale.asm g2.asm
yasm g2.asm -o g2.bin
