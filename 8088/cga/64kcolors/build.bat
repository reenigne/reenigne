@echo off
yasm 64k.asm -o 64.com
copy /b 64.com+data.com 64k.com
