@echo off
c:\msvc\bin\cl /nologo /W3 /DNDEBUG /Ox /AT /D_DOS /c /Ic:\msvc\include /Fa particle.cpp
c:\msvc\bin\link c:\msvc\lib\crtcom.lib particle.obj, particle.com, particle.map, c:\msvc\lib\slibce.lib c:\msvc\lib\oldnames.lib,  /NOLOGO /ONERROR:NOEXE /NOI /TINY /NOE
