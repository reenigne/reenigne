@echo off
yasm 8253.asm -o 8253.bin
rem doitclient wcmd ..\serial\run\release\run 8253.bin
yasm check_refresh_on.asm -o check_refresh_on.bin
