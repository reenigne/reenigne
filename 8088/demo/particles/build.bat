@echo off
set oldpath=%path%
set path=C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;D:\t\Utils;D:\msvc\bin
D:\msvc\bin\cl /nologo /W3 /DNDEBUG /Ox /AT /D_DOS /c /Id:\msvc\include /Fa particle.cpp
rem D:\msvc\bin\link d:\msvc\lib\crtcom.lib particle.obj STARS.OBJ, particle.com, particle.map, d:\msvc\lib\slibce.lib d:\msvc\lib\oldnames.lib,  /NOLOGO /ONERROR:NOEXE /NOI /TINY /NOE
D:\msvc\bin\link  particle.obj STARS.OBJ, particle.exe, particle.map, d:\msvc\lib\slibce.lib d:\msvc\lib\oldnames.lib,  /NOLOGO /ONERROR:NOEXE /NOI /NOE
rem /STACK:1024
set path=%oldpath%



