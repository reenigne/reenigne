@echo off
yasm stability_test.asm -o stable.com
yasm stability_test.asm -o stable.bin -dbin=1
yasm s2.asm -o s2.com
yasm s3.asm -o s3.com

