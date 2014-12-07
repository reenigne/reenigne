@echo off
yasm flicker.asm -o f.bin
rem copy /b f.com+data.com flicker.com
yasm demo.asm -o demo.com
yasm audio.asm -o audio.com

