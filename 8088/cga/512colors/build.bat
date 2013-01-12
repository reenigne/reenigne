@echo off
yasm 512.asm -o 5.bin
copy /b 5.bin+512colors.bin 512.bin
