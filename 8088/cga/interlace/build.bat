@echo off
yasm interlace.asm -o interlace.bin -l interlace.lst
yasm interlace2.asm -o interlace2.bin -l interlace2.lst
yasm lines50.asm -o lines50.com -l lines50.lst
yasm 50.asm -o 50.com
yasm 25.asm -o 25.com
yasm 40.asm -o 40.com
yasm 80.asm -o 80.com
yasm mandala.asm -o mandala.bin
yasm mandala0.asm -o mandala0.bin
yasm mandala0a.asm -o mandala0a.bin
