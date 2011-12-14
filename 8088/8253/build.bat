@echo off
yasm 8253.asm -o 8253.bin
doitclient wcmd ..\serial\run\release\run 8253.bin
