@echo off
yasm 1024.asm -o 1.bin
copy /b 1.bin+1024colors.bin 1024.bin
yasm 1spf.asm -o 1spf.bin
yasm 1spf2.asm -o 1spf2.bin
yasm 1spf_intermittent.asm -o 1spf_intermittent.bin -l 1spf_intermittent.lst
yasm 1spf2_intermittent.asm -o 1spf2_intermittent.bin

rem copy /b 1s.bin+1024colors.bin 1spf.bin
