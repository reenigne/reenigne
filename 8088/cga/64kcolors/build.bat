@echo off
yasm 64k.asm -o 64.bin
copy /b 64.bin+data.bin 64k.bin
