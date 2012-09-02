@echo off
yasm display.asm -o display.com
copy /b display.com+image.bin display2.com
