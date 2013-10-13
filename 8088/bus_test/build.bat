@echo off
yasm bus_test.asm -o bus_test.bin
yasm cga_bus.asm -o cga_bus.bin
yasm crtc_ma.asm -o crtc_ma.bin

