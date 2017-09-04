@echo off
yasm 1024.asm -o 1.bin
copy /b 1.bin+1024colors.bin 1024.bin
yasm 1spf.asm -o 1s.bin
copy /b 1s.bin+1024colors.bin 1spf.bin
