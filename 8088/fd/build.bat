@echo off
yasm fd.asm -o fd.bin
yasm write_image.asm -o write_image.bin
yasm read_image.asm -o read_image.bin
yasm 765.asm -o 765.bin
