@echo off
yasm fd.asm -o fd.bin
yasm write_image.asm -o write_image.bin
yasm write_image2.asm -o write_image2.bin
yasm write_verify_image.asm -o write_verify_image.bin
yasm read_image.asm -o read_image.bin
yasm 765.asm -o 765.bin
yasm fdspeed.asm -o fd.com
yasm seek_no_motor.asm -o seek_no_motor.bin
yasm write_imager.asm -o imager.bin
