@echo off
yasm vinst.asm -l vinst.lst
set oldpath=%path%
set path=C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;T:\t\Utils;T:\msvc\bin;..\Tools
nasm mod.asm -o mod.obj -f obj -l mod.lst
REM ..\tools\2obj\2obj b /s q:\projects\code\mod_convert\test1_out.dat song.obj .HUGE:song
..\tools\2obj\2obj b /s test1_out.dat song.obj .HUGE:song
..\tools\2obj\2obj b /s ..\credits.txt credits.obj .HUGE:creditstxt
..\tools\2obj\2obj b /s background.bin background.obj .HUGE:background
..\Tools\msvc\bin\link mod.obj song.obj credits.obj background.obj, song.exe, song.map,, /NOLOGO /ONERROR:NOEXE /NOI /NOE
set path=%oldpath%
