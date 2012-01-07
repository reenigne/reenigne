@echo off
yasm fd.asm -o fd.bin
yasm write_image.asm -o write_image.bin

