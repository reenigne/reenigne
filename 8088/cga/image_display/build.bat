@echo off
yasm display.asm -o display.com
copy /b display.com+image.bin display2.com
yasm dbin.asm -o dbin.bin
copy /b dbin.bin+\fs.dat d.bin
yasm bars.asm -o bars.bin

