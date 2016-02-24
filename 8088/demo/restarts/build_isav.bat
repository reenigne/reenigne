@echo off
yasm restarts_isav.asm -o 1.bin -l restarts_isav.lst
copy /b 1.bin+\rose_out.dat restarts_isav.bin
