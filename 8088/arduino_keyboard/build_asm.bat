@echo off
yasm mda_cga.asm -o mda_cga.bin
yasm restart_bios.asm -o restart_bios.bin
yasm chirp.asm -o chirp.bin
yasm testing.asm -o testing.bin

