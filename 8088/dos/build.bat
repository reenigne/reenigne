@echo off
yasm kernel.asm -o kernel.bin
yasm keyboard_kernel.asm -o keyboard_kernel.bin
