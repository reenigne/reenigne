@echo off
yasm refresh_test.asm -o refresh_test.bin
yasm good.asm -o good.com
yasm bad.asm -o bad.com
yasm address.asm -o address.com
yasm address0.asm -o address0.com

