@echo off
set HOME=C:\Users\Andrew
set DOIT_HOST=prospero
q:
cd \reenigne\8088\xtce\compare_hardware_lkg
doitclient wcmd xtkill
doitclient wcmd xtrun runtest.bin >runtests.output
