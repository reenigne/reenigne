@echo off
yasm serial.asm -o serial.bin
yasm send.asm -o send.bin
yasm kernel.asm -o kernel.bin
yasm test.asm -o test.bin
