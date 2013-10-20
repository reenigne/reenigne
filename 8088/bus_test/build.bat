@echo off
yasm bus_test.asm -o bus_test.bin
yasm cga_bus.asm -o cga_bus.bin
yasm crtc_ma.asm -o crtc_ma.bin
yasm crtc_ra_adjust.asm -o crtc_ra_adjust.bin
yasm 8253.asm -o 8253.bin
