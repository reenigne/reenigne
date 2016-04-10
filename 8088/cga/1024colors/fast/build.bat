@echo off
yasm 1024.asm -o 1.bin
copy /b 1.bin+1024colors.bin 1024.bin
