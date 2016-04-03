@echo off
yasm isav_timing.asm -o isav_timing.bin -l isav_timing.lst
nasm odd_even_bit.asm -o odd_even_bit.bin
