@echo off
yasm imager.asm -o imager.bin -l imager.lst
yasm imager_tsr.asm -o imagert.com -l imager_tsr.lst
yasm realfd.asm -o realfd.com
