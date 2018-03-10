@echo off
set HOME=C:\Users\Andrew
set DOIT_HOST=prospero
q:
cd \reenigne\8088\xtce\gentests
submit.pl andrew@reenigne.org runtest.bin >runtests.output
